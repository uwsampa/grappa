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

#include "SharedMessagePool.hpp"
#include "Metrics.hpp"
#include "ConditionVariable.hpp"
#include "common.hpp"
#include "ChunkAllocator.hpp"
#include <stack>

DEFINE_int64(shared_pool_size, 1L << 18, "Size (in bytes) of global SharedMessagePool (on each Core)");

/// Note: max is enforced only for blockable callers, and is enforced early to avoid callers
/// that cannot block (callers from message handlers) to have a greater chance of proceeding.
/// In 'init_shared_pool()', 'emergency_alloc' margin is set, currently, to 1/4 of the max.
///
/// Currently setting this max to cap shared_pool usage at ~1GB, though this may be too much
/// for some apps & machines, as it is allocated out of the locale-shared heap. It's also
/// worth noting that under current settings, most apps don't reach this point at steady state.
DEFINE_int64(shared_pool_max, -1, "Maximum number of shared pools allowed to be allocated (per core).");
DEFINE_double(shared_pool_memory_fraction, 0.5, "Fraction of remaining memory to use for shared pool (after taking out global_heap and stacks)");

GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, shared_message_pools_allocated, 0);


#define MESSAGE_POOL_ALLOCATION_COUNT (4 * KILO)

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_1cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_2cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_3cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_ncl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_toobig, 0 );

namespace Grappa {

namespace impl {

/// multiple of cacheline size
#define MAX_POOL_MESSAGE_SIZE (1024)

/// max size to preallocate; higher sizes will allocate as needed
#define MAX_POOL_MESSAGE_SIZE_PREALLOCATED_CUTOFF (256)

/// number of cachelines in largest pool message
#define MAX_POOL_CACHELINE_COUNT (MAX_POOL_MESSAGE_SIZE / CACHE_LINE_SIZE)

/// storage for message pools
struct aligned_pool_allocator message_pool[ MAX_POOL_CACHELINE_COUNT ];

/// set up shared pool for basic message sizes
void init_shared_pool() {
  for( int i = 0; i < MAX_POOL_CACHELINE_COUNT; ++i ) {
    aligned_pool_allocator_init( &message_pool[i],
                                 CACHE_LINE_SIZE,     // cacheline alignment
                                 i * CACHE_LINE_SIZE, // size of messages in this bucket
                                 ( i < (MAX_POOL_MESSAGE_SIZE_PREALLOCATED_CUTOFF / CACHE_LINE_SIZE)
                                   ? MESSAGE_POOL_ALLOCATION_COUNT
                                   : 1 ) );
  }
}


void* _shared_pool_alloc(size_t sz) {
  size_t cacheline_count = sz / CACHE_LINE_SIZE;
  CHECK_EQ( cacheline_count * CACHE_LINE_SIZE, sz ) << "Message size not a multiple of cacheline size?";
  
  if( sz > MAX_POOL_MESSAGE_SIZE ) {
    shared_pool_alloc_toobig++;
    return locale_alloc_aligned<char>(CACHE_LINE_SIZE, sz);
  } else {
    switch( cacheline_count ) {
    case 1: shared_pool_alloc_1cl++; break;
    case 2: shared_pool_alloc_2cl++; break;
    case 3: shared_pool_alloc_3cl++; break;
    default: shared_pool_alloc_ncl++; break;
    }
    return aligned_pool_allocator_alloc( &message_pool[cacheline_count] );
  }
}

void _shared_pool_free(MessageBase * m, size_t sz) {
  size_t cacheline_count = sz / CACHE_LINE_SIZE;
  
  if( sz > MAX_POOL_MESSAGE_SIZE ) {
    return locale_free(m);
  } else {
    return aligned_pool_allocator_free( &message_pool[cacheline_count], m );
  }
}

}

} // namespace Grappa
