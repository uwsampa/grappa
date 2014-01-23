
#pragma once

#include "CallbackMetric.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void CallbackMetric<T>::merge_all(impl::MetricBase* static_stat_ptr) {
    //TODO: make merge_all functional to avoid this ugly jekel and hyde
    this->start_merging();
    
    CallbackMetric<T>* this_static = reinterpret_cast<CallbackMetric<T>*>(static_stat_ptr);
    
    GlobalAddress<CallbackMetric<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<CallbackMetric<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          CallbackMetric<T>* s = remote_stat.pointer();
          T s_value = s->value();
          
          send_heap_message(combined_addr.core(), [combined_addr, s_value, &ce] {
              // merging is as simple as accumulating the value
              CallbackMetric<T>* combined_ptr = combined_addr.pointer();
              combined_ptr->merging_value_ += s_value;
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
