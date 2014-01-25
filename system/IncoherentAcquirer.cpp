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

