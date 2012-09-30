// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Future.hpp"

// TODO: stats class
// initialize futures stats
uint64_t global_futures_num_touched = 0;
uint64_t global_futures_num_local_started = 0;
uint64_t global_futures_num_remote_started = 0;
uint64_t global_futures_num_local_dq = 0;
uint64_t global_futures_num_remote_dq = 0;

// reset futures stats
void futures_reset_stats() {
    uint64_t global_futures_num_touched = 0;
    uint64_t global_futures_num_local_started = 0;
    uint64_t global_futures_num_remote_started = 0;
    uint64_t global_futures_num_local_dq = 0;
    uint64_t global_futures_num_remote_dq = 0;
}

// dump futures stats as dictionary
void futures_dump_stats() {
    LOG(INFO) << "Futures {" 
        << "futures_num_touched: " << global_futures_num_touched
        << ", futures_num_local_started: " << global_futures_num_local_started
        << ", futures_num_remote_started: " << global_futures_num_remote_started
        << ", futures_num_local_dq: " << global_futures_num_local_dq
        << ", futures_num_remote_dq: " << global_futures_num_remote_dq
    << "}";
}
