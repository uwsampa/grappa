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

#include "MaxMetric.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void MaxMetric<T>::merge_all(impl::MetricBase* static_stat_ptr) {
    this->value_ = 0;
    
    // TODO: use more generalized `reduce` operation to merge all
    MaxMetric<T>* this_static = reinterpret_cast<MaxMetric<T>*>(static_stat_ptr);
    
    GlobalAddress<MaxMetric<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<MaxMetric<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          MaxMetric<T>* s = remote_stat.pointer();
          T s_value = s->value_;
          
          send_heap_message(combined_addr.core(), [combined_addr, s_value, &ce] {
              // for this simple MaxMetric, merging is as simple as accumulating the value
              MaxMetric<T>* combined_ptr = combined_addr.pointer();
              combined_ptr->value_ = std::max(combined_ptr->value_, s_value);
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
