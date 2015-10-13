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

#ifndef __ADDRESSING_HPP__
#define __ADDRESSING_HPP__

/// Global Addresses for Grappa
///
///
/// We support two types of global addresses:
/// -# 2D addresses
/// -# Linear addresses 
///
/// 2D addresses are a PGAS-style tuple of ( core, address on core )
///
/// Linear addresses are block cyclic across all the cores in the system.
///
/// TODO: update "core" to mean "core" in all the right places.  
///
/// TODO:Some of this code implies that linear addresses have a "pool"
/// field. This is not how it works right now; instead, all non-tag
/// bits store core id and address on core encoded in a way that makes
/// incrementing by blocks work.

#include "Communicator.hpp"

typedef int Pool;


/// this core's base pointer
namespace Grappa {
namespace impl {
extern void * global_memory_chunk_base;
}
}



/// assumes user data will have the top 16 bits all 0.

/// Number of bytes in each block 
#define BLOCK_SIZE sizeof(int64_t)*8
static const int block_size = BLOCK_SIZE;

#define GRAPPA_BLOCK_ALIGNED __attribute__((aligned(BLOCK_SIZE)))

/// How many address type bits?
static const int tag_bits = 1;
/// How many address bits?
static const int pointer_bits = 48;
/// How many 
static const int core_bits = 64 - pointer_bits - tag_bits;
/// How many bits of memory poll tag? TODO: not currently used.
static const int pool_bits = 64 - pointer_bits - tag_bits;

static const int tag_shift_val = 64 - tag_bits;
static const int pointer_shift_val = 64 - pointer_bits;
static const int core_shift_val = pointer_bits;
static const int pool_shift_val = pointer_bits;

static const intptr_t tag_mask = (1L << tag_shift_val);
static const intptr_t core_mask = (1L << core_bits) - 1;
static const intptr_t pool_mask = (1L << pool_bits) - 1;
static const intptr_t pointer_mask = (1L << pointer_bits) - 1;

/// @addtogroup Memory
/// @{

/// Global address class
///
/// We support two types of global addresses:
/// -# 2D addresses
/// -# Linear addresses 
///
/// 2D addresses are a PGAS-style tuple of ( core, address on core )
///
/// Linear addresses are block cyclic across all the cores in the system.
template< typename T >
class GlobalAddress {
private:

  /// Storage for address
  intptr_t storage_;
  //DISALLOW_COPY_AND_ASSIGN( GlobalAddress );
  template< typename U > friend std::ostream& operator<<( std::ostream& o, const GlobalAddress< U >& ga );
  //friend template< typename U > GlobalAddress< U > operator+( const GlobalAddress< U >& t, ptrdiff_t i );
  template< typename U > friend ptrdiff_t operator-( const GlobalAddress< U >& t, const GlobalAddress< U >& u );

  /// Output global address in human-readable form
  std::ostream& dump( std::ostream& o ) const {
    if( is_2D() ) {
      return o << "<GA 2D " << (void*)storage_ 
               << ": core " << core() 
               << " pointer " << static_cast<void *>( pointer() )
               << ">";
    } else {
        return o << "<GA Linear " << (void*)storage_ 
//                 << ": pool " << pool() 
                 << " core " << core()
                 << " pointer " << static_cast<void *>( pointer()  )
                 << ">";
    }
  }

  // GlobalAddress( T * p, Core n = global_communicator.mycore )
  //   : storage_( ( 1L << tag_shift_val ) |
  //               ( ( n & core_mask) << core_shift_val ) |
  //               ( reinterpret_cast<intptr_t>( p ) ) )
  // {
  //   assert( global_communicator.mycore <= core_mask );
  //   assert( reinterpret_cast<intptr_t>( p ) >> core_shift_val == 0 );
  // }

public:

  /// Construct a global address, initialized to a null pointer
  GlobalAddress( ) : storage_( 0 ) { }

  /// Construct a 2D global address with an initial pointer and core.
  static GlobalAddress TwoDimensional( T * t, Core n = global_communicator.mycore )
  {
    GlobalAddress g;
    g.storage_ = ( ( 1L << tag_shift_val ) |
                   ( ( n & core_mask) << core_shift_val ) |
                   ( reinterpret_cast<intptr_t>( t ) ) );
    assert( global_communicator.mycore <= core_mask );
    assert( reinterpret_cast<intptr_t>( t ) >> core_shift_val == 0 );
    return g;
  }

  /// Construct a linear global address from a local pointer.
  /// TODO: the pool argument is currenly unused
  static GlobalAddress Linear( T * t, Pool p = 0 )
  {
    // adjust for chunk offset
    intptr_t tt = reinterpret_cast< intptr_t >( t ) - 
      reinterpret_cast< intptr_t >( Grappa::impl::global_memory_chunk_base );

    intptr_t offset = tt % block_size;
    intptr_t block = tt / block_size;
    // intptr_t core_from_address = block % global_communicator.cores;
    // CHECK_EQ( core_from_address, 0 ) << "Core from address should be zero. (Check alignment?)";
    intptr_t ga = ( block * global_communicator.cores + global_communicator.mycore ) * block_size + offset;
    
    T * ttt = reinterpret_cast< T * >( ga );
    
    GlobalAddress g;
    g.storage_ = ( ( 0L << tag_shift_val ) |
                   //( ( n & core_mask) << core_shift_val ) |
                   ( reinterpret_cast<intptr_t>( ttt ) ) );

    CHECK_EQ( g.core(), global_communicator.mycore ) << "converted linear address core doesn't match";
    CHECK_EQ( g.pointer(), t ) << "converted linear address local pointer doesn't match";
    
    return g;
  }

  /// Construct a global address from raw bits
  static GlobalAddress Raw( intptr_t t )
  {
    GlobalAddress g;
    g.storage_ = t;
    return g;
  }

  /// Return the raw bits of a global address
  inline intptr_t raw_bits() const {
    return storage_;
  }

  /// Return the home core of a global address
  inline Core core() const {
    if( is_2D() ) {
      return (storage_ >> core_shift_val) & core_mask;
    } else {
      intptr_t offset = storage_ % block_size;
      intptr_t core = (storage_ / block_size) % global_communicator.cores;
      intptr_t core_block = (storage_ / block_size) / global_communicator.cores;
      return core;
    }
  }
  
  /// Return the home core of a global address
  /// TODO: implement this.
  inline Pool pool() const {
    CHECK( false ) << "Not implemented.";
    return (storage_ >> pool_shift_val) & pool_mask;
  }
    
  /// Return the local pointer from a global address
  inline T * pointer() const { 
    if( is_2D() ) {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      return reinterpret_cast< T * >( signextended ); 
    } else {
      intptr_t offset = storage_ % block_size;
      intptr_t block = (storage_ / block_size);
      intptr_t core = (storage_ / block_size) % global_communicator.cores;
      intptr_t core_block = (storage_ / block_size) / global_communicator.cores;
      intptr_t address = core_block * block_size + offset + 
        reinterpret_cast< intptr_t >( Grappa::impl::global_memory_chunk_base );
      return reinterpret_cast< T * >( address );
    }
  }

  /// Find lowest local address of the object at this address.  Used
  /// for PGAS-style local iteration.
  inline T * localize(Core nid = -1) const {
    if (is_2D()) return pointer();
    
  	if (nid == -1) nid = global_communicator.mycore;
    T * local_base;
    size_t block_elems = block_size / sizeof(T);
    T * block_base = block_min().pointer();
    if (nid < core()) {
      local_base = block_base+block_elems;
    } else if (nid > core()) {
      local_base = block_base;
    } else {
      local_base = pointer();
    }
    return local_base;
  }
  
  /// Does @b not remotely dereference the global address, this is syntactic sugar for `localize()->`
  ///
  /// @code
  ///   struct { int y; } x = {1};
  ///   GlobalAddress<decltype(x)> xa = make_global(&x);
  ///   cout << xa->y;
  ///   //> 1
  /// @endcode
  T* operator->() const { return localize(); }

  /// Find base address of block containing this byte.
  inline GlobalAddress< T > block_min() const { 
    if( is_2D() ) {
      //intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      //GlobalAddress< U > u = GlobalAddress< U >::Raw( storage_ );
      return GlobalAddress< T >::TwoDimensional( (T*) 0, core() );
    } else {
      intptr_t first_byte = storage_;
      intptr_t first_byte_offset = first_byte % block_size;
      intptr_t core = (first_byte / block_size) %   global_communicator.cores;
      intptr_t core_block = (first_byte / block_size) / global_communicator.cores;
      return GlobalAddress< T >::Raw( this->raw_bits() - first_byte_offset );
    }
  }

  /// find global byte address of first byte of block
  inline GlobalAddress< char > first_byte() const { 
    return GlobalAddress< char >::Raw( this->raw_bits() );
  }

  /// find global byte address of last byte of block
  inline GlobalAddress< char > last_byte() const { 
    return GlobalAddress< char >::Raw( this->raw_bits() + sizeof(T) - 1 );
  }

  // base address of block after the one containing the last byte of this object
  inline GlobalAddress< T > block_max() const { 
    if( is_2D() ) {
      intptr_t signextended = (storage_ << pointer_shift_val) >> pointer_shift_val;
      //return reinterpret_cast< T * >( -1 ); 
      return GlobalAddress< T >::TwoDimensional( (T*) 1, core() );
    } else {
      intptr_t first_byte = storage_;
      intptr_t first_byte_offset = first_byte % block_size;
      intptr_t last_byte = first_byte + sizeof(T) - 1;
      intptr_t last_byte_offset = last_byte % block_size;
      intptr_t core = (last_byte / block_size) % global_communicator.cores;
      intptr_t core_block = (last_byte / block_size) / global_communicator.cores;
      return GlobalAddress< T >::Raw( this->raw_bits() + sizeof(T) + block_size - (last_byte_offset + 1) );
    }
  }

  /// is this a 2D address?
  inline bool is_2D() const {
    return storage_ & tag_mask; 
  }

  /// is this a linear address?
  inline bool is_linear() const {
    return !( storage_ & tag_mask );
  }

  // inline size_t block_size() const {
  //   return block_size_;
  // }

  /// increment address by one T
  inline GlobalAddress< T >& operator++() {
    storage_ += sizeof(T); 
    return *this;
  }
  
  /// increment address by one T
  inline GlobalAddress< T > operator++(int) {
    GlobalAddress<T> result = *this;
    storage_ += sizeof(T);
    return result;
  }

  /// decrement address by one T
  inline GlobalAddress< T >& operator--() { 
    storage_ -= sizeof(T); 
    return *this; 
  }
  
  //inline GlobalAddress< T > operator--(int i) { return storage_ ++ i; }
  /// increment address by i T's
  inline GlobalAddress< T >& operator+=(ptrdiff_t i) { 
    storage_ += i * sizeof(T); 
    return *this; 
  }

  /// decrement address by i T's
  inline GlobalAddress< T >& operator-=(ptrdiff_t i) { 
    storage_ -= i * sizeof(T); 
    return *this; 
  }

  /// test for equality
  /// TODO: get rid of this
  bool equals( const GlobalAddress< T >& t ) const {
    return raw_bits() == t.raw_bits();
  }

  /// test for equality
  /// TODO: get rid of this
  bool operator==( const GlobalAddress< T >& t ) const {
    return raw_bits() == t.raw_bits();
  }

  /// test for equality
  template< typename U >
  bool operator==( const GlobalAddress< U >& u ) const {
    return raw_bits() == u.raw_bits();
  }
  
  /// compare addresses
  bool operator<(const GlobalAddress<T>& t) const {
    return raw_bits() < t.raw_bits();
  }

  //T& operator[]( ptrdiff_t index ) { return 

  /// generic cast operator
  template< typename U >
  explicit operator GlobalAddress< U >( ) {
    GlobalAddress< U > u = GlobalAddress< U >::Raw( storage_ );
    return u;
  }

  /// cast to void *.
  operator void * ( ) {
    void * u = reinterpret_cast< void * >( storage_ );
    return u;
  }

};


/// return an address that's i T's more than t.
template< typename T >
GlobalAddress< T > operator+( const GlobalAddress< T >& t, ptrdiff_t i ) {
  GlobalAddress< T > tt(t);
  return tt += i;
}

// template< typename T >
// GlobalAddress< T >&& operator+( GlobalAddress< T >&& t, ptrdiff_t i ) {
//   return t += i;
// }

/// return an address that's i T's less than t.
template< typename T >
GlobalAddress< T > operator-( const GlobalAddress< T >& t, ptrdiff_t i ) {
  GlobalAddress< T > tt(t);
  return tt -= i;
}

// template< typename T >
// GlobalAddress< T >&& operator-( const GlobalAddress< T >&& t, ptrdiff_t i ) {
//   return t -= i;
// }

/// how many T's different are t and u?
template< typename T >
inline ptrdiff_t operator-( const GlobalAddress< T >& t, const GlobalAddress< T >& u ) {
  //LOG(WARNING) << "Danger! You probably don't want to call this function. " 
		   //<< "Try using .first_byte() or .last_byte() to get the right overload";
  return (t.raw_bits() - u.raw_bits())/sizeof(T);
}

/// how many bytes different are t and u?
template<  >
inline ptrdiff_t operator-<char>( const GlobalAddress< char >& t, const GlobalAddress< char >& u ) {
  return t.raw_bits() - u.raw_bits();
}

/// return a 2d global pointer to a local pointer on a particular core
template< typename T >
GlobalAddress< T > make_global( T * t, Core n = global_communicator.mycore ) {
  return GlobalAddress< T >::TwoDimensional( t, n );
}

/// takes a local pointer to a block-cyclic distributed chuck of
/// memory allocated at the same base address on all cores, and makes
/// a linear global pointer pointing to that byte.
template< typename T >
GlobalAddress< T > make_linear( T * t ) {
  return GlobalAddress< T >::Linear( t );
}

/// output human-readable version of global address
template< typename T >
std::ostream& operator<<( std::ostream& o, const GlobalAddress< T >& ga ) {
  return ga.dump( o );
}

/// computes offsets of members in structs and claases
/// call like this:
/// @code
///   struct Foo {
///     int i;
///   } foo;
///   GlobalAddress< Foo > foo_gp = make_global( foo );
///   GlobalAddress< int > foo_i_gp = global_pointer_to_member( foo_gp, &Foo::i );
/// @endcode
template< typename T, typename M >
inline GlobalAddress< M > global_pointer_to_member( const GlobalAddress< T > t, const M T::*m ) {
  const intptr_t t_raw = t.raw_bits();
  const T * tp = reinterpret_cast< T * >( t_raw );
  const M * mp = &(tp->*m);
  return GlobalAddress< M >::Raw( reinterpret_cast< intptr_t >( mp ) );
}

template<typename T>
struct LocalIterator {
  GlobalAddress<T> base;
  size_t nelem;
  T * begin() { return base.localize(); }
  T * end()   { return (base+nelem).localize(); }
  size_t size() { return end() - begin(); }
};

/// Helper for iterating over local elements of a Linear address range.
///
/// @code
///   auto array = global_alloc<long>(N);
///   on_all_cores([]{
///     for (auto& v : iterate_local(array,N)) {
///       v++;
///     }
///   });
/// @endcode
template<typename T>
LocalIterator<T> iterate_local(GlobalAddress<T> base, size_t nelem) { return LocalIterator<T>{base, nelem}; }

/// @}
//template< typename T >
#endif
