#pragma once

#include "StringMetric.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  void StringMetric::merge_all(impl::MetricBase* static_stat_ptr) {
    this->write_value("");
    
    // TODO: use more generalized `reduce` operation to merge all
    StringMetric* this_static = reinterpret_cast<StringMetric*>(static_stat_ptr);
    DVLOG(3) << "master node string " << this_static->value_;
    
    GlobalAddress<StringMetric> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<StringMetric> remote_stat = make_global(this_static, c);
      
      // TODO: remove 'this' capture for gcc4.8+; bug in 4.7 requires it here
      send_heap_message(c, [remote_stat, combined_addr, &ce, this] {
          StringMetric* s = remote_stat.pointer();
          char s_value[StringMetric::max_string_size];
          DVLOG(4) << "new string to accum " << s->value_;
          StringMetric::write_chars(s_value, s->value_);
          
          send_heap_message(combined_addr.core(), [combined_addr, s_value, &ce] {
              // for this simple StringMetric, merging is as simple as accumulating the value
              StringMetric* combined_ptr = combined_addr.pointer();
              if (combined_ptr->initf_ != NULL) {
                // max length str
                if (std::string(combined_ptr->value_).length() < std::string(s_value).length()) {
                  combined_ptr->write_value(std::string(s_value));
                }
              } else {
                //append
                DVLOG(4) << "accum " << std::string(combined_ptr->value_) << " + " << std::string(s_value) << " = " << std::string(combined_ptr->value_) + std::string(s_value);
                combined_ptr->write_value(std::string(combined_ptr->value_) + std::string(s_value));
              }
              
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
