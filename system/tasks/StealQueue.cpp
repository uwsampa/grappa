#include "StealQueue.hpp"
#include "DictOut.hpp"


StealStatistics::StealStatistics() {
  reset();
}

void StealStatistics::reset() {
  stealq_reply_messages = 0;
  stealq_reply_total_bytes = 0;
  stealq_request_messages = 0;
  stealq_request_total_bytes = 0;
}
            
void StealStatistics::record_steal_reply( size_t msg_bytes ) {
  stealq_reply_messages += 1;
  stealq_reply_total_bytes += msg_bytes;
}

void StealStatistics::record_steal_request( size_t msg_bytes ) {
  stealq_request_messages += 1;
  stealq_request_total_bytes += msg_bytes;
}
            
void StealStatistics::dump() {
  DictOut dout;
  DICT_ADD( dout, stealq_reply_messages );
  DICT_ADD( dout, stealq_reply_total_bytes );
  DICT_ADD( dout, stealq_request_messages );
  DICT_ADD( dout, stealq_request_total_bytes );

  std::cout << "StealStatistics " << dout.toString() << std::endl;
}

void StealStatistics::merge( const StealStatistics * other ) {
  stealq_reply_messages += other->stealq_reply_messages;
  stealq_reply_total_bytes += other->stealq_reply_total_bytes;
  stealq_request_messages += other->stealq_request_messages;
  stealq_request_total_bytes += other->stealq_request_total_bytes;
}

StealStatistics StealStatistics::reduce( const StealStatistics& a, const StealStatistics& b ) {
  StealStatistics newst = a;
  newst.merge(&b);
  return newst;
}
