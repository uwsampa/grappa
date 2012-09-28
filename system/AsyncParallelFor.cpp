#include "AsyncParallelFor.hpp"

/// When recursively breaking up for loop iterations, stop breaking it up and creating 
/// new tasks when this threshold is reached.
DEFINE_int64( async_par_for_threshold, 1, "Serialization amount for asynchronous parallel loops" );

