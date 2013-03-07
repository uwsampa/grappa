
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
template <typename T, T (*ReduceOp)(const T&, const T&)>
class Reducer {
  private:
    T localSum;
    bool finished;
    const T init;

  public:
    Reducer(T init) : init(init) {}

    void reset() {
      finished = false;
      localSum = init;
    }

    void accumulate(T val) {
      localSum += val;
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
        /* T must have operator+ defined */
        //localSum = Grappa_allreduce<T,coll_add<T>,init>(localSum);
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
