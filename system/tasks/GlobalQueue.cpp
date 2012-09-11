#include "GlobalQueue.hpp"

GlobalQueueStatistics global_queue_stats;

GlobalQueueStatistics::GlobalQueueStatistics() {
  reset();
}

void GlobalQueueStatistics::profiling_sample() {
  // TODO
}

void GlobalQueueStatistics::reset() {
    globalq_pull_reserve_request_messages = 0;
    globalq_pull_reserve_request_total_bytes = 0;
    
    globalq_pull_reserve_reply_messages = 0;
    globalq_pull_reserve_reply_total_bytes = 0;
    
    globalq_push_reserve_request_messages = 0;
    globalq_push_reserve_request_total_bytes = 0;
    
    globalq_push_entry_request_messages = 0;
    globalq_push_entry_request_total_bytes = 0;
    
    globalq_push_reserve_reply_messages = 0;
    globalq_push_reserve_reply_total_bytes = 0;

    globalq_pull_request_granted = 0;
    globalq_pull_request_denied = 0;
    globalq_pull_entry_request_messages = 0;
    globalq_pull_entry_request_total_bytes = 0;
    globalq_pull_entry_reply_messages = 0;
    globalq_pull_entry_reply_total_bytes = 0;
    
    globalq_push_request_accepted = 0;
    globalq_push_request_rejected = 0;
    globalq_push_entry_hadConsumer = 0;
    globalq_push_entry_noConsumer = 0;
}

#include "DictOut.hpp"
void GlobalQueueStatistics::dump() {
  DictOut dout;
  DICT_ADD( dout, globalq_pull_reserve_request_messages );
  DICT_ADD( dout, globalq_pull_reserve_request_total_bytes );
  
  DICT_ADD( dout, globalq_pull_reserve_reply_messages );
  DICT_ADD( dout, globalq_pull_reserve_reply_total_bytes );
  
  DICT_ADD( dout, globalq_push_reserve_request_messages );
  DICT_ADD( dout, globalq_push_reserve_request_total_bytes );
  
  DICT_ADD( dout, globalq_push_entry_request_messages );
  DICT_ADD( dout, globalq_push_entry_request_total_bytes );
  
  DICT_ADD( dout, globalq_push_reserve_reply_messages );
  DICT_ADD( dout, globalq_push_reserve_reply_total_bytes );

  DICT_ADD( dout, globalq_pull_request_granted );
  DICT_ADD( dout, globalq_pull_request_denied );
  DICT_ADD( dout, globalq_pull_entry_request_messages );
  DICT_ADD( dout, globalq_pull_entry_request_total_bytes );
  DICT_ADD( dout, globalq_pull_entry_reply_messages );
  DICT_ADD( dout, globalq_pull_entry_reply_total_bytes );
  
  DICT_ADD( dout, globalq_push_request_accepted );
  DICT_ADD( dout, globalq_push_request_rejected );
  DICT_ADD( dout, globalq_push_entry_hadConsumer );
  DICT_ADD( dout, globalq_push_entry_noConsumer );

  std::cout << "GlobalQueueStatistics " << dout.toString() << std::endl;
}

void GlobalQueueStatistics::merge( const GlobalQueueStatistics * other ) {
    globalq_pull_reserve_request_messages += other->globalq_pull_reserve_request_messages;
    globalq_pull_reserve_request_total_bytes += other->globalq_pull_reserve_request_total_bytes;
                                                                                                    
    globalq_pull_reserve_reply_messages += other->globalq_pull_reserve_reply_messages;
    globalq_pull_reserve_reply_total_bytes += other->globalq_pull_reserve_reply_total_bytes;
                                                                                                    
    globalq_push_reserve_request_messages += other->globalq_push_reserve_request_messages;
    globalq_push_reserve_request_total_bytes += other->globalq_push_reserve_request_total_bytes;
                                                                                                    
    globalq_push_entry_request_messages += other->globalq_push_entry_request_messages;
    globalq_push_entry_request_total_bytes += other->globalq_push_entry_request_total_bytes;
                                                                                                    
    globalq_push_reserve_reply_messages += other->globalq_push_reserve_reply_messages;
    globalq_push_reserve_reply_total_bytes += other->globalq_push_reserve_reply_total_bytes;

    globalq_pull_request_granted += other->globalq_pull_request_granted;
    globalq_pull_request_denied += other->globalq_pull_request_denied;
    globalq_pull_entry_request_messages += other->globalq_pull_entry_request_messages; 
    globalq_pull_entry_request_total_bytes += other->globalq_pull_entry_request_total_bytes;
    globalq_pull_entry_reply_messages += other->globalq_pull_entry_reply_messages;
    globalq_pull_entry_reply_total_bytes += other->globalq_pull_entry_reply_total_bytes;
                                                                              
    globalq_push_request_accepted += other->globalq_push_request_accepted;
    globalq_push_request_rejected += other->globalq_push_request_rejected;
    globalq_push_entry_hadConsumer += other->globalq_push_entry_hadConsumer;
    globalq_push_entry_noConsumer += other->globalq_push_entry_noConsumer;
} 

void GlobalQueueStatistics::record_pull_reserve_request( size_t msg_bytes, bool granted ) {
  globalq_pull_reserve_request_messages += 1;
  globalq_pull_reserve_request_total_bytes += msg_bytes;

  if ( granted ) {
    globalq_pull_request_granted += 1;
  } else {
    globalq_pull_request_denied += 1;
  }
}

void GlobalQueueStatistics::record_push_entry_request( size_t msg_bytes, bool had_consumer ) {
  globalq_push_entry_request_messages += 1;
  globalq_push_entry_request_total_bytes += msg_bytes;

  if ( had_consumer ) {
    globalq_push_entry_hadConsumer += 1;
  } else {
    globalq_push_entry_noConsumer += 1;
  }
}

void GlobalQueueStatistics::record_push_reserve_request( size_t msg_bytes, bool accepted ) {
  globalq_push_reserve_request_messages += 1;
  globalq_push_reserve_request_total_bytes += msg_bytes;

  if ( accepted ) {
    globalq_push_request_accepted += 1;
  } else {
    globalq_push_request_rejected += 1;
  }
}
    
void GlobalQueueStatistics::record_pull_entry_request( size_t msg_bytes ) {
  globalq_pull_entry_request_messages += 1;
  globalq_pull_entry_request_total_bytes += msg_bytes;
}
    
void GlobalQueueStatistics::record_push_reserve_reply( size_t msg_bytes ) {
  globalq_push_reserve_reply_messages += 1;
  globalq_push_reserve_reply_total_bytes += msg_bytes;
}

void GlobalQueueStatistics::record_pull_reserve_reply( size_t msg_bytes ) {
  globalq_pull_reserve_reply_messages += 1;
  globalq_pull_reserve_reply_total_bytes += msg_bytes;
}
    
void GlobalQueueStatistics::record_pull_entry_reply( size_t msg_bytes ) {
  globalq_pull_entry_reply_messages += 1;
  globalq_pull_entry_reply_total_bytes += msg_bytes;
}


