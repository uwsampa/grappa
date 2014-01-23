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
