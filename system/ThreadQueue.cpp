#include "ThreadQueue.hpp"


/// ThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const Grappa::ThreadQueue& tq ) {
    return tq.dump( o );
}

/// PrefetchingThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const Grappa::PrefetchingThreadQueue& tq ) {
  return o << "TODO PRINT IN ROUND ROBIN";
}

