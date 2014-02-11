#pragma once

#include "SimpleMetric.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void SimpleMetric<T>::merge_all(impl::MetricBase* static_stat_ptr) {
    this->value_ = 0;
    
    // TODO: use more generalized `reduce` operation to merge all
    SimpleMetric<T>* this_static = reinterpret_cast<SimpleMetric<T>*>(static_stat_ptr);
    
    GlobalAddress<SimpleMetric<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<SimpleMetric<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          SimpleMetric<T>* s = remote_stat.pointer();
          T s_value = s->value_;
          
          send_heap_message(combined_addr.core(), [combined_addr, s_value, &ce] {
              // for this simple SimpleMetric, merging is as simple as accumulating the value
              SimpleMetric<T>* combined_ptr = combined_addr.pointer();
              if (combined_ptr->initf_ != NULL) {
                // min
                if (combined_ptr->value_ > s_value) combined_ptr->value_ = s_value;
              } else {
                //sum
                combined_ptr->value_ += s_value;
              }
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
