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

#include "IncoherentAcquirer.hpp"
#include "common.hpp"
#include "Metrics.hpp"
#include <limits>
  


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, acquire_ams, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, acquire_ams_bytes, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, acquire_blocked, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, acquire_blocked_ticks_total, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, acquire_network_ticks_total, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<uint64_t>, acquire_wakeup_ticks_total, 0);

    
void IAMetrics::count_acquire_ams( uint64_t bytes ) {
  acquire_ams++;
  acquire_ams_bytes+=bytes;
}

void IAMetrics::record_wakeup_latency( int64_t start_time, int64_t network_time ) { 
  acquire_blocked++; 
  int64_t current_time = Grappa::timestamp();
  int64_t blocked_latency = current_time - start_time;
  int64_t wakeup_latency = current_time - network_time;
  acquire_blocked_ticks_total += blocked_latency;
  acquire_wakeup_ticks_total += wakeup_latency;
}

void IAMetrics::record_network_latency( int64_t start_time ) { 
  int64_t current_time = Grappa::timestamp();
  int64_t latency = current_time - start_time;
  acquire_network_ticks_total += latency;
}

