// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "ProfilerGroups.hpp"
#include <glog/logging.h>

#ifdef GRAPPA_TRACE
#include <TAU.h>
#endif

const int num_profile_groups = 8;

char * profile_group_names[] = { 
    (char*) "GRAPPA_COMM_GROUP",
    (char*) "GRAPPA_LATENCY_GROUP",
    (char*) "GRAPPA_USER_GROUP",
    (char*) "GRAPPA_USERAM_GROUP",
    (char*) "GRAPPA_SCHEDULER_GROUP",
    (char*) "GRAPPA_SUSPEND_GROUP",
    (char*) "GRAPPA_TASK_GROUP",
    (char*) "GRAPPA_STATE_TIMER_GROUP" };

int profile_groups[num_profile_groups] = { -11, -11, -11, -11, -11, -11, -11, -11 };

void generate_profile_groups() {
#ifdef GRAPPA_TRACE
    CHECK( sizeof(profile_group_names)/sizeof(char*) == num_profile_groups );
    for (int i=0; i<num_profile_groups; i++) {
        profile_groups[i] = TAU_GET_PROFILE_GROUP( profile_group_names[i] ) ;
    }
#endif
}
