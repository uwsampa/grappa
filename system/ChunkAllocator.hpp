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
