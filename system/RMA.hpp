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

class RMAAddress {
private:
  friend class RMA;
  void * base_;
  MPI_Win  window_;
  MPI_Aint offset_;

public:
  RMAAddress()
    : base_( 0 )
    , window_( MPI_WIN_NULL )
    , offset_( 0 )
  { }

  RMAAddress( void * base, MPI_Win window, MPI_Aint byte_offset )
    : base_( base )
    , window_( window )
    , offset_( byte_offset )
  { }

  void * to_local() {
    return reinterpret_cast<void*>( reinterpret_cast<char*>(base_) + offset_ );
  }
};


class RMA {
private:
  typedef std::map< intptr_t, RMAWindow > AddressMap;
  typedef std::vector< AddressMap > AddressMaps;
  AddressMaps address_maps_;

  typedef std::map< MPI_Win, RMAWindow > WindowMap;
  typedef std::vector< WindowMap > WindowMaps;
  AddressMaps window_maps_;

  AddressMap::iterator get_enclosing( Core core, intptr_t base_int ) {
    // this points to the region after this address
    auto it = address_maps_[core].upper_bound( base_int );
    if( it != address_maps_[core].begin() ) {
      it--;
      auto region_base_int = reinterpret_cast<intptr_t>( it->second.base_ );
      auto region_size = it->second.size_;
      if( ( region_base_int <= base_int ) &&
          ( base_int < ( region_base_int + region_size ) ) ) {
        return it;
      }
    }
    return address_maps_[core].end();
  }

  // we're requiring here that windows don't overlap, which should be
  // fine for an allocate() function. Registering regions allocated
  // elsewhere may need to deal with overlap.
  void verify_no_overlap( Core core, intptr_t base_int, size_t size ) {
    DVLOG(2) << "Adding region with base "
             << (void*) base_int
             << " size " << size
             << "/" << (void*) size 
             << " to " << *this;
    
    // check if we overlap with a window starting at an address equal to or higher than our base
    auto it = address_maps_[core].lower_bound( base_int );
    if( it != address_maps_[core].end() ) {
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
    if( it != address_maps_[core].begin() ) {
      it--;
      if( it != address_maps_[core].end() ) {
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
    : address_maps_()
  { }

  void init() {
    address_maps_.resize( cores() );
    window_maps_.resize( cores() );
  }

  const size_t size() const { return address_maps_[mycore()].size(); }
  
  friend std::ostream & operator<<( std::ostream & o, const RMA & rma ) {
    o << "[RMA address map <";
    Core i = 0;
    for( auto const & map : rma.address_maps_ ) {
      o << "\n   Core " << i++ << ":";
      for( auto const & kv : map ) {
        o << "\n      " << (void*) kv.first << "->" << kv.second << " ";
      }
    }
    return o << ">]";
  }

  RMAAddress to_global( Core core, void * local ) {
    auto local_int = reinterpret_cast< intptr_t >( local );
    auto it = get_enclosing( core, local_int );
    CHECK( it != address_maps_[core].end() ) << "No mapping found for " << (void*) local << " on core " << core;
    auto offset = local_int - it->first;
    CHECK_LT( offset, std::numeric_limits<MPI_Aint>::max() ) << "Operation would overflow MPI offset argument type";
    return RMAAddress( it->second.base_, it->second.window_, offset );
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
    verify_no_overlap( mycore(), base_int, size ); 

    // Collect all mappings and insert
    {
      MPI_Win windows[ cores() ];
      void *  bases[ cores() ];
      size_t  sizes[ cores() ];

      MPI_CHECK( MPI_Allgather( &window, sizeof(window), MPI_BYTE,
                                &windows[0], sizeof(window), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      MPI_CHECK( MPI_Allgather( &base, sizeof(base), MPI_BYTE,
                                &bases[0], sizeof(base), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      MPI_CHECK( MPI_Allgather( &size, sizeof(size), MPI_BYTE,
                                &sizes[0], sizeof(size), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      for( int i = 0; i < cores(); ++i ) {
        intptr_t base_int = reinterpret_cast<intptr_t>( bases[i] );
        DVLOG(5) << "Core " << i << " allocated " << sizes[i] << " bytes at " << bases[i] << " window " << windows[i];
        address_maps_[i].insert( { base_int, RMAWindow( bases[i], sizes[i], windows[i] ) } );
        window_maps_[i].insert( { windows[i], RMAWindow( bases[i], sizes[i], windows[i] ) } );
      }
    }

      
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

  // collective call to register window for passive one-sided ops
  void register_region( void * base, size_t size ) {
    MPI_Info info;
    MPI_CHECK( MPI_Info_create( &info ) );

    // Assure MPI that all locale shared memory segments have identical displacement units
    MPI_CHECK( MPI_Info_set( info, "same_disp_unit", "true" ) );

    // Allocate and add to map
    MPI_Win window;
    MPI_CHECK( MPI_Win_create( base, size, 1,
                               info,
                               global_communicator.grappa_comm,
                               &window ) );
    intptr_t base_int = reinterpret_cast<intptr_t>(base);
    verify_no_overlap( mycore(), base_int, size );

    // Collect all mappings and insert (TODO)
    {
      MPI_Win windows[ cores() ];
      void *  bases[ cores() ];
      size_t  sizes[ cores() ];

      MPI_CHECK( MPI_Allgather( &window, sizeof(window), MPI_BYTE,
                                &windows[0], sizeof(window), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      MPI_CHECK( MPI_Allgather( &base, sizeof(base), MPI_BYTE,
                                &bases[0], sizeof(base), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      MPI_CHECK( MPI_Allgather( &size, sizeof(size), MPI_BYTE,
                                &sizes[0], sizeof(size), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      for( int i = 0; i < cores(); ++i ) {
        intptr_t base_int = reinterpret_cast<intptr_t>( bases[i] );
        DVLOG(5) << "Core " << i << " allocated " << sizes[i] << " bytes at " << bases[i] << " window " << windows[i];
        address_maps_[i].insert( { base_int, RMAWindow( bases[i], sizes[i], windows[i] ) } );
        window_maps_[i].insert( { windows[i], RMAWindow( bases[i], sizes[i], windows[i] ) } );
      }
    }
      
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
  }

  // collective call to free window
  void free( void * base ) {
    // synchronize
    MPI_CHECK( MPI_Barrier( global_communicator.grappa_comm ) );

    // find window for address
    intptr_t base_int = reinterpret_cast<intptr_t>( base );
    auto it = get_enclosing( mycore(), base_int );
    CHECK_EQ( it->second.base_, base ) << "Called free() with different address than what was allocated!";
    MPI_Win window = it->second.window_;

    // unlock and free
    MPI_CHECK( MPI_Win_unlock_all( it->second.window_ ) );
    MPI_CHECK( MPI_Win_free( &it->second.window_ ) );
    
    // Collect all mappings and remove from maps (TODO)
    {
      void * bases[ cores() ];
      MPI_CHECK( MPI_Allgather( &base, sizeof(base), MPI_BYTE,
                                &bases[0], sizeof(base), MPI_BYTE,
                                global_communicator.grappa_comm ) );
      for( int i = 0; i < cores(); ++i ) {
        intptr_t base_int = reinterpret_cast<intptr_t>( bases[i] );
        DVLOG(5) << "Core " << i << " freeing " << bases[i];
        auto it = get_enclosing( i, base_int );
        address_maps_[i].erase( it );
        auto it2 = window_maps_[i].find( window );
        window_maps_[i].erase( it2 );
      }
    }
  }
  
  // deregistering memory allocated elsewhere is the same as freeing it in the MPI API
  void deregister_region( void * base ) {
    free(base);
  }
  
  // deregistering memory allocated elsewhere is the same as freeing it in the MPI API
  void deregister_or_free( void * base ) {
    free(base);
  }
  
  // check if an address is already registered
  bool registered( void * base ) {
    return get_enclosing( mycore(), reinterpret_cast<intptr_t>(base) ) != address_maps_[mycore()].end();
  }

  template< typename T >
  T * rebase( T * addr, Core from = mycore() ) {
    auto addr_int = reinterpret_cast<intptr_t>(addr);
    auto it = get_enclosing( from, addr_int );
    CHECK( it != address_maps_[from].end() ) << "No mapping found for " << from << "/" << addr << " size " << size;

  }

  // non-collective local fence/flush operation
  void fence() {
    for( auto & kv : address_maps_[mycore()] ) {
      MPI_CHECK( MPI_Win_flush_all( kv.second.window_ ) );
    }
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void put_bytes_nbi( const Core core, RMAAddress dest, const void * source, const size_t size ) {
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( dest.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,         std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Put( source, size, MPI_CHAR,
                        core, dest.offset_, size, MPI_CHAR,
                        dest.window_ ) );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. An MPI_Request is passed in to be used for completion
  // detection.
  void put_bytes_nb( const Core core, RMAAddress dest, const void * source, const size_t size, MPI_Request * request_p ) {
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( dest.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,         std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_Request request;
    MPI_CHECK( MPI_Rput( source, size, MPI_CHAR,
                         core, dest.offset_, size, MPI_CHAR,
                         dest.window_,
                         request_p ) );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  RMARequest put_bytes_nb( const Core core, RMAAddress dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    return RMARequest( request );
  }

  // Copy bytes to remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void put_bytes( const Core core, RMAAddress dest, const void * source, const size_t size ) {
    MPI_Request request;
    put_bytes_nb( core, dest, source, size, &request );
    RMARequest( request ).wait();
  }

  // Copy bytes from remote memory location. For non-symmetric
  // allocations, source pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local rank.
  void get_bytes_nbi( void * dest, const Core core, const RMAAddress source, const size_t size ) {
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( source.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,           std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Get( dest, size, MPI_CHAR,
                        core, source.offset_, size, MPI_CHAR,
                        source.window_ ) );
  }

  // Copy bytes from remote memory location. For non-symmetric
  // allocations, source pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. An MPI_Request is passed in to be used for completion
  // detection.
  void get_bytes_nb( void * dest, const Core core, const RMAAddress source, const size_t size, MPI_Request * request_p ) {
    // TODO: deal with >31-bit offsets and sizes
    CHECK_LT( source.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( size,           std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    MPI_CHECK( MPI_Rget( dest, size, MPI_CHAR,
                         core, source.offset_, size, MPI_CHAR,
                         source.window_,
                         request_p ) );
  }

  RMARequest get_bytes_nb( void * dest, const Core core, const RMAAddress source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    return RMARequest( request );
  }

  void get_bytes( void * dest, const Core core, const RMAAddress source, const size_t size ) {
    MPI_Request request;
    get_bytes_nb( dest, core, source, size, &request );
    RMARequest( request ).wait();
  }

  // Perform atomic op on remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T, typename OP >
  void atomic_op_nbi( T * result, const Core core, RMAAddress dest, OP op, const T * source ) {
    static_assert( MPI_OP_NULL != Grappa::impl::MPIOp< T, OP >::value,
                   "No MPI atomic op implementation for this operator" );
  
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    CHECK_LT( dest.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( sizeof(T),    std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    MPI_CHECK( MPI_Fetch_and_op( source, result, Grappa::impl::MPIDatatype< T >::value,
                                 core, dest.offset_,
                                 Grappa::impl::MPIOp< T, OP >::value,
                                 dest.window_ ) );
  }

  // Perform atomic op on remote memory location. For non-symmetric
  // allocations, dest pointer is converted to a remote offset
  // relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T, typename OP >
  T atomic_op( const Core core, RMAAddress dest, OP op, const T * source ) {
    T result;

    // start operation
    atomic_op_nbi( &result, core, dest, op, source );

    // make sure operation is complete
    MPI_CHECK( MPI_Win_flush( core, dest.window_ ) );
    
    return result;
  }

  // Atomic compare-and-swap. If dest and compare values are the same,
  // replace dest with source. Return previous value of dest. For
  // non-symmetric allocations, dest pointer is converted to a remote
  // offset relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T >
  void compare_and_swap_nbi( T * result, const Core core, RMAAddress dest, const T * compare, const T * source ) {
    // TODO: deal with >31-bit offsets and sizes. For now, just report error.
    CHECK_LT( dest.offset_, std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";
    CHECK_LT( sizeof(T),    std::numeric_limits<int>::max() ) << "Operation would overflow MPI argument type";

    MPI_CHECK( MPI_Compare_and_swap( source, compare, result, Grappa::impl::MPIDatatype< T >::value,
                                     core, dest.offset_,
                                     dest.window_ ) );
  }

  // Atomic compare-and-swap. If dest and compare values are the same,
  // replace dest with source. Return previous value of dest. For
  // non-symmetric allocations, dest pointer is converted to a remote
  // offset relative to the base of the enclosing window on the local
  // rank. See MPI.hpp for supported operations.
  template< typename T >
  T compare_and_swap( const Core core, RMAAddress dest, const T * compare, const T * source ) {
    T result;

    // start operation
    compare_and_swap_nbi( &result, core, dest, compare, source );
    
    // make sure operation is complete
    MPI_CHECK( MPI_Win_flush( core, dest.window_ ) );
    
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
