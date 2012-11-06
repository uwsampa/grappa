
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

DEFINE_bool( global_memory_use_hugepages, USE_HUGEPAGES_DEFAULT, "use 1GB huge pages for global heap" );
DEFINE_int64( global_memory_per_node_base_address, 0x0000123400000000L, "global memory base address");

/// Round up to gigabyte page size
static const size_t round_to_gb_huge_page( size_t size ) {
  const size_t half_gb_huge_page = (1L << 30) - 1;
  // make sure we use at least one gb huge page
  size += half_gb_huge_page;
  // now page-align the address (30 bits)
  size &= 0xffffffffc0000000L;
  return size;
}

/// Round up to 4KB page size
static const size_t round_to_4kb_page( size_t size ) {
  const size_t half_4kb_page = (1L << 12) - 1;
  // make sure we use at least one page
  size += half_4kb_page;
  // now page-align the address (12 bits)
  size &= 0xfffffffffffff000L;
  return size;
}

/// Tear down GlobalMemoryChunk, removing shm region if possible
GlobalMemoryChunk::~GlobalMemoryChunk() {
  PCHECK( -1 != shmdt( memory_ ) ) << "GlobalMemoryChunk destructor failed to detach from shared memory region";
  PCHECK( -1 != shmctl(shm_id_, IPC_RMID, NULL ) ) << "GlobalMemoryChunk destructor failed to deallocate shared memory region id";
}

/// Construct GlobalMemoryChunk.
GlobalMemoryChunk::GlobalMemoryChunk( size_t size )
  : shm_key_( -1 )
  , shm_id_( -1 )
  , size_( FLAGS_global_memory_use_hugepages ? 
           round_to_gb_huge_page( size ) : 
           round_to_4kb_page( size ) )
  , base_( reinterpret_cast< void * >( FLAGS_global_memory_per_node_base_address ) )
  , memory_( 0 )
{
  // build shm key from job id and node id
  char * job_id_str = getenv("SLURM_JOB_ID");
  int job_id = -1;
  if( job_id_str != NULL ) {
    job_id = atoi( job_id_str );
  } else {
    job_id = getpid();
  }
  shm_key_ = job_id * global_communicator.nodes() + global_communicator.mynode();
  DVLOG(2) << size << " rounded to " << size_;
  // get shared memory region id
  VLOG(1) << "shm_size_: " << size_ << ", use_hugepages? " << FLAGS_global_memory_use_hugepages;

  shm_id_ = shmget( shm_key_, size_, IPC_CREAT | SHM_R | SHM_W | 
                    (FLAGS_global_memory_use_hugepages ? SHM_HUGETLB : 0) );
  PCHECK( shm_id_ != -1 ) << "Failed to get shared memory region for shared heap of size " << size_;
  
  // map memory
  memory_ = shmat( shm_id_, base_, 0 );
  PCHECK( memory_ != (void*)-1 ) << "GlobalMemoryChunk allocation didn't happen at specified base address";
}
