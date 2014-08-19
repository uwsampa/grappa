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
  ArrayDereferenceProxy(GlobalAddress<T> ga): ga(ga) {}
  void set(GlobalAddress<T> ga_arg) { ga = ga_arg; }

  // return global address instead of local address
  GlobalAddress<T> operator&() {
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

public:
  ArrayDereferenceProxy( ): ga() { }
  ArrayDereferenceProxy(GlobalAddress<T> ga): ga(ga) {}

  ArrayDereferenceProxy<T,D2toN...> operator[]( size_t i ) {
    Core c = ga.core();
    T * p = ga.pointer();
    //GlobalAddress<T> offset = 
    return ArrayDereferenceProxy<T,D2toN...>(ga);
  }
};

}




// placeholder for more general global array
template< typename T, typename D1, typename... D2toN  >
class GlobalArray {
  static_assert(impl::templateStaticAssertFalse<T>(),
                "GlobalArray with these parameters not supported yet.");
};




// specialization of global 2D array block-distributed in first dimension
template< typename T >
class GlobalArray< T, Distribution::Block, Distribution::Local > 
  : public ArrayDereferenceProxy< T, Distribution::Block, Distribution::Local > {
private:

  GlobalAddress<T> & base;
  T * local_chunk;
  
  size_t chunk_size;

  Distribution::Block d1;
  Distribution::Local d2;
  
  void collectiveAllocate( size_t x, size_t y ) {
  }
public:
  typedef T Type;
  
  GlobalArray()
    : base(ArrayDereferenceProxy< T, Distribution::Block, Distribution::Local >::ga)
    , local_chunk(NULL)
    , chunk_size()
  { }
  
  void allocate( size_t x, size_t y ) {
    auto b = global_alloc<T>(x*y);
    on_all_cores( [this,x,y,b] {
        CHECK_NULL( local_chunk );

        d2.set( 1, y );
        d1.set( d2.size, x );

        CHECK_EQ( x*y, d1.size );
        chunk_size = x * y;
        base = b;
        local_chunk = b.localize();
        auto end = (b+x*y).localize();
        size_t cs = end - local_chunk;
        LOG(INFO) << "Chunk size is " << chunk_size << " / " << cs;
      } );
  }

  void deallocate( ) {
    if( base ) global_free( base );
    if( local_chunk ) local_chunk = NULL;
  }

  template< typename F >
  void forall( F body ) {
    on_all_cores( [this,body] {
        T * elem = local_chunk;
        size_t first_block = d1.per_core * mycore();
        for( size_t i = first_block; ((i < first_block + d1.per_core) &&
                                      (i < d1.max)); ++i ) {
          for( size_t j = 0; j < d2.per_core; ++j ) {
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
