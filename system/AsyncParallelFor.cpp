
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "AsyncParallelFor.hpp"

/// When recursively breaking up for loop iterations, stop breaking it up and creating 
/// new tasks when this threshold is reached.
DEFINE_int64( async_par_for_threshold, 1, "Serialization amount for asynchronous parallel loops" );

