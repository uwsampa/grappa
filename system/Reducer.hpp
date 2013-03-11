
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef REDUCER_HPP
#define REDUCER_HPP

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

#endif // REDUCER_HPP
