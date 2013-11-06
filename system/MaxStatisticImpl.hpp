#pragma once

#include "MaxStatistic.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void MaxStatistic<T>::merge_all(impl::StatisticBase* static_stat_ptr) {
    this->value_ = 0;
    
    // TODO: use more generalized `reduce` operation to merge all
    MaxStatistic<T>* this_static = reinterpret_cast<MaxStatistic<T>*>(static_stat_ptr);
    
    GlobalAddress<MaxStatistic<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<MaxStatistic<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          MaxStatistic<T>* s = remote_stat.pointer();
          T s_value = s->value_;
          
          send_heap_message(combined_addr.node(), [combined_addr, s_value, &ce] {
              // for this simple MaxStatistic, merging is as simple as accumulating the value
              MaxStatistic<T>* combined_ptr = combined_addr.pointer();
              combined_ptr->value_ = std::max(combined_ptr->value_, s_value);
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
