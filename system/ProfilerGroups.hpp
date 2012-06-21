#ifndef __PROFILER_GROUPS_HPP__
#define __PROFILER_GROUPS_HPP__


// define Tau groups for Grappa

extern const int num_profile_groups;
extern char * profile_group_names[]; 
extern int profile_groups[]; 

#include "ProfilerConfig.hpp"

void generate_profile_groups();


#endif
