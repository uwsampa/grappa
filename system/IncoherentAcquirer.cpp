
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "IncoherentAcquirer.hpp"
#include "common.hpp"
#include "Statistics.hpp"
#include <limits>
  


GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, acquire_ams, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, acquire_ams_bytes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, acquire_blocked, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, acquire_blocked_ticks_total, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, acquire_network_ticks_total, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, acquire_wakeup_ticks_total, 0);

    
void IAStatistics::count_acquire_ams( uint64_t bytes ) {
  acquire_ams++;
  acquire_ams_bytes+=bytes;
}

void IAStatistics::record_wakeup_latency( int64_t start_time, int64_t network_time ) { 
  acquire_blocked++; 
  int64_t current_time = Grappa::timestamp();
  int64_t blocked_latency = current_time - start_time;
  int64_t wakeup_latency = current_time - network_time;
  acquire_blocked_ticks_total += blocked_latency;
  acquire_wakeup_ticks_total += wakeup_latency;
}

void IAStatistics::record_network_latency( int64_t start_time ) { 
  int64_t current_time = Grappa::timestamp();
  int64_t latency = current_time - start_time;
  acquire_network_ticks_total += latency;
}

