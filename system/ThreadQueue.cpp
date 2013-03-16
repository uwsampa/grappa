#include "ThreadQueue.hpp"


//FIXME: rounrobin print
/// ThreadQueue output stream
std::ostream& operator<< ( std::ostream& o, const ThreadQueue& tq ) {
    return tq.dump( o );
}

