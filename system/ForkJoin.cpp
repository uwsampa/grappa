
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "ForkJoin.hpp"

DEFINE_int64(max_forkjoin_threads_per_node, 1024, "maximum number of threads to spawn for a fork-join region");

LocalTaskJoiner ljoin;

//std::map<uint64_t,packed_pair> unpacking_map;

