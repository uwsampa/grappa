#include "Collective.hpp"

template <typename T>
class Reduction {
  private:
    T localSum;
    bool finished;
    const T init;

  public:
    Reduction(T init) : init(init) {}

    void reset() {
      finished = false;
      localSum = init;
    }

    void add(T val) {
      localSum += val;
    } 

    /// Finish the reduction and return the final value.
    /// This must not be called until all corresponding
    /// local reduction objects have been finished and
    /// synchronized externally.
    ///
    /// ALLNODES
    T finish() {
      if (!finished) {
        finished = true;
        /* T must have operator+ defined */
        //localSum = Grappa_allreduce<T,coll_add<T>,init>(localSum);
        //TODO init version and specialized version for non-template allowed
        localSum = Grappa_allreduce_noinit<T,coll_add<T> > (localSum);
      }
      return localSum;
    }
};
