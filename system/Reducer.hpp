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
#pragma once
#include <Collective.hpp>

/// 
/// A Reducer object encapsulates a reduction
/// during the computation to a single location
///
/// Warning: may only use one object of type AllReducer<T, ReduceOp>
/// concurrently because relies on `Grappa::allreduce`,
/// which has no way of tagging messages
///
template <typename T, T (*ReduceOp)(const T&, const T&)>
class AllReducer {
  private:
    T localSum;
    bool finished;
    const T init;

  public:
    AllReducer(T init) : init(init) {}

    void reset() {
      finished = false;
      localSum = init;
    }

    void accumulate(T val) {
      localSum = ReduceOp(localSum, val);
    } 

    /// Finish the reduction and return the final value.
    /// This must not be called until all corresponding
    /// local reduction objects have been finished and
    /// synchronized externally.
    ///
    /// ALLCORES
    T finish() {
      if (!finished) {
        finished = true;
        //TODO init version and specialized version for non-template allowed
        localSum = Grappa::allreduce<T,ReduceOp> (localSum);
      }
      return localSum;
    }


    //TODO, a non collective call to finish would be nice
    //any such scheme will require knowing where each participant's
    //value lives (e.g. process-global variable)
};

namespace Grappa {

  /// @brief Symmetric Reduction object
  template< typename T >
  class SimpleSymmetric {
    T local_value;
    // GlobalAddress<SimpleSymmetric> self;
  public:
    SimpleSymmetric(): local_value() {}
  
    T& local() { return local_value; }
    const T& local() const { return local_value; }
  
    // static GlobalAddress<SimpleSymmetric> create() {
    //   auto s = symmetric_global_alloc<SimpleSymmetric>();
    //   call_on_all_cores([s]{
    //     s->self = s;
    //   });
    // }
  
    friend T all(SimpleSymmetric * r) {
      return reduce<T,collective_and>(&r->local_value);
    }
    friend T any(SimpleSymmetric * r) {
      return reduce<T,collective_or >(&r->local_value);
    }
    friend T sum(SimpleSymmetric * r) {
      return reduce<T,collective_add>(&r->local_value);
    }
    friend void set(SimpleSymmetric * r, const T& val) {
      call_on_all_cores([=]{ r->local_value = val; });
    }

    friend T all(SimpleSymmetric& r) { return all(&r); }
    friend T any(SimpleSymmetric& r) { return any(&r); }
    friend T sum(SimpleSymmetric& r) { return sum(&r); }
    friend void set(SimpleSymmetric& r, const T& val) { return set(&r, val); }
  
    void operator=(const T& val) { local() = val; }
    void operator+=(const T& val) { local() += val; }
    void operator++() { local()++; }
    void operator++(int) { local()++; }
    
  } GRAPPA_BLOCK_ALIGNED;

} // namespace Grappa