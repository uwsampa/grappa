#include "ProfilerGroups.hpp"
#include <glog/logging.h>
#include <TAU.h>

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
