#pragma once

#include "Communicator.hpp"
#include "PoolAllocator.hpp"
#include "FullEmptyLocal.hpp"
#include "LegacyDelegate.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include <type_traits>
#include "Statistics.hpp"

GRAPPA_DECLARE_STAT(SimpleStatistic<int64_t>, delegate_async_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<int64_t>, delegate_async_writes);
GRAPPA_DECLARE_STAT(SimpleStatistic<int64_t>, delegate_async_increments);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, delegate_ops_short_circuited);

namespace Grappa {
  
  namespace delegate {
    /// @addtogroup Delegates
    /// @{
    
    /// Do asynchronous generic delegate with `void` return type. Uses message pool to allocate
    /// the message. Enrolls with GCE so you can guarantee all have completed after a global
    /// GlobalCompletionEvent::wait() call.
    template<GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, typename PoolType = impl::MessagePoolBase, typename F = decltype(nullptr)>
    inline void call_async(PoolType& pool, Core dest, F remote_work) {
      static_assert(std::is_same< decltype(remote_work()), void >::value, "return type of callable must be void when not associated with Promise.");
      delegate_stats.count_op();
      delegate_async_ops++;
      Core origin = Grappa::mycore();
      
      if (dest == origin) {
        // short-circuit if local
        delegate_ops_short_circuited++;
        remote_work();
      } else {
        if (GCE) GCE->enroll();
        
        pool.send_message(dest, [origin, remote_work] {
          delegate_stats.count_op_am();
          remote_work();
          if (GCE) complete(make_global(GCE,origin));
        });
      }
    }
    
    /// Uses `call_async()` to write a value asynchronously.
    ///
    /// @note The helper struct Grappa::delegate::write_msg_proxy can be used to compute the
    ///       size of a pool of this kind of delegate.
    ///
    /// @b Example:
    /// @code
    ///   GlobalAddress<int> array;
    ///   MessagePool pool(10*sizeof(delegate::write_msg_proxy<int>));
    ///   for (int i=0; i<10; i++) {
    ///     write_async(pool, array+i, i);
    ///   }
    /// @endcode
    template<GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, typename T = decltype(nullptr), typename U = decltype(nullptr), typename PoolType = impl::MessagePoolBase >
    inline void write_async(PoolType& pool, GlobalAddress<T> target, U value) {
      delegate_async_writes++;
      delegate::call_async<GCE>(pool, target.core(), [target,value]{
        (*target.pointer()) = value;
      });
    }
    
    /// To help calculate pool sizes
    /// TODO: refactor call_async so it actually uses this struct so we don't have to maintain two
    template<typename T>
    struct write_msg_proxy {
      struct func {
        Core origin;
        GlobalAddress<T> target;
        T value;
        void operator()() {}
      };
      Message<func> msg;
    };
    
    /// Uses `call_async()` to atomically increment a value asynchronously.
    /// @see Grappa::delegate::write_async for example use.
    template< GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, typename T = void, typename U = void, typename PoolType = impl::MessagePoolBase >
    inline void increment_async(PoolType& pool, GlobalAddress<T> target, U increment) {
      delegate_async_increments++;
      delegate::call_async<GCE>(pool, target.core(), [target,increment]{
        (*target.pointer()) += increment;
      });
    }

    /// Overload Grappa::delegate::increment_async to use global message pool
    template< GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, typename T = void, typename U = void, typename PoolType = impl::MessagePoolBase >
    inline void increment_async(GlobalAddress<T> target, U increment) {
      increment_async( *shared_pool, target, increment );
    }

    
    /// A 'Promise' is a wrapper around a FullEmpty for async delegates with return values.
    /// The idea is to allocate storage for the result, issue the delegate request, and then
    /// block on waiting for the value when it's needed.
    ///
    /// @b Example:
    /// @code
    ///   delegate::Promise<int> x;
    ///   x.call_async(1, []()->int { return value; });
    ///   // other work
    ///   myvalue += x.get();
    /// @endcode
    ///
    // TODO: make this so you you can issue a call_async() directly and get a
    // delegate::Promise back. This should be able to be done using the same mechanism for
    // creating Messages (compiler-optimized-away move).
    template<typename R>
    class Promise {
      
      FullEmpty<R> _result;
      
      int64_t start_time;
      int64_t network_time;
      
    public:
      Promise(): _result(), network_time(0), start_time(0) {}
      
      inline void fill(const R& r) {
        _result.writeXF(r);
      }
      
      /// Block on result being returned.
      inline const R& get() {
        // ... and wait for the result
        const R& r = _result.readFF();
        delegate_stats.record_wakeup_latency(start_time, network_time);
        return r;
      }
      
      /// Call `func` on remote node, returning immediately.
      template <typename F>
      void call_async(impl::MessagePoolBase& pool, Core dest, F func) {
        static_assert(std::is_same<R, decltype(func())>::value, "return type of callable must match the type of this Promise");
        _result.reset();
        delegate_stats.count_op();
        delegate_async_ops++;
        Core origin = Grappa::mycore();
        
        if (dest == origin) {
          // short-circuit if local
          fill(func());
        } else {
          start_time = Grappa_get_timestamp();
          
          pool.send_message(dest, [origin, func, this] {
            delegate_stats.count_op_am();
            R val = func();
            
            // TODO: replace with handler-safe send_message
            send_heap_message(origin, [val, this] {
              this->network_time = Grappa_get_timestamp();
              delegate_stats.record_network_latency(this->start_time);
              this->fill(val);
            });
          }); // send message
        }
      }
    };
    
    /// @}
  } // namespace delegate
  
} // namespace Grappa
