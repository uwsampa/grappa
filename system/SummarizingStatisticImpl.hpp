#pragma once

#include "SummarizingStatistic.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void SummarizingStatistic<T>::merge_all(impl::StatisticBase* static_stat_ptr) {
    this->value_ = 0;
    this->n = 0;
    this->mean = 0.0;
    this->M2 = 0.0;
    
    // TODO: use more generalized `reduce` operation to merge all
    SummarizingStatistic<T>* this_static = reinterpret_cast<SummarizingStatistic<T>*>(static_stat_ptr);
    
    GlobalAddress<SummarizingStatistic<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<SummarizingStatistic<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          SummarizingStatistic<T>* s = remote_stat.pointer();
          T s_value = s->value_;
          size_t s_n = s->n;
          double s_mean = s->mean;
          double s_M2 = s->M2;

          send_heap_message(combined_addr.node(), [combined_addr, s_value, s_n, s_mean, s_M2, &ce] {
              SummarizingStatistic<T>* combined_ptr = combined_addr.pointer();
              if( s_n > 0 ) { // update only if we have accumulated samples
                combined_ptr->value_ += s_value;
                if( 0 == combined_ptr->n ) { // overwrite if we are the first samples
                  combined_ptr->n = s_n;
                  combined_ptr->mean = s_mean;
                  combined_ptr->M2 = s_M2;
                } else {
                  size_t new_n = s_n + combined_ptr->n;
                  double delta = s_mean - combined_ptr->mean;
                  double new_mean = combined_ptr->mean + delta * ( static_cast<double>(s_n) / new_n );
                  double new_M2 = combined_ptr->M2 + s_M2 + 
                    delta * delta * combined_ptr->n * s_n / new_n;
                  
                  combined_ptr->n = new_n;
                  combined_ptr->mean = new_mean;
                  combined_ptr->M2 = new_M2;
                }
              }
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
