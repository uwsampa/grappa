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

#ifndef __GLOBAL_ALLOCATOR_HPP__
#define __GLOBAL_ALLOCATOR_HPP__

#include <iostream>
#include <glog/logging.h>

#include <boost/scoped_ptr.hpp>


#include "Allocator.hpp"

#include "DelegateBase.hpp"

class GlobalAllocator;
extern GlobalAllocator * global_allocator;

/// Global memory allocator
class GlobalAllocator {
private:
  boost::scoped_ptr< Allocator > a_p_;

  /// allocate some number of bytes from local heap
  /// (should be called only on node responsible for allocator)
  GlobalAddress< void > local_malloc( size_t size ) {
    intptr_t address = reinterpret_cast< intptr_t >( a_p_->malloc( size ) );
    GlobalAddress< void > ga = GlobalAddress< void >::Raw( address );
    return ga;
  }

  // // allocate some number of some type
  // template< typename T >
  // GlobalAddress< T > local_malloc( size_t count ) {
  //   intptr_t address = reinterpret_cast< intptr_t >( a->malloc( sizeof( T ) * count ) );
  //   GlobalAddress< T > ga = GlobalAddress< T >::Raw( address );
  //   return ga;
  // }

  // release data at pointer in local heap
  /// (should be called only on node responsible for allocator)
  void local_free( GlobalAddress< void > address ) {
    void * va = reinterpret_cast< void * >( address.raw_bits() );
    a_p_->free( va );
  }


public:
  /// Construct global allocator. Allocates no storage, just controls
  /// ownership of memory region.
  ///   @param base base address of region to allocate from
  ///   @param size number of bytes available for allocation
  GlobalAllocator( GlobalAddress< void > base, size_t size )
    : a_p_( 0 == Grappa::mycore()  // node 0 does all allocation for now
            ? new Allocator( base, size )
            : NULL )
  { 
    // TODO: this won't work with pools....
    assert( !global_allocator );
    global_allocator = this;
  }

  //
  // basic operations
  //

  /// delegate malloc
  static GlobalAddress< void > remote_malloc( size_t size_bytes ) {
    // ask node 0 to allocate memory
    auto allocated_address = Grappa::impl::call( 0, [size_bytes] {
        DVLOG(5) << "got malloc request for size " << size_bytes;
        GlobalAddress< void > a = global_allocator->local_malloc( size_bytes );
        DVLOG(5) << "malloc returning pointer " << a.pointer();
        return a;
      });
    return allocated_address;
  }

  /// delegate free
  /// TODO: should free block?
  static void remote_free( GlobalAddress< void > address ) {
    // ask node 0 to free memory
    auto allocated_address = Grappa::impl::call( 0, [address] {
        DVLOG(5) << "got free request for descriptor " << address;
        global_allocator->local_free( address );
        return true;
      });
  }

  //
  // debugging
  //

  /// human-readable allocator state (not to be called directly---called by 'operator<<' overload)
  std::ostream& dump( std::ostream& o ) const {
    if( a_p_ ) {
      return o << "{GlobalAllocator: " << *a_p_ << "}";
    } else {
      return o << "{delegated GlobalAllocator}";
    }
  }

  /// Number of bytes available for allocation;
  size_t total_bytes() const { return a_p_->total_bytes(); }
  /// Number of bytes allocated
  size_t total_bytes_in_use() const { return a_p_->total_bytes_in_use(); }

};

std::ostream& operator<<( std::ostream& o, const GlobalAllocator& a );

/// @addtogroup Memory
/// @{

namespace Grappa {

/// Allocate bytes from the global shared heap.
template< typename T = int8_t >
GlobalAddress<T> global_alloc(size_t count) {
  CHECK_GT(count, 0) << "allocation must be greater than 0";
  return static_cast<GlobalAddress<T>>(GlobalAllocator::remote_malloc(sizeof(T)*count));
}

/// Free memory allocated from global shared heap.
template< typename T >
void global_free(GlobalAddress<T> address) {
  GlobalAllocator::remote_free(static_cast<GlobalAddress<void>>(address));
}

/// Allocate space for a T at the same localizable global address on all cores 
/// (must currently round up to a multiple of block_size plus an additional block 
/// to ensure there is a valid address range no matter which core allocation starts on).
template< typename T, Core MASTER_CORE = 0 >
GlobalAddress<T> symmetric_global_alloc() {
  static_assert(sizeof(T) % block_size == 0,
                "must pad global proxy to multiple of block_size, or use GRAPPA_BLOCK_ALIGNED");
  // allocate enough space that we are guaranteed to get one on each core at same location
  auto qac = global_alloc<char>(cores()*(sizeof(T)+block_size));
  while (qac.core() != MASTER_CORE) qac++;
  auto qa = static_cast<GlobalAddress<T>>(qac);
  CHECK_EQ(qa, qa.block_min());
  CHECK_EQ(qa.core(), MASTER_CORE);
  return qa;
}

} // namespace Grappa

/// Use this macro to allocate space for a variable at the same
/// address on all cores.
///
/// NOTE: the variable declared will be zero-initialized on all
/// cores. Assiging a nonzero initial value is not guaranteed to work;
/// constructors are not guaranteed to run. You should initialize the
/// variable yourself the first time you use it, or with an
/// on_all_cores().
#define symmetric_static static


/// @}

#endif
