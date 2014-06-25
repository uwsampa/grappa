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
#include "Addressing.hpp"
#include "FullEmptyLocal.hpp"
#include "Metrics.hpp"

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_short_circuits);

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, delegate_roundtrip_latency);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, delegate_network_latency);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, delegate_wakeup_latency);

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, delegate_targets);

namespace Grappa {
  
  namespace impl {
    
    inline void record_network_latency(int64_t start_time) {
      auto latency = Grappa::timestamp() - start_time;
      delegate_network_latency += latency;
    }

    inline void record_wakeup_latency(int64_t start_time, int64_t network_time) {
      auto current_time = Grappa::timestamp();
      auto blocked_time = current_time - start_time;
      auto wakeup_latency = current_time - network_time;
      delegate_roundtrip_latency += blocked_time;
      delegate_wakeup_latency += wakeup_latency;
    }
    
    
    // blocking call (void return type)
    template< typename F >
    void call(Core dest, F func, void (F::*mf)() const) {
      delegate_ops++;
      Core origin = Grappa::mycore();

      if (dest == origin) {
        // short-circuit if local
        delegate_short_circuits++;
        func();
      } else {
        
        struct Desc {
          int64_t network_time;
          int64_t start_time;
        } desc;
        
        desc.network_time = 0;
        desc.start_time = Grappa::timestamp();
        
        FullEmpty<Desc*> result(&desc);
        result.readFE();
        auto ra = make_global(&result);
        
        send_message(dest, [ra,func] {
          delegate_targets++;
  
          func();
  
          // TODO: replace with handler-safe send_message
          send_heap_message(ra.core(), [ra] {
            auto r = ra->readXX();
            r->network_time = Grappa::timestamp();
            record_network_latency(r->start_time);
            ra->writeXF(r);
          });
        }); // send message

        // ... and wait for the call to complete
        result.readFF();
        record_wakeup_latency(desc.start_time, desc.network_time);
      }
    }

    // blocking call (with return val)
    template< typename F, typename T >
    auto call(Core dest, F func, T (F::*mf)() const) -> T {
      static_assert(std::is_convertible< decltype(func()), T >(),
                    "lambda doesn't return the expected type");
      // Note: code below (calling call_async) could be used to avoid duplication of code,
      // but call_async adds some overhead (object creation overhead, especially for short
      // -circuit case)
      //   Promise<T> a;
      //   a.call_async(dest, func);
      //   return a.get();
      // TODO: find a way to implement using async version that doesn't introduce overhead

      delegate_ops++;
      Core origin = Grappa::mycore();

      if (dest == origin) {
        // short-circuit if local
        delegate_targets++;
        delegate_short_circuits++;
        return func();
      } else {
        
        struct Desc {
          T result;
          int64_t network_time;
          int64_t start_time;
        } desc;
        
        desc.network_time = 0;
        desc.start_time = Grappa::timestamp();
        
        FullEmpty<Desc*> dfe(&desc);
        dfe.readFE();
        auto da = make_global(&dfe);
        
        send_message(dest, [da,func] {
          delegate_targets++;
          
          auto val = func();
          
          // TODO: replace with handler-safe send_message
          send_heap_message(da.core(), [da,val] {
            auto d = da->readXX();
            d->result = val;
            d->network_time = Grappa::timestamp();
            record_network_latency(d->start_time);
            da->writeXF(d);
          });
        }); // send message
        
        // ... and wait for the call to complete
        dfe.readFF();
        record_wakeup_latency(desc.start_time, desc.network_time);
        return desc.result;
      }
    }
    
    template< typename F >
    auto call(Core dest, F func) -> decltype(func()) {
      return call(dest, func, &F::operator());
    }
    
  } // namespace impl

} // namespace Grappa

