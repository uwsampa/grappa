// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __PROFILER_GROUPS_HPP__
#define __PROFILER_GROUPS_HPP__


// define Tau groups for Grappa

extern const int num_profile_groups;
extern char * profile_group_names[]; 
extern int profile_groups[]; 

#include "ProfilerConfig.hpp"

void generate_profile_groups();


#endif
