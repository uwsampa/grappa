
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__


#include "Grappa.hpp"
#include "Message.hpp"
#include "FullEmpty.hpp"
#include "Message.hpp"
#include "ConditionVariable.hpp"

#include "MessagePool.hpp"
#include "AsyncDelegate.hpp"

namespace Grappa {
  namespace delegate {
    /// @addtogroup Delegates
    /// @{
    
    /// Implements essentially a blocking remote procedure call. Callable object (lambda,
    /// function pointer, or functor object) is called from the `dest` core and the return
    /// value is sent back to the calling task.
    template <typename F>
    inline auto call(Core dest, F func) -> decltype(func()) {
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
      Node origin = Grappa_mynode();
      
      if (dest == origin) {
        // short-circuit if local
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
    
    /// Read the value (potentially remote) at the given GlobalAddress, blocks the calling task until
    /// round-trip communication is complete.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T >
    T read(GlobalAddress<T> target) {
      delegate_stats.count_word_read();
      return call(target.node(), [target]() -> T {
        delegate_stats.count_word_read_am();
        return *target.pointer();
      });
    }
    
    /// Blocking remote write.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U >
    bool write(GlobalAddress<T> target, U value) {
      delegate_stats.count_word_write();
      // TODO: don't return any val, requires changes to `delegate::call()`.
      return call(target.node(), [target, value]() -> bool {
        delegate_stats.count_word_write_am();
        *target.pointer() = value;
        return true;
      });
    }
    
    /// Fetch the value at `target`, increment the value stored there with `inc` and return the
    /// original value to blocking thread.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U >
    T fetch_and_add(GlobalAddress<T> target, U inc) {
      delegate_stats.count_word_fetch_add();
      T * p = target.pointer();
      return call(target.node(), [p, inc]() -> T {
        delegate_stats.count_word_fetch_add_am();
        T r = *p;
        *p += inc;
        return r;
      });
    }
    
    /// If value at `target` equals `cmp_val`, set the value to `new_val` and return `true`,
    /// otherwise do nothing and return `false`.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U, typename V >
    bool compare_and_swap(GlobalAddress<T> target, U cmp_val, V new_val) {
      delegate_stats.count_word_compare_swap();
      T * p = target.pointer();
      return call(target.node(), [p, cmp_val, new_val]() -> bool {
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
