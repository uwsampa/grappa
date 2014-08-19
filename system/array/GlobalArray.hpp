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

#include <type_traits>

namespace Grappa {

namespace Distribution {

class Block {
// public:
  // static size_t core_for_index( size_t index, size_t total ) {
  //   return 0;
  // }
};

class Cyclic { };

class Local { };

}


namespace impl {

// Helper function to make it possible to use static assert to throw
// an error only if a template is instantiated.
template<typename T> 
constexpr bool templateStaticAssertFalse() {
  return false;
}



// forward declaration for GlobalArray element proxy object
template< typename T, typename... D1toN >
class ArrayDereferenceProxy;

// GlobalArray element proxy object ase case. This overloads the & and
// cast operators to allow use as a value.
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

template< typename T, typename D1, typename... D2toN >
class ArrayDereferenceProxy<T,D1,D2toN...> {
protected:
  GlobalAddress<T> ga;

public:
  ArrayDereferenceProxy( ): ga() { }
  ArrayDereferenceProxy(GlobalAddress<T> ga): ga(ga) {}

  ArrayDereferenceProxy<T,D2toN...> operator[]( size_t i ) {
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
    on_all_cores( [this,x,y] {
        CHECK_NULL( local_chunk );

        chunk_size = x / Grappa::cores();
        if( Grappa::mycore() < x % Grappa::cores() ) chunk_size++;
        LOG(INFO) << "Chunk size is " << chunk_size;
      } );
  }

  void deallocate( ) {
    if( base ) global_free( base );
    if( local_chunk ) local_chunk = NULL;
  }

};


// namespace impl {

// /// Parallel, localized iteration over elements of 2D array with their indices
// template< GlobalCompletionEvent * C, int64_t Threshold, typename G, typename F >
// void forall(GlobalAddress<G> g, F body,
//             void (F::*mf)(int64_t,int64_t,typename G::Type&) const) {
//   Grappa::forall<C,Threshold>(g->vs, g->nv,
//                               [body](int64_t x, int64_t y, typename G::Type& d){
                                
//                                 if (v.valid) {
//                                   body(i, v);
//                                 }
//                               });
// }

// }

// /// Parallel iteration over GlobalArray, specialized by argument.
// template< GlobalCompletionEvent * C = &impl::local_gce,
//           int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
//           typename T,
//           typename F = nullptr_t >
// void forall(GlobalAddress<GlobalArray<T>> arr, F loop_body) {
//   impl::forall<C,Threshold>(arr, loop_body, &F::operator());
// }

} // namespace Grappa
