#pragma once
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

#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "Cache.hpp"
#include "GlobalCompletionEvent.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include <type_traits>
#include "Delegate.hpp"
#include <common.hpp>

#include <array/Distributions.hpp>

#include <type_traits>

namespace Grappa {

// forward declaration for global array
template< typename T, typename... D1toN  >
class GlobalArray;


namespace impl {

// forward declaration for GlobalArray element proxy object
template< typename T, typename... D1toN >
class ArrayDereferenceProxy;

// GlobalArray element proxy object base case. This overloads the &
// and cast operators to allow use as a value.
template< typename T >
class ArrayDereferenceProxy<T> {
protected:
  GlobalAddress<T> ga;

public:
  ArrayDereferenceProxy( ): ga() { }
  ArrayDereferenceProxy(GlobalAddress<T> ga, size_t percore, size_t size): ga(ga) { }
  //void set(GlobalAddress<T> ga_arg) { ga = ga_arg; }

  // return global address instead of local address
  GlobalAddress<T> operator&() {
    LOG(INFO) << "Returning address " << ga;
    return ga;
  }

  // allow local access to array element through reference
  operator T&() {
    return *ga.pointer();
  }
};

// GlobalArray element proxy object recursive case. This overloads the &
// and cast operators to allow use as a value.
template< typename T, typename D1, typename... D2toN >
class ArrayDereferenceProxy<T,D1,D2toN...> {
protected:
  GlobalAddress<T> ga;
  size_t dim2_percore;
  size_t dim2_size;
  
public:
  ArrayDereferenceProxy( ): ga(), dim2_percore(), dim2_size() { }
  ArrayDereferenceProxy(GlobalAddress<T> ga, size_t percore, size_t size): ga(ga), dim2_percore(percore), dim2_size(size) { }

  //template< typename N >
  //ArrayDereferenceProxy<T,D2toN...> operator[]( N index ) {
  //size_t i = static_cast< size_t >( index );
  ArrayDereferenceProxy<T,D2toN...> operator[]( size_t i ) {
    LOG(INFO) << "Depth " << sizeof...(D2toN) << " Indexing with index " << i;
    Core c = ga.core();
    T * p = ga.pointer();
    if( sizeof...(D2toN) == 0 ) {
      size_t core_offset = i / dim2_percore;
      size_t dim_offset = i % dim2_percore;
      LOG(INFO) << "Index " << i << " base: " << ga << " core_offset: " << core_offset << " dim_offset: " << dim_offset;
      c += core_offset;
      p += dim_offset;
    } else {
      LOG(INFO) << "Index " << i << " base: " << ga << " dim_offset: " << i * dim2_percore;
      p += i * dim2_percore;
    }
    return ArrayDereferenceProxy<T,D2toN...>(make_global(p,c),dim2_percore,dim2_size);
  }
};

}




// placeholder for more general global array
template< typename T, typename... D1toN  >
class GlobalArray {
  static_assert(impl::templateStaticAssertFalse<T>(),
                "GlobalArray with these parameters not supported yet.");
};




// specialization of global 2D array block-distributed in first dimension
template< typename T >
class GlobalArray< T, Distribution::Local, Distribution::Block > 
  : public ArrayDereferenceProxy< T, Distribution::Local, Distribution::Block > {
private:
  GlobalAddress<char> block;
  GlobalAddress<T> & base;
  size_t & dim2_percore;
  size_t & dim2_size;

  T * local_chunk;
  size_t dim1_size;

  size_t dim1_max;

public:
  typedef T Type;
  
  GlobalArray()
    : block()
    , base(ArrayDereferenceProxy< T, Distribution::Local, Distribution::Block >::ga)
    , dim2_percore(ArrayDereferenceProxy< T, Distribution::Local, Distribution::Block >::dim2_percore)
    , dim2_size(ArrayDereferenceProxy< T, Distribution::Local, Distribution::Block >::dim2_size)
    , local_chunk(NULL)
    , dim1_size(0)
    , dim1_max(0)
  { }
  
  void allocate( size_t x, size_t y ) {
    size_t cores = Grappa::cores();
    size_t corrected_x = x;
    size_t corrected_y = y + ((y % cores) != 0) * cores;
    size_t y_percore2 = y / cores;
    size_t y_percore = y / cores + ((y % cores) != 0);

    VLOG(2) << "x: " << x << " / " << corrected_x << ", "
            << "y: " << y << " / " << corrected_y
            << " percore: " << y_percore2 << " / " << y_percore;

    // TODO: once we've fixed messaging, replace this with better symmetric allocation.
    // allocate one extra block per core so we can have a consistent base address
    size_t bytes_percore = x * y_percore * sizeof(T);
    size_t block_count = (bytes_percore + block_size - 1) / block_size;
    size_t byte_count = cores * ((block_count + 1) * block_size);
    LOG(INFO) << "bytes_percore: " << bytes_percore
              << " block_count: " << block_count
              << " byte_count: " << byte_count;
    block = global_alloc<char>( byte_count );

    // find base that's valid everywhere
    auto bb = block;
    const Core MASTER_CORE = 0;
    while (bb.core() != MASTER_CORE) bb++;

    // now make it a pointer of the right type
    auto b = static_cast< GlobalAddress<T> >( bb );


    on_all_cores( [this,x,y,corrected_y,y_percore,b] {
        CHECK_NULL( local_chunk );
        dim1_size = x; 
        dim2_size = y;
        dim2_percore = y_percore;

        VLOG(2) << "per core: " << dim2_percore;
        
        base = make_global( b.pointer(), b.core() );
        local_chunk = b.localize();
        auto end = (b + (x * y_percore)).localize();
        size_t cs = end - local_chunk;
        VLOG(2) << "Chunk at " << local_chunk << " size is " << dim2_percore * y << " / " << cs;
      } );
  }

  void deallocate( ) {
    if( block ) global_free( block );
    if( local_chunk ) local_chunk = NULL;
  }

  template< typename F >
  void forall( F body ) {
    on_all_cores( [this,body] {
        T * elem = local_chunk;
        LOG(INFO) << "Base: " << elem << " percore: " << dim2_percore;
        size_t first_j = dim2_percore * mycore();
        LOG(INFO) << "first_j: " << first_j 
                  << " last_j: " << first_j + dim2_percore
                  << " max_j: " << dim2_size;
        for( size_t i = 0; i < dim1_size; ++i ) {
          elem = local_chunk + i * dim2_percore;
          for( size_t j = first_j; (j < first_j + dim2_percore) && (j < dim2_size); ++j ) {
            LOG(INFO) << "At " << i << ", " << j << ": " << elem;
            body(i, j, *elem );
            elem++;
          }
        }
      } );
  }
    
};

/// Parallel iteration over GlobalArray
template< GlobalCompletionEvent * C = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T, typename D1, typename... D2toN,
          typename F = nullptr_t >
void forall(GlobalAddress<GlobalArray<T,D1,D2toN...>> arr, F loop_body) {
  arr->forall( loop_body );
}
template< GlobalCompletionEvent * C = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T, typename D1, typename... D2toN,
          typename F = nullptr_t >
void forall(GlobalArray<T,D1,D2toN...>& arr, F loop_body) {
  arr.forall( loop_body );
}

} // namespace Grappa
