// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <cassert>
#include "Collective.hpp"
#include "Delegate.hpp"

Thread * reducing_thread;
Node reduction_reported_in = 0;

