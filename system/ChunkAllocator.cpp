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

#include "ChunkAllocator.hpp"
#include "LocaleSharedMemory.hpp"

#include "Metrics.hpp"

GRAPPA_DEFINE_METRIC(SimpleMetric<int64_t>, chunkallocator_append, 0 );


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
    
    new_chunk->next = NULL;
    new_chunk->chunk_size = MAX(min_size, aa->chunk_size) + aa->align_on;
    new_chunk->chunk = Grappa::locale_alloc_aligned<char>(CACHE_LINE_SIZE, new_chunk->chunk_size);
    CHECK_NOTNULL(new_chunk->chunk);
    
    // for timing only
    memset(new_chunk->chunk, new_chunk->chunk_size, 0);
    
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
    object_size = MAX(object_size, sizeof(void *));
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

    if (!apa->firsts[apa->first_index]) {
        int index = 0;
        while (index < ALLOCATOR_PREFETCH_DISTANCE)
            if (apa->firsts[index])
                break;
            else
                ++index;
        if (index < ALLOCATOR_PREFETCH_DISTANCE)
            apa->first_index = index;
        else {
            ++apa->newly_allocated_objects;
            return aligned_allocator_alloc(apa->aa, apa->object_size);
        }
    }
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
