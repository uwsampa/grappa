#include "Future.hpp"

uint64_t global_futures_num_touched = 0;
uint64_t global_futures_num_local_started = 0;
uint64_t global_futures_num_remote_started = 0;
uint64_t global_futures_num_local_dq = 0;
uint64_t global_futures_num_remote_dq = 0;

void futures_reset_stats() {
    uint64_t global_futures_num_touched = 0;
    uint64_t global_futures_num_local_started = 0;
    uint64_t global_futures_num_remote_started = 0;
    uint64_t global_futures_num_local_dq = 0;
    uint64_t global_futures_num_remote_dq = 0;
}

void futures_dump_stats() {
    LOG(INFO) << "Futures {" 
        << "futures_num_touched: " << global_futures_num_touched
        << ", futures_num_local_started: " << global_futures_num_local_started
        << ", futures_num_remote_started: " << global_futures_num_remote_started
        << ", futures_num_local_dq: " << global_futures_num_local_dq
        << ", futures_num_remote_dq: " << global_futures_num_remote_dq
    << "}";
}
