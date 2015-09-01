////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "Communicator.hpp"
#include "FullEmptyLocal.hpp"
#include "GlobalCompletionEvent.hpp"
// #include "ParallelLoop.hpp"
#include <type_traits>
#include "Metrics.hpp"

GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, delegate_async_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, delegate_async_writes);
GRAPPA_DECLARE_METRIC(SimpleMetric<int64_t>, delegate_async_increments);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_short_circuits);

namespace Grappa {
  
  namespace delegate {
    /// @addtogroup Delegates
    /// @{
    
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
      inline const R get() {
        // ... and wait for the result
        const R r = _result.readFF();
        Grappa::impl::record_wakeup_latency(start_time, network_time);
        return r;
      }
      
      /// Call `func` on remote node, returning immediately.
      template <typename F>
      void call_async(Core dest, F func) {
        static_assert(std::is_same<R, decltype(func())>::value, "return type of callable must match the type of this Promise");
        _result.reset();
        delegate_ops++;
        delegate_async_ops++;
        Core origin = Grappa::mycore();
        
        if (dest == origin) {
          // short-circuit if local
          delegate_targets++;
          delegate_short_circuits++;
          fill(func());
        } else {
          start_time = Grappa::timestamp();
          
          send_heap_message(dest, [origin, func, this] {
            delegate_targets++;
            R val = func();
            
            // TODO: replace with handler-safe send_message
            send_heap_message(origin, [val, this] {
              this->network_time = Grappa::timestamp();
              Grappa::impl::record_network_latency(this->start_time);
              this->fill(val);
            });
          }); // send message
        }
      }
    };
    
    /// @}
  } // namespace delegate



  /*
   * Grappa utilities enabled
   * by async delegate
   */

  
} // namespace Grappa
