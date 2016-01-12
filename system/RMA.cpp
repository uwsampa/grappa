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

#include <SymmetricAllocator.hpp>
#include <RMA.hpp>

namespace Grappa {
namespace impl {

RMA global_rma;

/// Call before using RMA operations
void RMA::init() {
  create_dynamic_window();
  auto p = allocate( FLAGS_initial_symmetric_heap_size );
  free( p );
}

/// Call during shutdown, after using RMA operations
void RMA::finish() {
  teardown_dynamic_window();
}

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
  return Grappa::spmd::blocking::symmetric_alloc<char>( size );
}

/// collective call to free symmetric region
void RMA::free( void * base ) {
  Grappa::spmd::blocking::symmetric_free( base );
}

} // namespace impl
} // namespace Grappa

