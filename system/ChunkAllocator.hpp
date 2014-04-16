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
#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>

namespace Grappa {
namespace impl {

#define ALLOCATOR_PREFETCH_DISTANCE 8
struct memory_chunk {
    struct memory_chunk *next;
    char    *chunk;
    size_t  chunk_size;
    size_t  offset;
};
struct aligned_allocator {
    size_t  align_on;
    size_t  chunk_size;
    struct  memory_chunk    *first;
    struct  memory_chunk    *last;
};

struct link_object {
    struct link_object *next;
};

struct aligned_pool_allocator {
    struct aligned_allocator *aa;
    size_t object_size;
    int    first_index;
    struct link_object *firsts[ALLOCATOR_PREFETCH_DISTANCE];
    int64_t   allocated_objects;
    int64_t   newly_allocated_objects;
};

struct aligned_allocator *aligned_allocator_create(void);
void aligned_allocator_destroy(struct aligned_allocator *aa);
void aligned_allocator_init(struct aligned_allocator *aa,
                            size_t align_on,
                            u_int64_t chunk_size);

void aligned_allocator_clean(struct aligned_allocator *aa);
void *aligned_allocator_alloc(struct aligned_allocator *aa, size_t size);
void aligned_allocator_free(struct aligned_allocator *aa, void *obj);
struct aligned_pool_allocator *aligned_pool_allocator_create(void);
void aligned_pool_allocator_destroy(struct aligned_pool_allocator *apa);
void aligned_pool_allocator_init(struct aligned_pool_allocator *apa,
                                 size_t align_on,
                                 size_t object_size,
                                 u_int64_t chunk_count);
void aligned_pool_allocator_clean(struct aligned_pool_allocator *apa);
void *aligned_pool_allocator_alloc(struct aligned_pool_allocator *apa);
void aligned_pool_allocator_free(struct aligned_pool_allocator *apa, void *p);

int64_t aligned_pool_allocator_freelist_size(struct aligned_pool_allocator *apa);

class ChunkAllocator {
private:

public:
  ChunkAllocator();

};

} // namespace impl
} // namespace Grappa
