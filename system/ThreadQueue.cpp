#include "ThreadQueue.hpp"


/// ThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq ) {
    return tq.dump( o );
}

/// PrefetchingThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const PrefetchingThreadQueue& tq ) {
  return o << "TODO PRINT IN ROUND ROBIN";
}

