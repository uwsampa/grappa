
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__


#include "Grappa.hpp"
#include "Message.hpp"
#include "FullEmptyLocal.hpp"
#include "Message.hpp"
#include "ConditionVariable.hpp"
#include "LegacyDelegate.hpp"
#include "MessagePool.hpp"
#include <type_traits>

GRAPPA_DECLARE_STAT(SummarizingStatistic<uint64_t>, flat_combiner_fetch_and_add_amount);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, delegate_ops_short_circuited);

GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, delegate_op_roundtrip_latency);

GRAPPA_DECLARE_STAT(SimpleStatistic<int64_t>, delegate_ops_small_msg);

#define SMALL_MSG_SIZE 16

namespace Grappa {
    /// @addtogroup Delegates
    /// @{
    
  namespace impl {
    
    /// Overloaded version for func with void return type.
    template <typename F>
    inline void call(Core dest, F func, void (F::*mf)() const) {
      delegate_stats.count_op();
      Core origin = Grappa::mycore();
    
      if (dest == origin) {
        // short-circuit if local
        delegate_ops_short_circuited++;
        return func();
      } else {
        int64_t network_time = 0;
        int64_t start_time = Grappa_get_timestamp();
        FullEmpty<bool> result;
        send_message(dest, [&result, origin, func, &network_time, start_time] {
          delegate_stats.count_op_am();
        
          func();
        
          // TODO: replace with handler-safe send_message
          send_heap_message(origin, [&result, &network_time, start_time] {
            network_time = Grappa_get_timestamp();
            delegate_stats.record_network_latency(start_time);
            result.writeXF(true);
          });
        }); // send message
      
        // ... and wait for the call to complete
        result.readFF();
        delegate_stats.record_wakeup_latency(start_time, network_time);
        return;
      }
    }
  
    template <typename F>
    inline auto call(Core dest, F func, decltype(func()) (F::*mf)() const) -> decltype(func()) {
      // Note: code below (calling call_async) could be used to avoid duplication of code,
      // but call_async adds some overhead (object creation overhead, especially for short
      // -circuit case and extra work in MessagePool)
      //   AsyncHandle<decltype(func())> a;
      //   MessagePool<sizeof(Message<F>)> pool;
      //   a.call_async(pool, dest, func);
      //   return a.get_result();
      // TODO: find a way to implement using async version that doesn't introduce overhead
    
      delegate_stats.count_op();
      using R = decltype(func());
      Core origin = Grappa::mycore();
    
      if (dest == origin) {
        // short-circuit if local
        delegate_ops_short_circuited++;
        return func();
      } else {
        FullEmpty<R> result;
        int64_t network_time = 0;
        int64_t start_time = Grappa_get_timestamp();
      
        send_message(dest, [&result, origin, func, &network_time, start_time] {
          delegate_stats.count_op_am();
          R val = func();
        
          // TODO: replace with handler-safe send_message
          send_heap_message(origin, [&result, val, &network_time, start_time] {
            network_time = Grappa_get_timestamp();
            delegate_stats.record_network_latency(start_time);
            result.writeXF(val); // can't block in message, assumption is that result is already empty
          });
        }); // send message
        // ... and wait for the result
        R r = result.readFE();
        delegate_stats.record_wakeup_latency(start_time, network_time);
        return r;
      }
    }
    
  } // namespace impl
  
  namespace delegate {
    
    /// Implements essentially a blocking remote procedure call. Callable object (lambda,
    /// function pointer, or functor object) is called from the `dest` core and the return
    /// value is sent back to the calling task.
    template <typename F>
    inline auto call(Core dest, F func) -> decltype(func()) {
      return impl::call(dest, func, &F::operator());
    }
    
    /// Helper that makes it easier to implement custom delegate operations on global
    /// addresses specifically.
    ///
    /// Example:
    /// @code
    ///   GlobalAddress<int> xa;
    ///   bool is_zero = delegate::call(xa, [](int* x){ return *x == 0; });
    /// @endcode
    template< typename T, typename F >
    inline auto call(GlobalAddress<T> target, F func) -> decltype(func(target.pointer())) {
      return call(target.core(), [target,func]{ return func(target.pointer()); });
    }
    
    /// Try lock on remote mutex. Does \b not lock or unlock, creates a SuspendedDelegate if lock has already
    /// been taken, which is triggered on unlocking of the Mutex.
    template< typename M, typename F >
    inline auto call(Core dest, M mutex, F func) -> decltype(func(mutex())) {
      using R = decltype(func(mutex()));      
      delegate_stats.count_op();
      
      if (dest == mycore()) {
        delegate_ops_short_circuited++;
        // auto l = mutex();
        // lock(l);
        auto r = func(mutex());
        // unlock(l);
        return r;
      } else {
        FullEmpty<R> result;
        auto result_addr = make_global(&result);
        auto set_result = [result_addr](const R& val){
          send_heap_message(result_addr.core(), [result_addr,val]{
            result_addr->writeXF(val);
          });
        };
      
        send_message(dest, [set_result,mutex,func] {
          auto l = mutex();
          if (is_unlocked(l)) { // if lock is not held
            // lock(l);
            set_result(func(l));
          } else {
            add_waiter(l, new_suspended_delegate([set_result,func,l]{
              // lock(l);
              CHECK(is_unlocked(l));
              set_result(func(l));
            }));
          }
        });
        auto r = result.readFE();
        return r;
      }
    }
    
    /// Alternative version of delegate::call that spawns a privateTask to allow the delegate 
    /// to perform suspending actions.
    /// 
    /// @note Use of this is not advised: suspending violates much of the assumptions about
    /// delegates we usually make, and can easily cause deadlock if no workers are available 
    /// to execute the spawned privateTask. A better option for possibly-blocking delegates 
    /// is to use the Mutex version of delegate::call(Core,M,F).
    template <typename F>
    inline auto call_suspendable(Core dest, F func) -> decltype(func()) {
      delegate_stats.count_op();
      using R = decltype(func());
      Core origin = Grappa::mycore();
    
      if (dest == origin) {
        delegate_ops_short_circuited++;
        return func();
      } else {
        FullEmpty<R> result;
        int64_t network_time = 0;
        int64_t start_time = Grappa_get_timestamp();
      
        send_message(dest, [&result, origin, func, &network_time, start_time] {
          delegate_stats.count_op_am();
          
          privateTask([&result, origin, func, &network_time, start_time] {
            R val = func();
            // TODO: replace with handler-safe send_message
            send_heap_message(origin, [&result, val, &network_time, start_time] {
              network_time = Grappa_get_timestamp();
              delegate_stats.record_network_latency(start_time);
              result.writeXF(val); // can't block in message, assumption is that result is already empty
            });
          });
        }); // send message
        // ... and wait for the result
        R r = result.readFE();
        delegate_stats.record_wakeup_latency(start_time, network_time);
        return r;
      }
    }
    
    /// Read the value (potentially remote) at the given GlobalAddress, blocks the calling task until
    /// round-trip communication is complete.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< typename T >
    T read(GlobalAddress<T> target) {
      delegate_stats.count_word_read();
      return call(target.node(), [target]() -> T {
        delegate_stats.count_word_read_am();
        return *target.pointer();
      });
    }
    
    /// Blocking remote write.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< typename T, typename U >
    void write(GlobalAddress<T> target, U value) {
      delegate_stats.count_word_write();
      // TODO: don't return any val, requires changes to `delegate::call()`.
      return call(target.node(), [target, value] {
        delegate_stats.count_word_write_am();
        *target.pointer() = value;
      });
    }
    
    /// Fetch the value at `target`, increment the value stored there with `inc` and return the
    /// original value to blocking thread.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< typename T, typename U >
    T fetch_and_add(GlobalAddress<T> target, U inc) {
      delegate_stats.count_word_fetch_add();
      return call(target.node(), [target, inc]() -> T {
        delegate_stats.count_word_fetch_add_am();
        T* p = target.pointer();
        T r = *p;
        *p += inc;
        return r;
      });
    }

    /// Flat combines fetch_and_add to a single global address
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template < typename T, typename U >
    class FetchAddCombiner {
      // TODO: generalize to define other types of combiners
      private:
        // configuration
        const GlobalAddress<T> target;
        const U initVal;
        const uint64_t flush_threshold;
       
        // state 
        T result;
        U increment;
        uint64_t committed;
        uint64_t participant_count;
        uint64_t ready_waiters;
        bool outstanding;
        ConditionVariable untilNotOutstanding;
        ConditionVariable untilReceived;

        // wait until fetch add unit is in aggregate mode 
        // TODO: add concurrency (multiple fetch add units)
        void block_until_ready() {
          while ( outstanding ) {
            ready_waiters++;
            Grappa::wait(&untilNotOutstanding);
            ready_waiters--;
          }
        }

        void set_ready() {
          outstanding = false;
          Grappa::broadcast(&untilNotOutstanding);
        }
         
        void set_not_ready() {
          outstanding = true;
        }

      public:
        FetchAddCombiner( GlobalAddress<T> target, uint64_t flush_threshold, U initVal ) 
         : target( target )
         , initVal( initVal )
         , flush_threshold( flush_threshold )
         , result()
         , increment( initVal )
         , committed( 0 )
         , participant_count( 0 )
         , ready_waiters( 0 )
         , outstanding( false )
         , untilNotOutstanding()
         , untilReceived()
         {}

        /// Promise that in the future
        /// you will call `fetch_and_add`.
        /// 
        /// Must be called before a call to `fetch_and_add`
        ///
        /// After calling promise, this task must NOT have a dependence on any
        /// `fetch_and_add` occurring before it calls `fetch_and_add` itself
        /// or deadlock may occur.
        ///
        /// For good performance, should allow other
        /// tasks to run before calling `fetch_and_add`
        void promise() {
          committed += 1;
        }
        // because tasks run serially, promise() replaces the flat combining tree

        T fetch_and_add( U inc ) {

          block_until_ready();

          // fetch add unit is now aggregating so add my inc

          participant_count++;
          committed--;
          increment += inc;
        
          // if I'm the last entered client and either the flush threshold
          // is reached or there are no more committed participants then start the flush 
          if ( ready_waiters == 0 && (participant_count >= flush_threshold || committed == 0 )) {
            set_not_ready();
            uint64_t increment_total = increment;
            flat_combiner_fetch_and_add_amount += increment_total;
            auto t = target;
            result = call(target.node(), [t, increment_total]() -> U {
              T * p = t.pointer();
              uint64_t r = *p;
              *p += increment_total;
              return r;
            });
            // tell the others that the result has arrived
            Grappa::broadcast(&untilReceived);
          } else {
            // someone else will start the flush
            Grappa::wait(&untilReceived);
          }

          uint64_t my_start = result;
          result += inc;
          participant_count--;
          increment -= inc;   // for validation purposes (could just set to 0)
          if ( participant_count == 0 ) {
            CHECK( increment == 0 ) << "increment = " << increment << " even though all participants are done";
            set_ready();
          }

          return my_start;
        }
    };

    
    /// If value at `target` equals `cmp_val`, set the value to `new_val` and return `true`,
    /// otherwise do nothing and return `false`.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< typename T, typename U, typename V >
    bool compare_and_swap(GlobalAddress<T> target, U cmp_val, V new_val) {
      delegate_stats.count_word_compare_swap();
      return call(target.node(), [target, cmp_val, new_val]() -> bool {
        T * p = target.pointer();
        delegate_stats.count_word_compare_swap_am();
        if (cmp_val == *p) {
          *p = new_val;
          return true;
        } else {
          return false;
        }
      });
    }
    
    /// @}
  } // namespace delegate
} // namespace Grappa

#endif
