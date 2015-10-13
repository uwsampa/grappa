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

#include "SummarizingMetric.hpp"
#include "Communicator.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"

namespace Grappa {
  template< typename T >
  void SummarizingMetric<T>::merge_all(impl::MetricBase* static_stat_ptr) {
    this->value_ = 0;
    this->n = 0;
    this->mean = 0.0;
    this->M2 = 0.0;
    this->max = 0;
    this->min = 0;
    
    // TODO: use more generalized `reduce` operation to merge all
    SummarizingMetric<T>* this_static = reinterpret_cast<SummarizingMetric<T>*>(static_stat_ptr);
    
    GlobalAddress<SummarizingMetric<T>> combined_addr = make_global(this);
    
    CompletionEvent ce(Grappa::cores());
    
    for (Core c = 0; c < Grappa::cores(); c++) {
      // we can compute the GlobalAddress here because we have pointers to globals,
      // which are guaranteed to be the same on all nodes
      GlobalAddress<SummarizingMetric<T>> remote_stat = make_global(this_static, c);
      
      send_heap_message(c, [remote_stat, combined_addr, &ce] {
          SummarizingMetric<T>* s = remote_stat.pointer();
          struct {
            T value_, min, max; size_t n; double mean, M2;
          } sp = {
            s->value_, s->min, s->max, s->n, s->mean, s->M2
          };
          
          send_heap_message(combined_addr.core(), [combined_addr, sp, &ce] {
              SummarizingMetric<T>* combined_ptr = combined_addr.pointer();
              if( sp.n > 0 ) { // update only if we have accumulated samples
                combined_ptr->value_ += sp.value_;
                if( 0 == combined_ptr->n ) { // overwrite if we are the first samples
                  combined_ptr->n = sp.n;
                  combined_ptr->mean = sp.mean;
                  combined_ptr->M2 = sp.M2;
                  combined_ptr->min = sp.min;
                  combined_ptr->max = sp.max;
                } else {
                  size_t new_n = sp.n + combined_ptr->n;
                  double delta = sp.mean - combined_ptr->mean;
                  double new_mean = combined_ptr->mean + delta * ( static_cast<double>(sp.n) / new_n );
                  double new_M2 = combined_ptr->M2 + sp.M2 + 
                    delta * delta * combined_ptr->n * sp.n / new_n;
                  
                  combined_ptr->n = new_n;
                  combined_ptr->mean = new_mean;
                  combined_ptr->M2 = new_M2;
                  combined_ptr->max = std::max(combined_ptr->max, sp.max);
                  combined_ptr->min = std::min(combined_ptr->min, sp.min);
                }
              }
              ce.complete();
            });
        });
    }
    ce.wait();
  }
}
