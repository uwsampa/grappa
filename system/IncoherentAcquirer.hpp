
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __INCOHERENT_ACQUIRER_HPP__
#define __INCOHERENT_ACQUIRER_HPP__

#include "Grappa.hpp"
#include "Addressing.hpp"
#include "Message.hpp"
#include "MessagePool.hpp"
#include "tasks/TaskingScheduler.hpp"

// forward declare for active message templates
template< typename T >
class IncoherentAcquirer;

/// IncoherentAcquirer statistics
class IAStatistics {
public:
  static void count_acquire_ams( uint64_t bytes ) ;
  static void record_wakeup_latency( int64_t start_time, int64_t network_time ) ; 
  static void record_network_latency( int64_t start_time ) ; 
};

/// IncoherentAcquirer behavior for Cache.
template< typename T >
class IncoherentAcquirer {
private:
  bool acquire_started_;
  bool acquired_;
  GlobalAddress< T > * request_address_;
  size_t * count_;
  T ** pointer_;
  Grappa::Worker * thread_;
  int num_messages_;
  int response_count_;
  int total_reply_payload_;
  int expected_reply_payload_;
  int64_t start_time_;
  int64_t network_time_;

public:

  IncoherentAcquirer( GlobalAddress< T > * request_address, size_t * count, T ** pointer )
    : request_address_( request_address )
    , count_( count )
    , pointer_( pointer )
    , acquire_started_( false )
    , acquired_( false )
    , thread_(NULL)
    , num_messages_(0)
    , response_count_(0)
    , total_reply_payload_( 0 )
    , expected_reply_payload_( 0 )
    , start_time_(0)
    , network_time_(0)
  { 
    reset( );
  }

  void reset( ) {
    DVLOG(5) << "In " << __PRETTY_FUNCTION__;
    CHECK( !acquire_started_ || acquired_ ) << "inconsistent state for reset";
    acquire_started_ = false;
    acquired_ = false;
    thread_ = NULL;
    num_messages_ = 0;
    response_count_ = 0;
    expected_reply_payload_ = sizeof( T ) * *count_;
    total_reply_payload_ = 0;
    start_time_ = 0;
    network_time_ = 0;
    if( *count_ == 0 ) {
      DVLOG(5) << "Zero-length acquire";
      *pointer_ = NULL;
      acquire_started_ = true;
      acquired_ = true;
    } else if( request_address_->is_2D() ) {
      num_messages_ = 1;
      if( request_address_->node() == Grappa::mycore() ) {
        DVLOG(5) << "Short-circuiting to address " << request_address_->pointer();
        *pointer_ = request_address_->pointer();
        acquire_started_ = true;
        acquired_ = true;
      }
    } else {
      DVLOG(5) << "Straddle: block_max is " << (*request_address_ + *count_).block_max() ;
      DVLOG(5) << ", request_address is " << *request_address_;
      DVLOG(5) << ", sizeof(T) is " << sizeof(T);
      DVLOG(5) << ", count is " << *count_;
      DVLOG(5) << ", block_min is " << request_address_->block_min();


      DVLOG(5) << "Straddle: address is " << *request_address_ ;
      DVLOG(5) << ", address + count is " << *request_address_ + *count_;

      ptrdiff_t byte_diff = ( (*request_address_ + *count_ - 1).last_byte().block_max() - 
                               request_address_->first_byte().block_min() );

      DVLOG(5) << "Straddle: address block max is " << request_address_->block_max();
      DVLOG(5) << " address + count block max is " << (*request_address_ + *count_).block_max();
      DVLOG(5) << " address block min " << request_address_->block_min();
      DVLOG(5) << "Straddle: diff is " << byte_diff << " bs " << block_size;
      num_messages_ = byte_diff / block_size;
    }

    if( num_messages_ > 1 ) DVLOG(5) << "****************************** MULTI BLOCK CACHE REQUEST ******************************";

    DVLOG(5) << "In " << __PRETTY_FUNCTION__ << "; detecting straddle for sizeof(T):" << sizeof(T)
             << " count:" << *count_
             << " num_messages_:" << num_messages_
             << " request_address:" << *request_address_;
  }
    
  void start_acquire() { 
    if( !acquire_started_ ) {
#ifdef VTRACE_FULL
      VT_TRACER("incoherent start_acquire");
#endif
      DVLOG(5) << "Worker " << CURRENT_THREAD 
              << " issuing acquire for " << *request_address_ 
              << " * " << *count_ ;
      acquire_started_ = true;
      size_t total_bytes = *count_ * sizeof(T);
      
      // allocate enough requests/messages that we don't run out
      size_t nmsg = total_bytes / block_size + 2;
      size_t msg_size = sizeof(Grappa::Message<RequestArgs>);
      
      if (nmsg*msg_size < Grappa::current_worker().stack_remaining()-8192) {
        CHECK_LT(Grappa::current_worker().stack_remaining(), STACK_SIZE);
        // try to put message storage on stack if there's space
        char msg_buf[nmsg*msg_size];
        Grappa::MessagePool pool(msg_buf, sizeof(msg_buf));
        do_acquire(pool);
      } else {
        // fall back on heap-allocating the storage
        Grappa::MessagePool pool(nmsg*msg_size);
        do_acquire(pool);
      }
    }
  }

  void do_acquire(Grappa::impl::MessagePoolBase& pool) {
    size_t total_bytes = *count_ * sizeof(T);
    RequestArgs args;
    args.request_address = *request_address_;
    DVLOG(5) << "Computing request_bytes from block_max " << request_address_->first_byte().block_max() << " and " << *request_address_;
    args.reply_address = make_global( this );
    args.offset = 0;  
    
    for(size_t i = 0;
         args.offset < total_bytes; 
         args.offset += args.request_bytes, i++) {

      args.request_bytes = args.request_address.first_byte().block_max() - args.request_address.first_byte();


      if( args.request_bytes > total_bytes - args.offset ) {
        args.request_bytes = total_bytes - args.offset;
      }

      DVLOG(5) << "sending acquire request for " << args.request_bytes
               << " of total bytes = " << *count_ * sizeof(T)
               << " from " << args.request_address;

      pool.send_message(args.request_address.core(), [args]{
        IAStatistics::count_acquire_ams( args.request_bytes );
        DVLOG(5) << "Worker " << CURRENT_THREAD
        << " received acquire request to " << args.request_address
        << " size " << args.request_bytes
        << " offset " << args.offset
        << " reply to " << args.reply_address;
          
        DVLOG(5) << "Worker " << CURRENT_THREAD
        << " sending acquire reply to " << args.reply_address
        << " offset " << args.offset
        << " request address " << args.request_address
        << " payload address " << args.request_address.pointer()
        << " payload size " << args.request_bytes;
          
        // note: this will read the payload *later* when the message is copied into the actual send buffer,
        // should be okay because we're already assuming DRF, but something to watch out for
        auto reply_address = args.reply_address;
        auto offset = args.offset;
          
        Grappa::send_heap_message(args.reply_address.node(),
          [reply_address, offset](void * payload, size_t payload_size) {
            DVLOG(5) << "Worker " << CURRENT_THREAD
            << " received acquire reply to " << reply_address
            << " offset " << offset
            << " payload size " << payload_size;
            reply_address.pointer()->acquire_reply( offset, payload, payload_size);
          },
          args.request_address.pointer(), args.request_bytes
        );
          
        DVLOG(5) << "Worker " << CURRENT_THREAD
        << " sent acquire reply to " << args.reply_address
        << " offset " << args.offset
        << " request address " << args.request_address
        << " payload address " << args.request_address.pointer()
        << " payload size " << args.request_bytes;
      });

      // TODO: change type so we don't screw with pointer like this
      args.request_address = GlobalAddress<T>::Raw( args.request_address.raw_bits() + args.request_bytes );
    }
    DVLOG(5) << "acquire started for " << args.request_address;      
  }

  void block_until_acquired() {
    if( !acquired_ ) {
      start_acquire();
#ifdef VTRACE_FULL
      VT_TRACER("incoherent block_until_acquired");
#endif
      DVLOG(5) << "Worker " << CURRENT_THREAD 
              << " ready to block on " << *request_address_ 
              << " * " << *count_ ;
      if( !acquired_ ) {
        start_time_ = Grappa::timestamp();
      } else {
        start_time_ = 0;
      }
      while( !acquired_ ) {
      DVLOG(5) << "Worker " << CURRENT_THREAD 
              << " blocking on " << *request_address_ 
              << " * " << *count_ ;
        if( !acquired_ ) {
          thread_ = CURRENT_THREAD;
          Grappa::suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "Worker " << CURRENT_THREAD 
                 << " woke up for " << *request_address_ 
                 << " * " << *count_ ;
      }
      IAStatistics::record_wakeup_latency( start_time_, network_time_ );
    }
  }

  void acquire_reply( size_t offset, void * payload, size_t payload_size ) { 
    DVLOG(5) << "Worker " << CURRENT_THREAD 
             << " copying reply payload of " << payload_size
             << " and waking Worker " << thread_;
    memcpy( ((char*)(*pointer_)) + offset, payload, payload_size );
    ++response_count_;
    total_reply_payload_ += payload_size;
    if ( response_count_ == num_messages_ ) {
      DCHECK_EQ( total_reply_payload_, expected_reply_payload_ ) << "Got back the wrong amount of data "
                                                                 << "(with base = " << *request_address_
                                                                 << " and sizeof(T) = " << sizeof(T) 
                                                                 << " and count = " << *count_;
      acquired_ = true;
      if( thread_ != NULL ) {
        Grappa::wake( thread_ );
      }
      if( start_time_ != 0 ) {
        network_time_ = Grappa::timestamp();
        IAStatistics::record_network_latency( start_time_ );
      }
    }
  }

  /// Has acquire completed?
  bool acquired() const { return acquired_; }

  /// Args for incoherent acquire request 
  struct RequestArgs {
    GlobalAddress< T > request_address;
    size_t request_bytes;
    GlobalAddress< IncoherentAcquirer > reply_address;
    int offset;
  };


};


#endif
