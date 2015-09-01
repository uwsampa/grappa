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

#include "SharedMessagePool.hpp"
#include "Metrics.hpp"
#include "ConditionVariable.hpp"
#include "common.hpp"
#include "ChunkAllocator.hpp"
#include <stack>

DEFINE_int64( shared_pool_chunk_size, 1 << 13, "Number of bytes to allocate when shared message pool is empty" );

DECLARE_int64( locale_shared_size );

DEFINE_int64(shared_pool_max_size, 0, "Soft maximum size (in bytes) of shared message pool storage (on each Core) (0 sets automatically base on memory fraction)");
DEFINE_double(shared_pool_memory_fraction, 0.25, "Fraction of locale shared heap to use for shared pool");


GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, shared_message_pools_allocated, 0);

/// record counts of messages allocated by cache line count
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_1cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_2cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_3cl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_ncl, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, shared_pool_alloc_toobig, 0 );

namespace Grappa {

/// messages larger than this size will be malloced directly.
/// this should be a multiple of the cacheline size.
#define MAX_POOL_MESSAGE_SIZE (1 << 10)

/// We will preallocate pools of messages up to this size (in cachelines).
/// Higher sizes will allocate as needed.
#define MAX_POOL_MESSAGE_SIZE_PREALLOCATED_CUTOFF (1 << 8)

/// number of cachelines in largest pool message
#define MAX_POOL_CACHELINE_COUNT (MAX_POOL_MESSAGE_SIZE / CACHE_LINE_SIZE)

namespace SharedMessagePool {
  
/// storage for message pools
struct aligned_pool_allocator message_pool[ MAX_POOL_CACHELINE_COUNT ];

size_t estimate_footprint() {
  return FLAGS_shared_pool_max_size;
}

size_t adjust_footprint(size_t target) {
  if (estimate_footprint() > target) {
    while (estimate_footprint() > target) FLAGS_shared_pool_max_size /= 2;
    MASTER_ONLY VLOG(2) << "\nAdjusted:" << "- shared_message_pool: " << estimate_footprint();
  }
  return estimate_footprint();
}
  
/// set up shared pool for basic message sizes
void init() {
  if( 0 == FLAGS_shared_pool_max_size ) {
    double size = FLAGS_locale_shared_size;
    size /= Grappa::locale_cores();
    size *= FLAGS_shared_pool_memory_fraction;
    FLAGS_shared_pool_max_size = size;
  }

}

void activate() {
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

void* alloc(size_t sz) {
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

void free(MessageBase * m, size_t sz) {
  size_t cacheline_count = sz / CACHE_LINE_SIZE;
  
  if( sz > MAX_POOL_MESSAGE_SIZE ) {
    return locale_free(m);
  } else {
    return aligned_pool_allocator_free( &message_pool[cacheline_count], m );
  }
}

} // namespace SharedMessagePool

} // namespace Grappa
