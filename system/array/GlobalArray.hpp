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
template< typename T, typename INDEX, typename... D1toN  >
class GlobalArray;


namespace impl {

// forward declaration for GlobalArray element proxy object
template< typename T, typename INDEX, typename... D1toN >
class ArrayDereferenceProxy;

// GlobalArray element proxy object base case. This overloads the &
// and cast operators to allow use as a value.
template< typename T, typename INDEX >
class ArrayDereferenceProxy<T,INDEX> {
protected:
  GlobalAddress<T> ga;

public:
  ArrayDereferenceProxy( ): ga() { }
  ArrayDereferenceProxy(GlobalAddress<T> base, INDEX percore, INDEX size)
    : ga(base)
  { }

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
template< typename T, typename INDEX, typename D1, typename... D2toN >
class ArrayDereferenceProxy<T,INDEX,D1,D2toN...> {
protected:
  GlobalAddress<T> ga;
  INDEX dim2_percore;
  INDEX dim2_size;
  
public:
  ArrayDereferenceProxy( ): ga(), dim2_percore(), dim2_size() { }
  ArrayDereferenceProxy(GlobalAddress<T> base, INDEX percore, INDEX size)
    : ga(base)
    , dim2_percore(percore)
    , dim2_size(size)
  { }

  ArrayDereferenceProxy<T,INDEX,D2toN...> operator[]( INDEX i ) {
    auto p = ga;
    auto base_pointer = ga.pointer();
    auto base_core = ga.core();
    if( sizeof...(D2toN) != 0 ) { // first dimension
      INDEX dim = i * dim2_percore;  // which element on this core is it?
      p = make_linear( base_pointer + dim, base_core );
    } else { // second dimension
      INDEX core = i / dim2_percore; // which core is this element on?
      INDEX dim = i % dim2_percore;  // which element on this core is it?
      p = make_linear( base_pointer + dim, base_core + core );
    }
    return ArrayDereferenceProxy<T,INDEX,D2toN...>(p,dim2_percore,dim2_size);
  }
};

}




// placeholder for more general global array
template< typename T, typename INDEX, typename... D1toN  >
class GlobalArray {
  static_assert(impl::templateStaticAssertFalse<T>(),
                "GlobalArray with these parameters not supported yet.");
};




// specialization of global 2D array block-distributed in first dimension
template< typename T, typename INDEX >
class GlobalArray< T, INDEX, Distribution::Local, Distribution::Block > 
  : public ArrayDereferenceProxy< T, INDEX, Distribution::Local, Distribution::Block > {
private:
  GlobalAddress<char> block;
  GlobalAddress<T> & base;

public:

  T * local_chunk;

  INDEX & dim2_percore;
  INDEX & dim2_size;

  INDEX dim1_size;

public:
  typedef T Type;
  
  GlobalArray()
    : block()
    , base(ArrayDereferenceProxy< T, INDEX, Distribution::Local, Distribution::Block >::ga)
    , local_chunk(NULL)
    , dim2_percore(ArrayDereferenceProxy< T, INDEX, Distribution::Local, Distribution::Block >::dim2_percore)
    , dim2_size(ArrayDereferenceProxy< T, INDEX, Distribution::Local, Distribution::Block >::dim2_size)
    , dim1_size(0)
  { }
  
  void allocate( INDEX m, INDEX n ) {
    INDEX cores = Grappa::cores();
    INDEX n_percore = n / cores + ((n % cores) != 0);

    // TODO: once we've fixed messaging, replace this with better symmetric allocation.
    // allocate one extra block per core so we can have a consistent base address
    size_t bytes_percore = m * n_percore * sizeof(T);
    size_t block_count = (bytes_percore + block_size - 1) / block_size;
    size_t byte_count = cores * ((block_count + 1) * block_size);
    block = global_alloc<char>( byte_count );

    // find base that's valid everywhere
    auto bb = block;
    const Core MASTER_CORE = 0;
    while (bb.core() != MASTER_CORE) bb++;

    // now make it a pointer of the right type
    auto b = static_cast< GlobalAddress<T> >( bb );

    on_all_cores( [this,m,n,n_percore,b] {
        CHECK_NULL( local_chunk );
        dim1_size = m; 
        dim2_size = n;
        dim2_percore = n_percore;
        base = b;
        local_chunk = b.localize();
        VLOG(2) << "base is " << local_chunk;
      } );
  }

  void deallocate( ) {
    if( block ) global_free( block );
    if( local_chunk ) local_chunk = NULL;
   }

  template< typename F, GlobalCompletionEvent * GCE = &impl::local_gce>
  void forall( F body ) {
    Core origin = mycore();
    if (GCE) GCE->enroll(Grappa::cores());
    on_all_cores( [this,body,origin] {
        T * elem = local_chunk;
        INDEX first_j = dim2_percore * mycore();
        for( INDEX i = 0; i < dim1_size; ++i ) {
          elem = local_chunk + i * dim2_percore;
          for( INDEX j = first_j; (j < first_j + dim2_percore) && (j < dim2_size); ++j ) {
            body(i, j, *elem );
            elem++;
          }
        }
        if (GCE) complete(make_global(GCE,origin));
      } );
    if (GCE) GCE->wait();
    }
    
  template< typename F, GlobalCompletionEvent * GCE = &impl::local_gce>
  void forallx( F body ) {
    Core origin = mycore();
    if (GCE) GCE->enroll(Grappa::cores());
    on_all_cores( [this,body,origin] {
        T * base = local_chunk;
        INDEX first_j = dim2_percore * mycore();
        forall_here<async,GCE,1>( 0, dim1_size, [this,first_j,body] (int64_t start, int64_t iters) {
            T * elem = local_chunk + start * dim2_percore;
            for( INDEX j = first_j; (j < first_j + dim2_percore) && (j < dim2_size); ++j ) {
              body(start, j, *elem );
              elem++;
            }
          } );
        if (GCE) complete(make_global(GCE,origin));
      } );
    if (GCE) GCE->wait();
    }
    
};

/// Parallel iteration over GlobalArray
template< GlobalCompletionEvent * C = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T, typename INDEX, typename D1, typename... D2toN,
          typename F = nullptr_t >
void forall(GlobalAddress<GlobalArray<T,INDEX,D1,D2toN...>> arr, F loop_body) {
  arr->forall( loop_body );
}
template< GlobalCompletionEvent * C = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T, typename INDEX, typename D1, typename... D2toN,
          typename F = nullptr_t >
void forall(GlobalArray<T,INDEX,D1,D2toN...>& arr, F loop_body) {
  arr.forall( loop_body );
}

} // namespace Grappa
