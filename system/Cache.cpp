
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// Main Explicit Cache API

#include "Cache.hpp"
#include "Statistics.hpp"


GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, ro_acquires, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, wo_releases, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, rw_acquires, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, rw_releases, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, bytes_acquired, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, bytes_released, 0);

void CacheStatistics::count_ro_acquire( uint64_t bytes ) { 
  ro_acquires++;
  bytes_acquired+=bytes;
}
void CacheStatistics::count_wo_release( uint64_t bytes ) { 
  wo_releases++; 
  bytes_released+=bytes;
}
void CacheStatistics::count_rw_acquire( uint64_t bytes) { 
  rw_acquires++;
  bytes_acquired+=bytes;
}
void CacheStatistics::count_rw_release( uint64_t bytes ) { 
  rw_releases++; 
  bytes_released+=bytes;
}

/// TODO: delete me.
Core address2node( void * ) {
  return 0;
}
