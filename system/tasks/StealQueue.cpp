#include "StealQueue.hpp"
#include "DictOut.hpp"

StealStatistics steal_queue_stats;

StealStatistics::StealStatistics() {
  reset();
}

void StealStatistics::reset() {
  stealq_reply_messages = 0;
  stealq_reply_total_bytes = 0;
  stealq_request_messages = 0;
  stealq_request_total_bytes = 0;

  workshare_request_messages = 0;
  workshare_request_total_bytes = 0;
  workshare_reply_messages = 0;
  workshare_reply_total_bytes = 0;

  workshare_request_elements_denied.reset();
  workshare_request_elements_received.reset();
  workshare_reply_elements_sent.reset();
  workshare_requests_client_smaller = 0;
  workshare_requests_client_larger = 0;
  workshare_reply_nacks = 0;

  globalq_data_pull_request_messages = 0;
  globalq_data_pull_request_total_bytes = 0;
  globalq_data_pull_reply_messages = 0;
  globalq_data_pull_reply_total_bytes = 0;

  globalq_data_pull_request_num_elements.reset();
  globalq_data_pull_reply_num_elements.reset();
}
            
void StealStatistics::record_steal_reply( size_t msg_bytes ) {
  stealq_reply_messages += 1;
  stealq_reply_total_bytes += msg_bytes;
}

void StealStatistics::record_steal_request( size_t msg_bytes ) {
  stealq_request_messages += 1;
  stealq_request_total_bytes += msg_bytes;
}
            
void StealStatistics::record_workshare_request( size_t msg_bytes ) {
  workshare_request_messages += 1;
  workshare_request_total_bytes += msg_bytes;
}

void StealStatistics::record_workshare_reply( size_t msg_bytes, bool isAccepted, int num_received, int num_denying, int num_sending ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  if ( isAccepted ) {
    workshare_requests_client_smaller += 1;
  } else {
    workshare_requests_client_larger += 1;
  }

  workshare_request_elements_received.update( num_received );
  workshare_request_elements_denied.update( num_denying );
  workshare_reply_elements_sent.update( num_sending );
}

void StealStatistics::record_workshare_reply_nack( size_t msg_bytes ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  workshare_reply_nacks += 1;
}

void StealStatistics::record_globalq_data_pull_request( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_request_messages += 1;
  globalq_data_pull_request_total_bytes += msg_bytes;

  globalq_data_pull_request_num_elements.update( amount );
}

void StealStatistics::record_globalq_data_pull_reply( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_reply_messages += 1;
  globalq_data_pull_reply_total_bytes += msg_bytes;

  globalq_data_pull_reply_num_elements.update( amount );
}
  

void StealStatistics::dump() {
  DictOut dout;
  DICT_ADD( dout, stealq_reply_messages );
  DICT_ADD( dout, stealq_reply_total_bytes );
  DICT_ADD( dout, stealq_request_messages );
  DICT_ADD( dout, stealq_request_total_bytes );

  DICT_ADD( dout, workshare_request_messages );
  DICT_ADD( dout, workshare_request_total_bytes );
  DICT_ADD( dout, workshare_reply_messages );
  DICT_ADD( dout, workshare_reply_total_bytes );

  DICT_ADD_STAT_TOTAL( dout, workshare_request_elements_denied );
  DICT_ADD_STAT_TOTAL( dout, workshare_request_elements_received );
  DICT_ADD_STAT_TOTAL( dout, workshare_reply_elements_sent );
  DICT_ADD( dout, workshare_requests_client_smaller );
  DICT_ADD( dout, workshare_requests_client_larger );
  DICT_ADD( dout, workshare_reply_nacks );

  DICT_ADD( dout, globalq_data_pull_request_messages );
  DICT_ADD( dout, globalq_data_pull_request_total_bytes );
  DICT_ADD( dout, globalq_data_pull_reply_messages );
  DICT_ADD( dout, globalq_data_pull_reply_total_bytes );

  DICT_ADD_STAT_TOTAL( dout, globalq_data_pull_request_num_elements );
  DICT_ADD_STAT_TOTAL( dout, globalq_data_pull_reply_num_elements );   

  std::cout << "StealStatistics " << dout.toString() << std::endl;
}

void StealStatistics::merge( const StealStatistics * other ) {
  stealq_reply_messages += other->stealq_reply_messages;
  stealq_reply_total_bytes += other->stealq_reply_total_bytes;
  stealq_request_messages += other->stealq_request_messages;
  stealq_request_total_bytes += other->stealq_request_total_bytes;

  workshare_request_messages   +=   other->workshare_request_messages;
  workshare_request_total_bytes +=   other->workshare_request_total_bytes;
  workshare_reply_messages     +=   other->workshare_reply_messages;
  workshare_reply_total_bytes  +=   other->workshare_reply_total_bytes;

  MERGE_STAT_TOTAL( workshare_request_elements_denied, other );
  MERGE_STAT_TOTAL( workshare_request_elements_received, other );
  MERGE_STAT_TOTAL( workshare_reply_elements_sent, other );
  workshare_requests_client_smaller += other->workshare_requests_client_smaller;
  workshare_requests_client_larger  += other->workshare_requests_client_larger;
  workshare_reply_nacks  += other->workshare_reply_nacks;

  globalq_data_pull_request_messages += other->globalq_data_pull_request_messages;
  globalq_data_pull_request_total_bytes += other->globalq_data_pull_request_total_bytes;
  globalq_data_pull_reply_messages += other->globalq_data_pull_reply_messages;
  globalq_data_pull_reply_total_bytes += other->globalq_data_pull_reply_total_bytes;

  MERGE_STAT_TOTAL( globalq_data_pull_request_num_elements, other );
  MERGE_STAT_TOTAL( globalq_data_pull_reply_num_elements, other );
}

void StealStatistics::profiling_sample() {
}
