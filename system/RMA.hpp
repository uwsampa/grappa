#pragma once
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

#include <unordered_map>
#include <limits>
#include <functional>
#include <algorithm>
#include <utility>

#include <Communicator.hpp>
#include <CollectiveOps.hpp>

namespace Grappa {
namespace impl {

class RMA;

class RMARequest {
private:
  MPI_Request request_;

public:
  RMARequest(): request_( MPI_REQUEST_NULL ) {}
  RMARequest( MPI_Request request): request_( request ) {}

  void reset() {
    // MPI_Wait just returns if called with MPI_REQUEST_NULL.
    // TODO: this may need to yield when used inside Grappa::run()
    MPI_CHECK( MPI_Wait( &request_, MPI_STATUS_IGNORE ) );
  }

  /// Warning: may leak request if someone has not completed it elsewhere.
  void reset_nowait() {
    request_ = MPI_REQUEST_NULL;
  }
  
  void wait() {
    // TODO: this may need to yield when used inside Grappa::run()
    MPI_CHECK( MPI_Wait( &request_, MPI_STATUS_IGNORE ) );
  }

  bool test() {
    int flag;
    MPI_CHECK( MPI_Test( &request_, &flag, MPI_STATUS_IGNORE ) );
    return flag;
  }
};  

class RMAWindow {
private:
  friend class RMA;
  
  void * base_;
  size_t size_;
  MPI_Win window_;

public:
  RMAWindow()
    : base_( nullptr )
    , size_( 0 )
    , window_( MPI_WIN_NULL )
  { }
  
  RMAWindow( void * base, size_t size, MPI_Win win )
    : base_( base )
    , size_( size )
    , window_( win )
  { }

  void * base() const { return base_; }
  const size_t size() const { return size_; }

  friend std::ostream & operator<<( std::ostream & o, const RMAWindow & window ) {
    return o << "[RMAWindow base " << window.base_
             << " size " << window.size_ << "/" << (void*) window.size_
             << " window " << window.window_
             << "]";
  }
};

class RMA {
private:
  MPI_Win dynamic_window_;

  // collective call to create dynamic window
  void create_dynamic_window();

  // collective call to free dynamic window
  void teardown_dynamic_window();

  std::unordered_map< void *, size_t > alloc_sizes_;
  
public:
  RMA()
    : dynamic_window_( MPI_WIN_NULL)
  {
    static_assert( sizeof(MPI_Aint) >= sizeof(void*), "Sorry! Grappa requires that MPI displacements are large enough to represent an address." );
    CHECK_EQ( reinterpret_cast<ptrdiff_t>(MPI_BOTTOM), 0 )
      << "Sorry! Grappa requires that pointers and MPI dynamic window offsets are equivalent (and thus MPI_BOTTOM must be 0). Please use a different version MPI.";
  }

  void init() {
    create_dynamic_window();
  }

  void finish() {
    teardown_dynamic_window();
  }

  // non-collective call to register region for passive one-sided ops
  void register_region( void * base, size_t size ) {
    if( 0 == size ) {
      DVLOG(1) << "Registering size 0 region at " << base << " as size 1.";
      size++;
    }
    MPI_CHECK( MPI_Win_attach( dynamic_window_, base, size ) );
  }

  // non-collective call to de-register region for passive one-sided ops
  void deregister_region( void * base ) {
    int already_finalised;
    MPI_CHECK( MPI_Finalized( &already_finalised ) );
    if( !already_finalised ) {
      MPI_CHECK( MPI_Win_detach( dynamic_window_, base ) );
    }
  }
  
  // collective call to allocate symmetric region for passive one-sided ops
  void * allocate( size_t size );

  // collective call to free symmetric region
  void free( void * base );
  
  // non-collective local fence/flush operation
  void fence() {
    MPI_CHECK( MPI_Win_flush_all( dynamic_window_ ) );
  }

  //
  // One-sided API
  //
  // In these methods, the destination is on the left of the argument list, and sources are on the right.
  //
  
  // Copy bytes to remote memory location.
  void put_bytes_nbi( const Core core, void * dest, const void * source, const size_t size ) {
    // TODO: deal with >31-bit offsets and sizes
    auto dest_int = reinterpret_cast< MPI_Aint >( dest );
    CHECK_LT( dest_int, std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,     std::numeric_limits<int>::max() )      << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Put( source, size, MPI_BYTE,
                        core, dest_int, size, MPI_BYTE,
                        dynamic_window_ ) );
  }

  // Copy bytes to remote memory location. An MPI_Request pointer is
  // passed in to be used for completion detection.
  void put_bytes_nb( const Core core, void * dest, const void * source, const size_t size, MPI_Request * request_p ) {
    // TODO: deal with >31-bit offsets and sizes
    auto dest_int = reinterpret_cast< MPI_Aint >( dest );
    CHECK_LT( dest_int, std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,     std::numeric_limits<int>::max() )      << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Rput( source, size, MPI_BYTE,
                         core, dest_int, size, MPI_BYTE,
                         dynamic_window_,
                         request_p ) );
  }

  // Copy bytes to remote memory location. A RMARequest object is
  // returned for completion detection.
  RMARequest put_bytes_nb( const Core core, void * dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    return RMARequest( request );
  }

  // Copy bytes to remote memory location, blocking until the transfer is complete.
  void put_bytes( const Core core, void * dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    RMARequest( request ).wait();
  }

  // Copy bytes from remote memory location. 
  void get_bytes_nbi( void * dest, const Core core, const void * source, const size_t size ) {
    // TODO: deal with >31-bit offsets and sizes
    auto source_int = reinterpret_cast< MPI_Aint >( source );
    CHECK_LT( source_int, std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,       std::numeric_limits<int>::max() )      << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Get( dest, size, MPI_BYTE,
                        core, source_int, size, MPI_BYTE,
                        dynamic_window_ ) );
  }

  // Copy bytes from remote memory location. An MPI_Request pointer is
  // passed in to be used for completion detection.
  void get_bytes_nb( void * dest, const Core core, const void * source, const size_t size, MPI_Request * request_p ) {
    // TODO: deal with >31-bit offsets and sizes
    auto source_int = reinterpret_cast< MPI_Aint >( source );
    CHECK_LT( source_int, std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,       std::numeric_limits<int>::max() )      << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Rget( dest, size, MPI_BYTE,
                         core, source_int, size, MPI_BYTE,
                         dynamic_window_,
                         request_p ) );
  }

  // Copy bytes from remote memory location. An RMARequest object is
  // returned for completion detection.
  RMARequest get_bytes_nb( void * dest, const Core core, const void * source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    return RMARequest( request );
  }

  // Copy bytes from remote memory location, blocking until the transfer is complete.
  void get_bytes( void * dest, const Core core, const void * source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    RMARequest( request ).wait();
  }

  // Perform atomic op on remote memory location. See MPI.hpp for supported operations.
  template< typename T, typename OP >
  void atomic_op_nbi( T * result, const Core core, T * dest, OP op, const T * source ) {
    static_assert( MPI_OP_NULL != Grappa::impl::MPIOp< T, OP >::value,
                   "No MPI atomic op implementation for this operator" );
  
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    auto dest_int = reinterpret_cast< MPI_Aint >( dest );
    CHECK_LT( dest_int,  std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( sizeof(T), std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    MPI_CHECK( MPI_Fetch_and_op( source, result, Grappa::impl::MPIDatatype< T >::value,
                                 core, dest_int,
                                 Grappa::impl::MPIOp< T, OP >::value,
                                 dynamic_window_ ) );
  }

  // Perform atomic op on remote memory location, blocking until the
  // operation is complete. See MPI.hpp for supported operations.
  template< typename T, typename OP >
  T atomic_op( const Core core, T * dest, OP op, const T * source ) {
    T result;

    // start operation
    atomic_op_nbi( &result, core, dest, op, source );

    // make sure operation is complete
    MPI_CHECK( MPI_Win_flush( core, dynamic_window_ ) );
    
    return result;
  }

  // Atomic compare-and-swap. If dest and compare values are the same,
  // replace dest with source. Return previous value of dest. See
  // MPI.hpp for supported operations.
  template< typename T >
  void compare_and_swap_nbi( T * result, const Core core, T * dest, const T * compare, const T * source ) {
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    auto dest_int = reinterpret_cast< MPI_Aint >( dest );
    CHECK_LT( dest_int,  std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( sizeof(T), std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    MPI_CHECK( MPI_Compare_and_swap( source, compare, result, Grappa::impl::MPIDatatype< T >::value,
                                     core, dest_int,
                                     dynamic_window_ ) );
  }

  // Atomic compare-and-swap. If dest and compare values are the same,
  // replace dest with source. Return previous value of dest. Blocks
  // until complete. See MPI.hpp for supported operations.
  template< typename T >
  T compare_and_swap( const Core core, T * dest, const T * compare, const T * source ) {
    T result;

    // start operation
    compare_and_swap_nbi( &result, core, dest, compare, source );
    
    // make sure operation is complete
    MPI_CHECK( MPI_Win_flush( core, dynamic_window_ ) );
    
    return result;
  }

};

//
// static RMA instance
//

extern RMA global_rma;

namespace rma {
} // namespace rma

} // namespace impl
} // namespace Grappa
