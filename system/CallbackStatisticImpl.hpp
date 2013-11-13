
#pragma once

#include "CallbackStatistic.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void CallbackStatistic<T>::merge_all(impl::StatisticBase* static_stat_ptr) {
    //TODO: make merge_all functional to avoid this ugly jekel and hyde
    this->start_merging();
    
    CallbackStatistic<T>* this_static = reinterpret_cast<CallbackStatistic<T>*>(static_stat_ptr);
    
    GlobalAddress<CallbackStatistic<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<CallbackStatistic<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          CallbackStatistic<T>* s = remote_stat.pointer();
          T s_value = s->value();
          
          send_heap_message(combined_addr.node(), [combined_addr, s_value, &ce] {
              // merging is as simple as accumulating the value
              CallbackStatistic<T>* combined_ptr = combined_addr.pointer();
              combined_ptr->merging_value_ += s_value;
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
