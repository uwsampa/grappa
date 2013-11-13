#include "StealQueue.hpp"
#include "Statistics.hpp"
#include "DictOut.hpp"


/* metrics */

// work steal network usage
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, stealq_reply_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, stealq_reply_total_bytes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, stealq_request_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, stealq_request_total_bytes, 0);

// work share network usage 
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_request_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_request_total_bytes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_reply_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_reply_total_bytes, 0);

// work share elements transfered
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, workshare_request_elements_denied, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, workshare_request_elements_received, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, workshare_reply_elements_sent, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_requests_client_smaller, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_requests_client_larger, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, workshare_reply_nacks, 0);

// global queue data transfer network usage
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_data_pull_request_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_data_pull_request_total_bytes, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_data_pull_reply_messages, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, globalq_data_pull_reply_total_bytes, 0);

// global queue elements transfered
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, globalq_data_pull_request_num_elements, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<uint64_t>, globalq_data_pull_reply_num_elements, 0);


namespace Grappa {


void StealQueue::record_steal_reply( size_t msg_bytes ) {
  stealq_reply_messages += 1;
  stealq_reply_total_bytes += msg_bytes;
}

void StealQueue::record_steal_request( size_t msg_bytes ) {
  stealq_request_messages += 1;
  stealq_request_total_bytes += msg_bytes;
}

void StealQueue::record_workshare_request( size_t msg_bytes ) {
  workshare_request_messages += 1;
  workshare_request_total_bytes += msg_bytes;
}

void StealQueue::record_workshare_reply( size_t msg_bytes, bool isAccepted, int num_received, int num_denying, int num_sending ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  if ( isAccepted ) {
    workshare_requests_client_smaller += 1;
  } else {
    workshare_requests_client_larger += 1;
  }

  workshare_request_elements_received+=( num_received );
  workshare_request_elements_denied+=( num_denying );
  workshare_reply_elements_sent+=( num_sending );
}

void StealQueue::record_workshare_reply_nack( size_t msg_bytes ) {
  workshare_reply_messages += 1;
  workshare_reply_total_bytes += msg_bytes;

  workshare_reply_nacks += 1;
}

void StealQueue::record_globalq_data_pull_request( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_request_messages += 1;
  globalq_data_pull_request_total_bytes += msg_bytes;

  globalq_data_pull_request_num_elements+=amount ;
}

void StealQueue::record_globalq_data_pull_reply( size_t msg_bytes, uint64_t amount ) {
  globalq_data_pull_reply_messages += 1;
  globalq_data_pull_reply_total_bytes += msg_bytes;

  globalq_data_pull_reply_num_elements +=  amount ;
}


} // namespace Grappa
