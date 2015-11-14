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

#include <unordered_set>
#include <map>
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
    MPI_CHECK( MPI_Wait( &request_, MPI_STATUS_IGNORE ) );
  }

  /// Warning: may leak request if someone has not completed it elsewhere.
  void reset_nowait() {
    request_ = MPI_REQUEST_NULL;
  }
  
  void wait() {
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
  std::map< intptr_t, RMAWindow > address_map_;

  std::map< intptr_t, RMAWindow >::const_iterator get_enclosing( intptr_t base_int ) {
    // this points to the region after this address
    auto it = address_map_.upper_bound( base_int );
    if( it != address_map_.begin() ) {
      it--;
      auto region_base_int = reinterpret_cast<intptr_t>( it->second.base_ );
      auto region_size = it->second.size_;
      if( ( region_base_int <= base_int ) &&
          ( base_int < ( region_base_int + region_size ) ) ) {
        return it;
      }
    }
    return address_map_.end();
  }

  // we're requiring here that windows don't overlap, which should be
  // fine for an allocate() function. Registering regions allocated
  // elsewhere may need to deal with overlap.
  void verify_no_overlap( intptr_t base_int, size_t size ) {
    DVLOG(2) << "Adding region with base "
             << (void*) base_int
             << " size " << size
             << "/" << (void*) size 
             << " to " << *this;
    
    // check if we overlap with a window starting at an address equal to or higher than our base
    auto it = address_map_.lower_bound( base_int );
    if( it != address_map_.end() ) {
      auto other_base_int = reinterpret_cast<intptr_t>( it->second.base_ );
      auto other_size = it->second.size_;
      CHECK_LE( base_int + size, other_base_int )
        << "Newly-allocated window (base "
        << (void*) base_int << " size " << size << ")"
        << " overlaps with previously-allocated window (base "
        << (void*) other_base_int << " size " << other_size << ").";
    }

    // since we don't allow overlapping windows, if a window starting
    // at an earlier address overlaps with the window we're checking,
    // it must be the one previous to the lower_bound result (or the
    // highest, if there was no result).
    if( it != address_map_.begin() ) {
      it--;
      if( it != address_map_.end() ) {
        auto other_base_int = reinterpret_cast<intptr_t>( it->second.base_ );
        auto other_size = it->second.size_;
        CHECK_LE( other_base_int + other_size, base_int )
          << "Newly-allocated window (base "
          << (void*) base_int << " size " << size << ")"
          << " overlaps with previously-allocated window (base "
          << (void*) other_base_int << " size " << other_size << ").";
      }
    }
  }
  
  int64_t atomic_op_int64_helper( const Core core, int64_t * dest, const int64_t * source, MPI_Op op );

public:
  RMA()
    : address_map_()
  { }

  const size_t size() const { return address_map_.size(); }
  
  friend std::ostream & operator<<( std::ostream & o, const RMA & rma ) {
    o << "[RMA address map <";
    for( auto const & kv : rma.address_map_ ) {
      o << "\n   " << (void*) kv.first << "->" << kv.second << " ";
    }
    return o << ">]";
  }
  
  // collective call to allocate window for passive one-sided ops
  void * allocate( size_t size ) {
    MPI_Info info;
    MPI_CHECK( MPI_Info_create( &info ) );

    // Assure MPI that all locale shared memory segments have identical sizes
    MPI_CHECK( MPI_Info_set( info, "same_size", "true" ) );
    // Assure MPI that all locale shared memory segments have identical displacement units
    MPI_CHECK( MPI_Info_set( info, "same_disp_unit", "true" ) );

    // Allocate and add to map
    MPI_Win window;
    void * base;
    MPI_CHECK( MPI_Win_allocate( size, 1,
                                 info,
                                 global_communicator.grappa_comm,
                                 &base,
                                 &window ) );
    intptr_t base_int = reinterpret_cast<intptr_t>(base);
    verify_no_overlap( base_int, size ); 
    address_map_.emplace( base_int, RMAWindow( base, size, window ) );
      
    // Clean up info struct now that we're done with it
    MPI_CHECK( MPI_Info_free( &info ) );
    
    // Verify we got what we expected
    int * attr;
    int attr_flag;
    MPI_CHECK( MPI_Win_get_attr( window, MPI_WIN_MODEL, &attr, &attr_flag ) );
    CHECK( attr_flag ) << "Error getting attributes for MPI window";
    CHECK_EQ( *attr, MPI_WIN_UNIFIED ) << "MPI window doesn't use MPI_WIN_UNIFIED model.";

    // "acquire locks" for all other regions to allow async puts and gets
    MPI_CHECK( MPI_Win_lock_all( MPI_MODE_NOCHECK, window ) );
    MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

    return base;
  }

  // collective call to free window
  void free( void * base ) {
    // synchronize
    MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

    // find window for address
    auto it = address_map_.lower_bound( reinterpret_cast<intptr_t>(base) );
    CHECK_EQ( it->second.base_, base ) << "Called free() with different address than what was allocated!";
    
    // unlock and free
    MPI_CHECK( MPI_Win_unlock_all( it->second.window_ ) );
    MPI_CHECK( MPI_Win_free( &it->second.window_ ) );

    // remove from map 
    address_map_.erase( it );
  }

  // non-collective local fence/flush operation
  void fence() {
    for( auto & kv : address_map_ ) {
      MPI_CHECK( MPI_Win_flush_all( kv.second.window_ ) );
    }
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void put_bytes_nbi( const Core core, void * dest, const void * source, const size_t size ) {
    auto dest_int = reinterpret_cast<intptr_t>(dest);
    auto it = get_enclosing( dest_int );
    DVLOG(2) << "Found " << it->second << " for dest " << dest << " size " << size;
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = dest_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
    
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Put( source, size, MPI_CHAR,
                        core, offset, size, MPI_CHAR,
                        it->second.window_ ) );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. An MPI_Request is passed in to be used for completion
  // detection.
  void put_bytes_nb( const Core core, void * dest, const void * source, const size_t size, MPI_Request * request_p ) {
    auto dest_int = reinterpret_cast<intptr_t>(dest);
    auto it = get_enclosing( dest_int );
    DVLOG(2) << "Found " << it->second << " for dest " << dest << " size " << size;
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = dest_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
    
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_Request request;
    MPI_CHECK( MPI_Rput( source, size, MPI_CHAR,
                         core, offset, size, MPI_CHAR,
                         it->second.window_,
                         request_p ) );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  RMARequest put_bytes_nb( const Core core, void * dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    return RMARequest( request );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void put_bytes( const Core core, void * dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    RMARequest( request ).wait();
  }

  // Copy bytes from remote memory location. For non-symmetric
  // allocations, source pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void get_bytes_nbi( void * dest, const Core core, const void * source, const size_t size ) {
    auto source_int = reinterpret_cast<intptr_t>(source);
    auto it = address_map_.lower_bound( source_int );
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = source_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
    
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Get( dest, size, MPI_CHAR,
                        core, offset, size, MPI_CHAR,
                        it->second.window_ ) );
  }

  // Copy bytes from remote memory location. For non-symmetric
  // allocations, source pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. An MPI_Request is passed in to be used for completion
  // detection.
  void get_bytes_nb( void * dest, const Core core, const void * source, const size_t size, MPI_Request * request_p ) {
    auto source_int = reinterpret_cast<intptr_t>(source);
    auto it = address_map_.lower_bound( source_int );
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = source_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
    
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Rget( dest, size, MPI_CHAR,
                         core, offset, size, MPI_CHAR,
                         it->second.window_,
                         request_p ) );
  }

  RMARequest get_bytes_nb( void * dest, const Core core, const void * source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    return RMARequest( request );
  }

  void get_bytes( void * dest, const Core core, const void * source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    RMARequest( request ).wait();
  }

  // Perform atomic op on remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T, typename OP >
  T atomic_op( const Core core, T * dest, OP op, const T * source ) {
    static_assert( MPI_OP_NULL != Grappa::impl::MPIOp< T, OP >::value,
                   "No MPI atomic op implementation for this operator" );
    size_t size = sizeof(T);
    auto dest_int = reinterpret_cast<intptr_t>(dest);
    auto it = get_enclosing( dest_int );
    DVLOG(2) << "Found " << it->second << " for dest " << dest << " size " << size;
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = dest_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
  
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    int64_t result;
    MPI_CHECK( MPI_Fetch_and_op( source, &result, Grappa::impl::MPIDatatype< T >::value,
                                 core, offset,
                                 Grappa::impl::MPIOp< T, OP >::value,
                                 it->second.window_ ) );
    MPI_CHECK( MPI_Win_flush_all( it->second.window_ ) );
    
    return result;
  }

  // Atomic compare-and-swap. If dest and compare values are the same,
  // replace dest with source. Return previous value of dest. For
  // non-symmetric allocations, dest pointer is converted to a remote
  // offset relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T >
  T compare_and_swap( const Core core, T * dest, const T * compare, const T * source ) {
    size_t size = sizeof(T);
    auto dest_int = reinterpret_cast<intptr_t>(dest);
    auto it = get_enclosing( dest_int );
    DVLOG(2) << "Found " << it->second << " for dest " << dest << " size " << size;
    auto base_int = reinterpret_cast<intptr_t>( it->second.base_ );
    auto offset = dest_int - base_int;
    CHECK_LE( offset + size, it->second.size_ ) << "Operation would overrun RMA window";
  
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    CHECK_LT( offset, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,   std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    int64_t result;
    MPI_CHECK( MPI_Compare_and_swap( source, compare, &result, Grappa::impl::MPIDatatype< T >::value,
                                     core, offset,
                                     it->second.window_ ) );
    MPI_CHECK( MPI_Win_flush_all( it->second.window_ ) );

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
