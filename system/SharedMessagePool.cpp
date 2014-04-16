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

DEFINE_int64( shared_pool_chunk_size, 1 * MEGA, "Number of bytes to allocate when shared message pool is empty" );


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, shared_message_pools_allocated, 0);

/// record counts of messages allocated by cache line count
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_1cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_2cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_3cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_ncl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_toobig, 0 );

namespace Grappa {

namespace impl {

/// messages larger than this size will be malloced directly.
/// this should be a multiple of the cacheline size.
#define MAX_POOL_MESSAGE_SIZE (1 << 10)

/// We will preallocate pools of messages up to this size (in cachelines).
/// Higher sizes will allocate as needed.
#define MAX_POOL_MESSAGE_SIZE_PREALLOCATED_CUTOFF (1 << 8)

/// number of cachelines in largest pool message
#define MAX_POOL_CACHELINE_COUNT (MAX_POOL_MESSAGE_SIZE / CACHE_LINE_SIZE)

/// storage for message pools
struct aligned_pool_allocator message_pool[ MAX_POOL_CACHELINE_COUNT ];

/// set up shared pool for basic message sizes
void init_shared_pool() {
  for( int i = 0; i < MAX_POOL_CACHELINE_COUNT; ++i ) {
    auto message_size = (i+1) * CACHE_LINE_SIZE;
    auto chunk_count = FLAGS_shared_pool_chunk_size / message_size;

    // only make big chunks for frequently-used message sizes
    if( i >= (MAX_POOL_MESSAGE_SIZE_PREALLOCATED_CUTOFF / CACHE_LINE_SIZE) ) {
      chunk_count = 1;
    }

    aligned_pool_allocator_init( &message_pool[i],
                                 CACHE_LINE_SIZE, // alignment
                                 message_size,
                                 chunk_count );
  }
}


void* _shared_pool_alloc(size_t sz) {
  size_t cacheline_count = sz / CACHE_LINE_SIZE;
  CHECK_EQ( cacheline_count * CACHE_LINE_SIZE, sz ) << "Message size not a multiple of cacheline size?";
  
  if( sz > MAX_POOL_MESSAGE_SIZE ) {
    shared_pool_alloc_toobig++;
    return locale_alloc_aligned<char>(CACHE_LINE_SIZE, sz);
  } else {
    // record the pool allocation (bucketed by number of cachelines)
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
