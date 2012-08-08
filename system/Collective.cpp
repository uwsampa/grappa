#include <cassert>
#include "Collective.hpp"
#include "Delegate.hpp"

Thread * reducing_thread;
Node reduction_reported_in = 0;

