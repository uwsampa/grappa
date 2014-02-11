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

/// Main Explicit Cache API

#include "Cache.hpp"
#include "Metrics.hpp"


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, ro_acquires, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, wo_releases, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, rw_acquires, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, rw_releases, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, bytes_acquired, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, bytes_released, 0);

void CacheMetrics::count_ro_acquire( uint64_t bytes ) { 
  ro_acquires++;
  bytes_acquired+=bytes;
}
void CacheMetrics::count_wo_release( uint64_t bytes ) { 
  wo_releases++; 
  bytes_released+=bytes;
}
void CacheMetrics::count_rw_acquire( uint64_t bytes) { 
  rw_acquires++;
  bytes_acquired+=bytes;
}
void CacheMetrics::count_rw_release( uint64_t bytes ) { 
  rw_releases++; 
  bytes_released+=bytes;
}

/// TODO: delete me.
Core address2node( void * ) {
  return 0;
}
