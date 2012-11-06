// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef PROFILER_CONFIG_HPP
#define PROFILER_CONFIG_HPP

// Define profiling groups
// To add a new profiling group also see ProfilerGroups.cpp
#define GRAPPA_COMM_GROUP profile_groups[0]
#define GRAPPA_LATENCY_GROUP profile_groups[1]
#define GRAPPA_USER_GROUP profile_groups[2]
#define GRAPPA_USERAM_GROUP profile_groups[3]
#define GRAPPA_SCHEDULER_GROUP profile_groups[4]
#define GRAPPA_SUSPEND_GROUP profile_groups[5]
#define GRAPPA_TASK_GROUP profile_groups[6]
#define GRAPPA_STATE_TIMER_GROUP profile_groups[7]

#endif // PROFILER_CONFIG_HPP
