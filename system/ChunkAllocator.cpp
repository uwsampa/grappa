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

#include "ChunkAllocator.hpp"
#include "LocaleSharedMemory.hpp"
#include "TaskingScheduler.hpp"
#include "Communicator.hpp"

#include "Metrics.hpp"

DECLARE_int64( shared_pool_max_size );
DECLARE_double( shared_pool_memory_fraction );

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, chunkallocator_append, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, chunkallocator_allocated, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, chunkallocator_yielded, 0 );

static int64_t shared_pool_total_allocated = 0;
static bool shared_pool_max_exceeded = false;

namespace Grappa {
namespace impl {




struct aligned_allocator *aligned_allocator_create(void) {
  return Grappa::locale_alloc_aligned<struct aligned_allocator>(CACHE_LINE_SIZE);
}

void aligned_allocator_destroy(struct aligned_allocator *aa) {
    aligned_allocator_clean(aa);
    Grappa::locale_free(aa);
}

void aligned_allocator_init(struct aligned_allocator *aa,
                            size_t align_on,
                            u_int64_t chunk_size) {
    aa->align_on = align_on;
    aa->chunk_size = chunk_size;
    aa->first = aa->last = NULL;                       
}

void aligned_allocator_clean(struct aligned_allocator *aa) {
    struct memory_chunk *walk, *next;
    
    walk = aa->first;
    while(walk) {
        free(walk->chunk);
        next = walk->next;
        free(walk);
        walk = next;
    }
    aa->first = aa->last = NULL;
}

static void _aligned_allocator_append_chunk(struct aligned_allocator *aa,
    size_t min_size) {
    struct memory_chunk *new_chunk;

    chunkallocator_append++;
    
    new_chunk = Grappa::locale_alloc_aligned<struct memory_chunk>(CACHE_LINE_SIZE);
    CHECK_NOTNULL(new_chunk);

    auto chunk_struct_size = sizeof( struct memory_chunk );
    shared_pool_total_allocated += std::max( chunk_struct_size, (decltype(chunk_struct_size)) CACHE_LINE_SIZE );
    chunkallocator_allocated += std::max( chunk_struct_size, (decltype(chunk_struct_size)) CACHE_LINE_SIZE );
    
    new_chunk->next = NULL;
    new_chunk->chunk_size = std::max(min_size, aa->chunk_size) + aa->align_on;
    new_chunk->chunk = Grappa::locale_alloc_aligned<char>(CACHE_LINE_SIZE, new_chunk->chunk_size);
    CHECK_NOTNULL(new_chunk->chunk);

    auto chunk_size = sizeof(new_chunk->chunk_size);
    shared_pool_total_allocated += std::max( new_chunk->chunk_size, (decltype(chunk_size)) CACHE_LINE_SIZE );
    
    if (!aa->first)
        aa->last = aa->first = new_chunk;
    else {
        aa->last->next = new_chunk;
        aa->last = new_chunk;
        aa->last->next = NULL;
    }
    aa->last->offset = (size_t)((u_int64_t)ALIGN_UP(new_chunk->chunk, aa->align_on) -
                                (u_int64_t)new_chunk->chunk);    
}

void *aligned_allocator_alloc(struct aligned_allocator *aa, size_t size) {
    size = (size_t)ALIGN_UP(size, aa->align_on);
    void *p;

    if (!aa->last)
        _aligned_allocator_append_chunk(aa, size);
    if ((aa->last->chunk_size - aa->last->offset) < size)
        _aligned_allocator_append_chunk(aa, size);
    
    p = &aa->last->chunk[aa->last->offset];
    aa->last->offset += size;
    
    return p;
}

void aligned_allocator_free(struct aligned_allocator *aa, void *obj) {
    // do nothing
}

struct aligned_pool_allocator   *aligned_pool_allocator_create(void) {
  auto allocator_size = sizeof(struct aligned_pool_allocator);
  shared_pool_total_allocated += std::max( allocator_size, (decltype(allocator_size)) CACHE_LINE_SIZE );
  return locale_alloc_aligned<struct aligned_pool_allocator>(CACHE_LINE_SIZE);
}

void aligned_pool_allocator_destroy(struct aligned_pool_allocator *apa) {
    aligned_allocator_destroy(apa->aa);
    free(apa);
}

void aligned_pool_allocator_init(struct aligned_pool_allocator *apa,
                                 size_t align_on,
                                 size_t object_size,
                                 u_int64_t chunk_count) {
    int i;
    object_size = std::max(object_size, sizeof(void *));
    apa->aa = aligned_allocator_create();
    aligned_allocator_init(apa->aa, align_on, object_size * chunk_count);
    for (i = 0; i < ALLOCATOR_PREFETCH_DISTANCE; i++)
        apa->firsts[i] = NULL;
    apa->object_size = object_size;
    apa->first_index = 0;
    apa->allocated_objects = 0;
    apa->newly_allocated_objects = 0;                     
}

void aligned_pool_allocator_clean(struct aligned_pool_allocator *apa) {
    // not really meaningful
}

void *aligned_pool_allocator_alloc(struct aligned_pool_allocator *apa) {
    struct link_object *ret;
    prefetchnta(apa);
    prefetchnta(&apa->firsts[0]);
    bool yielded = false;

    do {
      // try allocating from the default list
      ret = apa->firsts[apa->first_index];
      
      // if that didn't work, search for a list with something available
      if( !ret ) {
        int index = 0;
        
        while (index < ALLOCATOR_PREFETCH_DISTANCE) {
          if (apa->firsts[index]) {
            break;
          } else {
            ++index;
          }
        }
        
        if (index < ALLOCATOR_PREFETCH_DISTANCE) {
          apa->first_index = index;
          ret = apa->firsts[apa->first_index];
        }
      }
      
      // if that didn't work, either make a new one or block
      if( !ret ) {
        if( (shared_pool_total_allocated < FLAGS_shared_pool_max_size) ||
            Grappa::impl::global_scheduler.in_no_switch_region() ) {

          ++apa->newly_allocated_objects;
          ret = (struct link_object*) aligned_allocator_alloc(apa->aa, apa->object_size);

          if( (shared_pool_total_allocated > FLAGS_shared_pool_max_size) &&
              (shared_pool_max_exceeded == false) ) {
            shared_pool_max_exceeded = true;
            LOG(WARNING) << "Shared pool size " << shared_pool_total_allocated
                         << " exceeded max size " << FLAGS_shared_pool_max_size;
          }

          return ret;
          
        } else {
          
          if( yielded == false ) {
            chunkallocator_yielded++;
            yielded = true;
          }

          global_communicator.poll();
          Grappa::yield();
        
        }
      }

    } while( !ret );

//    while( (!aa->last) ||
//            ((aa->last->chunk_size - aa->last->offset) < size) ) {
//       if(  (shared_pool_total_allocated < FLAGS_shared_pool_max_size) ||
//            Grappa::impl::global_scheduler.in_no_switch_region() ) {
//         _aligned_allocator_append_chunk(aa, size);
//       } else {
//         if( yielded == false ) {
//           chunkallocator_yielded++;
//           yielded = true;
//         }
//         global_communicator.poll();
//         Grappa::yield();
//       }
//     }

    prefetchnta(apa->firsts[apa->first_index]);

    ++apa->allocated_objects;
    ret = apa->firsts[apa->first_index];
    apa->firsts[apa->first_index] = ret->next;
    if (apa->firsts[apa->first_index])
        prefetcht0(apa->firsts[apa->first_index]);
    apa->first_index = (apa->first_index + 1) % ALLOCATOR_PREFETCH_DISTANCE;
    ret->next = NULL;
    return (void*)ret;
}

void aligned_pool_allocator_free(struct aligned_pool_allocator *apa, void *p) {
    struct link_object *next = (struct link_object *) p;
    
    --apa->allocated_objects;
    next->next = apa->firsts[apa->first_index];
    apa->firsts[apa->first_index] = next;
    apa->first_index = (apa->first_index + 1) % ALLOCATOR_PREFETCH_DISTANCE;
}

int64_t aligned_pool_allocator_freelist_size(struct aligned_pool_allocator *apa) {
    return -1 * apa->allocated_objects;
}

} // namespace impl
} // namespace Grappa
