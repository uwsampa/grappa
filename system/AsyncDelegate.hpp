#pragma once

#include "Communicator.hpp"
#include "PoolAllocator.hpp"
#include "FullEmpty.hpp"
#include "LegacyDelegate.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include <type_traits>

namespace Grappa {
  
  namespace delegate {
    
    /// Do asynchronous generic delegate with `void` return type. Uses message pool to allocate
    /// the message. Enrolls with GCE so you can guarantee all have completed after a global
    /// GlobalCompletionEvent::wait() call.
    template<GlobalCompletionEvent * GCE = &Grappa::impl::local_gce, typename F = decltype(nullptr)>
    inline void call_async(MessagePoolBase& pool, Core dest, F remote_work) {
      static_assert(std::is_same< decltype(remote_work()), void >::value, "return type of callable must be void when not associated with Promise.");
      delegate_stats.count_op();
      Core origin = Grappa::mycore();
      
      if (dest == origin) {
        // short-circuit if local
        remote_work();
      } else {
        delegate_stats.count_op_am();
        if (GCE) GCE->enroll();
        
        pool.send_message(dest, [origin, remote_work] {
          remote_work();
          if (GCE) complete(make_global(GCE,origin));
        });
      }
    }
    
    template<typename T, typename U, GlobalCompletionEvent * GCE = &Grappa::impl::local_gce >
    inline void increment_async(MessagePoolBase& pool, GlobalAddress<T> target, U increment) {
      delegate::call_async(target.core(), [increment]{
        (target.pointer()) += increment;
      });
    }
    
    /// A 'Promise' is a wrapper around a FullEmpty for async delegates with return values.
    /// The idea is to allocate storage for the result, issue the delegate request, and then
    /// block on waiting for the value when it's needed.
    ///
    /// Usage example: @code {
    ///   delegate::Promise<int> x;
    ///   x.call_async(1, []()->int { return value; });
    ///   // other work
    ///   myvalue += x.get();
    /// }
    ///
    /// TODO: make this so you you can issue a call_async() directly and get a
    /// delegate::Promise back. This should be able to be done using the same mechanism for
    /// creating Messages (compiler-optimized-away move).
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
      
      template <typename F>
      void call_async(MessagePoolBase& pool, Core dest, F func) {
        static_assert(std::is_same<R, decltype(func())>::value, "return type of callable must match the type of this Promise");
        _result.reset();
        delegate_stats.count_op();
        Core origin = Grappa::mycore();
        
        if (dest == origin) {
          // short-circuit if local
          fill(func());
        } else {
          delegate_stats.count_op_am();
          start_time = Grappa_get_timestamp();
          
          pool.send_message(dest, [origin, func, this] {
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
  } // namespace delegate
  
} // namespace Grappa
