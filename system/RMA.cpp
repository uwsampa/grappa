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

#include <sys/mman.h>

#include <RMA.hpp>

namespace Grappa {
namespace impl {

RMA global_rma;

/// collective call to create dynamic window
void RMA::create_dynamic_window() {
  MPI_Info info;
  MPI_CHECK( MPI_Info_create( &info ) );

  // Assure MPI that all locale shared memory segments have identical sizes
  MPI_CHECK( MPI_Info_set( info, "same_size", "true" ) );
  // Assure MPI that all locale shared memory segments have identical displacement units
  MPI_CHECK( MPI_Info_set( info, "same_disp_unit", "true" ) );

  MPI_CHECK( MPI_Win_create_dynamic( info,
                                     global_communicator.grappa_comm,
                                     &dynamic_window_ ) );

  // Clean up info struct now that we're done with it
  MPI_CHECK( MPI_Info_free( &info ) );

  // check info
    
  // Verify we got what we expected
  int * attr;
  int attr_flag;
  MPI_CHECK( MPI_Win_get_attr( dynamic_window_, MPI_WIN_MODEL, &attr, &attr_flag ) );
  CHECK( attr_flag ) << "Error getting attributes for MPI window";
  CHECK_EQ( *attr, MPI_WIN_UNIFIED ) << "MPI window doesn't use MPI_WIN_UNIFIED model.";

  // "acquire locks" for all other regions to allow async puts and gets
  MPI_CHECK( MPI_Win_lock_all( MPI_MODE_NOCHECK, dynamic_window_ ) );
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );
}

/// collective call to free dynamic window
void RMA::teardown_dynamic_window() {
  if( MPI_WIN_NULL != dynamic_window_ ) {
    MPI_CHECK( MPI_Win_unlock_all( dynamic_window_ ) );
    MPI_CHECK( MPI_Win_free( &dynamic_window_ ) );
  }
}

/// collective call to allocate symmetric region for passive one-sided ops
void * RMA::allocate( size_t size ) {
  // TODO: this is all a hack; replace with a proper allocator
  static size_t alloc_count = 0;
  static char * alloc_base = reinterpret_cast<char*>( 0x0000200000000000ULL );
    
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );
  void * base = mmap( alloc_base, size,
                      PROT_READ | PROT_WRITE,
                      // TODO: consider MAP_FIXED or MAP_HUGETLB here );
                      MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED,
                      -1, 0 );

  DVLOG(2) << "Allocating " << size << " bytes at " << base;

  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  if( MAP_FAILED == base ) {
    perror( "Failed to allocate memory at requested address." );
    exit(1);
  }

  { // check that allocations happened at same address
    size_t counts[ cores() ];
    void * bases[ cores() ];

    MPI_CHECK( MPI_Allgather( &alloc_count, sizeof(size_t), MPI_BYTE,
                              &counts[0], sizeof(size_t), MPI_BYTE,
                              global_communicator.grappa_comm ) );
    MPI_CHECK( MPI_Allgather( &base, sizeof(base), MPI_BYTE,
                              &bases[0], sizeof(base), MPI_BYTE,
                              global_communicator.grappa_comm ) );

    for( int i = 0; i < cores(); ++i ) {
      CHECK_EQ( alloc_count, counts[i] ) << "Allocation sequence number didn't match!";
      CHECK_EQ( base, bases[i] ) << "Allocation didn't happen at same address!";
    }
  }
    
  MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

  // save size for deallocation
  alloc_sizes_[ base ] = size;

  // increment next allocation pointer
  alloc_count++;
  {
    const size_t page_size = 1 << 12;
    size_t increment = size;
      
    if( increment < page_size ) {
      increment = page_size;
    } else {
      size_t diff = increment % page_size;
      if( 0 != diff ) {
        increment = size + page_size - diff;
      }
    }
    alloc_base += increment;
  }

  register_region( base, size );
    
  return base;
}

/// collective call to free symmetric region
void RMA::free( void * base ) {
  deregister_region( base );
  auto it = alloc_sizes_.find( base );
  CHECK( it != alloc_sizes_.end() ) << "Couldn't find symmetric region to free at " << base;
  DVLOG(2) << "Freeing " << it->second << " bytes at " << base;
  auto result = munmap( base, it->second );
  alloc_sizes_.erase( it );
  if( result < 0 ) {
    perror( "Failed to deallocate memory at requested address." );
    exit(1);
  }
}

} // namespace impl
} // namespace Grappa

