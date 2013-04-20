
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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

DEFINE_bool( global_memory_use_hugepages, USE_HUGEPAGES_DEFAULT, "UNUSED: use 1GB huge pages for global heap" );
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
  DVLOG(2) << "Core " << Grappa::cores << " allocating " << size_ << " bytes ";
  memory_ = Grappa::impl::locale_shared_memory.allocate_aligned( size_, 64 );
  CHECK_NOTNULL( memory_ );
  Grappa::impl::global_memory_chunk_base = memory_;
  DVLOG(2) << "Core " << Grappa::cores << " allocated " << size_ << " bytes ";
}
