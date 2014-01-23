////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "Message.hpp"
#include "FullEmptyLocal.hpp"
#include "ConditionVariable.hpp"
#include "MessagePool.hpp"
#include "DelegateBase.hpp"
#include "GlobalCompletionEvent.hpp"
#include "AsyncDelegate.hpp"
#include <type_traits>

GRAPPA_DECLARE_METRIC(SummarizingMetric<uint64_t>, flat_combiner_fetch_and_add_amount);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_reads);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_read_targets);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_writes);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_write_targets);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_cmpswaps);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_cmpswap_targets);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_fetchadds);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_fetchadd_targets);


namespace Grappa {
    /// @addtogroup Delegates
    /// @{
    
  namespace impl {
            
    template< SyncMode S, GlobalCompletionEvent * C, typename F >
    struct Specializer {
      // async call with void return type
      static void call(Core dest, F func, void (F::*mf)() const) {
        delegate_ops++;
        delegate_async_ops++;
        Core origin = Grappa::mycore();
      
        if (dest == origin) {
          // short-circuit if local
          delegate_targets++;
          delegate_short_circuits++;
          func();
        } else {
          if (C) C->enroll();
        
          send_heap_message(dest, [origin, func] {
            delegate_targets++;
            func();
            if (C) C->send_completion(origin);
          });
        }
      }
      
      // async call with return val (via Promise)
      template< typename T >
      static delegate::Promise<T> call(Core dest, F f, T (F::*mf)() const) {
        static_assert(std::is_same<void,T>::value, "not implemented yet");
        // return std::move(Promise<T>(f()));
      }
      
    };
    
    template< GlobalCompletionEvent * C, typename F >
    struct Specializer<SyncMode::Blocking,C,F> {
      template< typename T >
      static auto call(Core dest, F f, T (F::*mf)() const) -> T {
        return impl::call(dest, f, mf); // defined in DelegateBase.hpp
      }
    };    
    
  } // namespace impl
  
  namespace delegate {
    
#define INVOCATION impl::Specializer<S,C,F>::call(dest, f, &F::operator())
    
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename F = decltype(nullptr) >
    auto call(Core dest, F f) -> decltype(INVOCATION) { return INVOCATION; }
    
#undef INVOCATION
    
    /// Try lock on remote mutex. Does \b not lock or unlock, creates a SuspendedDelegate if lock has already
    /// been taken, which is triggered on unlocking of the Mutex.
    template< typename M, typename F >
    inline auto call(Core dest, M mutex, F func) -> decltype(func(mutex())) {
      using R = decltype(func(mutex()));      
      delegate_ops++;
      
      if (dest == mycore()) {
        delegate_targets++;
        delegate_short_circuits++;
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
          delegate_targets++;
          auto l = mutex();
          if (is_unlocked(l)) { // if lock is not held
            // lock(l);
            set_result(func(l));
          } else {
            add_waiter(l, SuspendedDelegate::create([set_result,func,l]{
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
      delegate_ops++;
      using R = decltype(func());
      Core origin = Grappa::mycore();
    
      if (dest == origin) {
        delegate_targets++;
        delegate_short_circuits++;
        return func();
      } else {
        FullEmpty<R> result;
        int64_t network_time = 0;
        int64_t start_time = Grappa::timestamp();
      
        send_message(dest, [&result, origin, func, &network_time, start_time] {
          delegate_targets++;
          
          spawn([&result, origin, func, &network_time, start_time] {
            R val = func();
            // TODO: replace with handler-safe send_message
            send_heap_message(origin, [&result, val, &network_time, start_time] {
              network_time = Grappa::timestamp();
              record_network_latency(start_time);
              result.writeXF(val); // can't block in message, assumption is that result is already empty
            });
          });
        }); // send message
        // ... and wait for the result
        R r = result.readFE();
        record_wakeup_latency(start_time, network_time);
        return r;
      }
    }
    
    /// Read the value (potentially remote) at the given GlobalAddress, blocks the calling task until
    /// round-trip communication is complete.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr) >
    T read(GlobalAddress<T> target) {
      delegate_reads++;
      return call<S,C>(target.core(), [target]() -> T {
        delegate_read_targets++;
        return *target.pointer();
      });
    }
    
    
    /// Remove 'const' qualifier to do read.
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr) >
    T read(GlobalAddress<const T> target) {
      return read<S,C>(static_cast<GlobalAddress<T>>(target));
    }
        
    /// Blocking remote write.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr),
              typename U = decltype(nullptr) >
    void write(GlobalAddress<T> target, U value) {
      static_assert(std::is_convertible<T,U>(), "type of value must match GlobalAddress type");
      delegate_writes++;
      // TODO: don't return any val, requires changes to `delegate::call()`.
      return call<S,C>(target.core(), [target, value] {
        delegate_write_targets++;
        *target.pointer() = value;
      });
    }
    
    /// Fetch the value at `target`, increment the value stored there with `inc` and return the
    /// original value to blocking thread.
    /// @warning Target object must lie on a single node (not span blocks in global address space).
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr),
              typename U = decltype(nullptr) >
    T fetch_and_add(GlobalAddress<T> target, U inc) {
      delegate_fetchadds++;
      return call(target.core(), [target, inc]() -> T {
        delegate_fetchadd_targets++;
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
            result = call(target.core(), [t, increment_total]() -> U {
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
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr),
              typename U = decltype(nullptr),
              typename V = decltype(nullptr) >
    bool compare_and_swap(GlobalAddress<T> target, U cmp_val, V new_val) {
      static_assert(std::is_convertible<T,U>(), "type of cmp_val must match GlobalAddress type");
      static_assert(std::is_convertible<T,V>(), "type of new_val must match GlobalAddress type");
      
      delegate_cmpswaps++;
      return call(target.core(), [target, cmp_val, new_val]() -> bool {
        T * p = target.pointer();
        delegate_cmpswap_targets++;
        if (cmp_val == *p) {
          *p = new_val;
          return true;
        } else {
          return false;
        }
      });
    }
    
    template< SyncMode S = SyncMode::Blocking, 
              GlobalCompletionEvent * C = &impl::local_gce,
              typename T = decltype(nullptr),
              typename U = decltype(nullptr) >
    void increment(GlobalAddress<T> target, U inc) {
      static_assert(std::is_convertible<T,U>(), "type of inc must match GlobalAddress type");
      delegate_async_increments++;
      delegate::call<SyncMode::Async,C>(target.core(), [target,inc]{
        (*target.pointer()) += inc;
      });
    }
    
        
    // /// Implements essentially a remote procedure call. Callable object (lambda,
    // /// function pointer, or functor object) is called from the `dest` core and the return
    // /// value is sent back to the calling task.
    // template< SyncMode S = SyncMode::Blocking, typename F = decltype(nullptr) >
    // inline auto call(Core dest, F func) -> decltype(func()) {
    //   if (S == SyncMode::Blocking) {
    //     return impl::call(dest, func, &F::operator());
    //   } else {
    //     return impl::call_async()
    //   }
    // }
  
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
    
  } // namespace delegate
  
  /// Synchronizing remote private task spawn. Automatically enrolls task with GlobalCompletionEvent and
  /// sends `complete`  message when done (if C is non-null).  
  template< TaskMode B = TaskMode::Bound,
            GlobalCompletionEvent * C = &impl::local_gce,
            typename F = decltype(nullptr) >
  void spawnRemote(Core dest, F f) {
    if (C) C->enroll();
    Core origin = mycore();
    delegate::call<SyncMode::Async,C>(dest, [origin,f] {
      spawn<B>([origin,f] {
        f();
        if (C) C->send_completion(origin);
      });
    });
  }
    
} // namespace Grappa
/// @}


