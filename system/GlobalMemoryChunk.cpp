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

#include <glog/logging.h>
#include <gflags/gflags.h>

extern "C" {
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
}
#ifndef SHM_HUGETLB
#define SHM_HUGETLB 0
#define USE_HUGEPAGES_DEFAULT false
#else
#ifndef USE_HUGEPAGES_DEFAULT
#define USE_HUGEPAGES_DEFAULT true
#endif
#endif

#include "Communicator.hpp"

#include "GlobalMemoryChunk.hpp"
#include "LocaleSharedMemory.hpp"

DEFINE_bool( global_memory_use_hugepages, false, "UNUSED: use 1GB huge pages for global heap" );
DEFINE_int64( global_memory_per_node_base_address, 0x0000123400000000L, "UNUSED: global memory base address");


namespace Grappa {
namespace impl {
void * global_memory_chunk_base = NULL;
}
}

/// Tear down GlobalMemoryChunk, removing shm region if possible
GlobalMemoryChunk::~GlobalMemoryChunk() {
  Grappa::impl::locale_shared_memory.deallocate( memory_ );
}

/// Construct GlobalMemoryChunk.
GlobalMemoryChunk::GlobalMemoryChunk( size_t size )
  : size_( size )
  , memory_( 0 )
{
  DVLOG(2) << "Core " << Grappa::mycore() << " allocating " << size_ << " bytes ";
  memory_ = Grappa::impl::locale_shared_memory.allocate_aligned( size_, 64 );
  CHECK_NOTNULL( memory_ );
  Grappa::impl::global_memory_chunk_base = memory_;
  DVLOG(2) << "Core " << Grappa::mycore() << " allocated " << size_ << " bytes ";
}
