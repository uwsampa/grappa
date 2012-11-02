
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __INCOHERENT_ACQUIRER_HPP__
#define __INCOHERENT_ACQUIRER_HPP__

#include "Grappa.hpp"
#include "Addressing.hpp"


#ifdef VTRACE
#include <vt_user.h>
#endif

// forward declare for active message templates
template< typename T >
class IncoherentAcquirer;

template< typename T >
static void incoherent_acquire_reply_am( typename IncoherentAcquirer< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size );

template< typename T >
static void incoherent_acquire_request_am( typename IncoherentAcquirer< T >::RequestArgs * args, 
                                           size_t size, 
                                           void * payload, size_t payload_size );
/// IncoherentAcquirer statistics
class IAStatistics {
  private:
  uint64_t acquire_ams;
  uint64_t acquire_ams_bytes;
  uint64_t acquire_blocked;
  uint64_t acquire_blocked_ticks_total;
  uint64_t acquire_network_ticks_total;
  uint64_t acquire_wakeup_ticks_total;
  uint64_t acquire_blocked_ticks_max;
  uint64_t acquire_blocked_ticks_min;
  uint64_t acquire_network_ticks_max;
  uint64_t acquire_network_ticks_min;
  uint64_t acquire_wakeup_ticks_max;
  uint64_t acquire_wakeup_ticks_min;
#ifdef VTRACE_SAMPLED
  unsigned ia_grp_vt;
  unsigned acquire_ams_ev_vt;
  unsigned acquire_ams_bytes_ev_vt;
  unsigned acquire_blocked_ev_vt;
  unsigned acquire_blocked_ticks_total_ev_vt;
  unsigned acquire_network_ticks_total_ev_vt;
  unsigned acquire_wakeup_ticks_total_ev_vt;
  unsigned acquire_blocked_ticks_max_ev_vt;
  unsigned acquire_blocked_ticks_min_ev_vt;
  unsigned acquire_wakeup_ticks_max_ev_vt;
  unsigned acquire_wakeup_ticks_min_ev_vt;
  unsigned acquire_network_ticks_max_ev_vt;
  unsigned acquire_network_ticks_min_ev_vt;
  unsigned average_latency_ev_vt;
  unsigned average_network_latency_ev_vt;
  unsigned average_wakeup_latency_ev_vt;
#endif

  public:
    IAStatistics();
    void reset();

    inline void count_acquire_ams( uint64_t bytes ) {
      acquire_ams++;
      acquire_ams_bytes+=bytes;
    }

  inline void record_wakeup_latency( int64_t start_time, int64_t network_time ) { 
    acquire_blocked++; 
    int64_t current_time = Grappa_get_timestamp();
    int64_t blocked_latency = current_time - start_time;
    int64_t wakeup_latency = current_time - network_time;
    acquire_blocked_ticks_total += blocked_latency;
    acquire_wakeup_ticks_total += wakeup_latency;
    if( blocked_latency > acquire_blocked_ticks_max ) 
      acquire_blocked_ticks_max = blocked_latency;
    if( blocked_latency < acquire_blocked_ticks_min ) 
      acquire_blocked_ticks_min = blocked_latency;
    if( wakeup_latency > acquire_wakeup_ticks_max )
      acquire_wakeup_ticks_max = wakeup_latency;
    if( wakeup_latency < acquire_wakeup_ticks_min )
      acquire_wakeup_ticks_min = wakeup_latency;
  }

  inline void record_network_latency( int64_t start_time ) { 
    int64_t current_time = Grappa_get_timestamp();
    int64_t latency = current_time - start_time;
    acquire_network_ticks_total += latency;
    if( latency > acquire_network_ticks_max )
      acquire_network_ticks_max = latency;
    if( latency < acquire_network_ticks_min )
      acquire_network_ticks_min = latency;
  }

    void dump( std::ostream& o, const char * terminator );
    void sample();
    void profiling_sample();
    void merge(const IAStatistics * other);
};

extern IAStatistics incoherent_acquirer_stats;

/// IncoherentAcquirer behavior for Cache.
template< typename T >
class IncoherentAcquirer {
private:
  bool acquire_started_;
  bool acquired_;
  GlobalAddress< T > * request_address_;
  size_t * count_;
  T ** pointer_;
  Thread * thread_;
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
      if( request_address_->node() == Grappa_mynode() ) {
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

    DVLOG(5) << "New IncoherentAcquirer; detecting straddle for sizeof(T):" << sizeof(T)
             << " count:" << *count_
             << " num_messages_:" << num_messages_
             << " request_address:" << *request_address_;
  }
    
  void start_acquire() { 
    if( !acquire_started_ ) {
#ifdef VTRACE_FULL
      VT_TRACER("incoherent start_acquire");
#endif
      DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " issuing acquire for " << *request_address_ 
              << " * " << *count_ ;
      acquire_started_ = true;
      RequestArgs args;
      args.request_address = *request_address_;
      DVLOG(5) << "Computing request_bytes from block_max " << request_address_->first_byte().block_max() << " and " << *request_address_;
      args.reply_address = make_global( this );
      args.offset = 0;  

      for( size_t total_bytes = *count_ * sizeof(T);
           args.offset < total_bytes; 
           args.offset += args.request_bytes) {

        args.request_bytes = args.request_address.first_byte().block_max() - args.request_address.first_byte();


        if( args.request_bytes > total_bytes - args.offset ) {
          args.request_bytes = total_bytes - args.offset;
        }

        DVLOG(5) << "sending acquire request for " << args.request_bytes
                 << " of total bytes = " << *count_ * sizeof(T)
                 << " from " << args.request_address;

        Grappa_call_on( args.request_address.node(), &incoherent_acquire_request_am<T>, &args );

	// TODO: change type so we don't screw with pointer like this
        args.request_address = GlobalAddress<T>::Raw( args.request_address.raw_bits() + args.request_bytes );
      }
      DVLOG(5) << "acquire started for " << args.request_address;
    }
  }

  void block_until_acquired() {
    if( !acquired_ ) {
      start_acquire();
#ifdef VTRACE_FULL
      VT_TRACER("incoherent block_until_acquired");
#endif
      DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " ready to block on " << *request_address_ 
              << " * " << *count_ ;
      if( !acquired_ ) {
	start_time_ = Grappa_get_timestamp();
      } else {
	start_time_ = 0;
      }
      while( !acquired_ ) {
      DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " blocking on " << *request_address_ 
              << " * " << *count_ ;
        if( !acquired_ ) {
          thread_ = CURRENT_THREAD;
          Grappa_suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "Thread " << CURRENT_THREAD 
                 << " woke up for " << *request_address_ 
                 << " * " << *count_ ;
      }
      incoherent_acquirer_stats.record_wakeup_latency( start_time_, network_time_ );
    }
  }

  void acquire_reply( size_t offset, void * payload, size_t payload_size ) { 
    DVLOG(5) << "Thread " << CURRENT_THREAD 
             << " copying reply payload of " << payload_size
             << " and waking Thread " << thread_;
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
        Grappa_wake( thread_ );
      }
      if( start_time_ != 0 ) {
	network_time_ = Grappa_get_timestamp();
	incoherent_acquirer_stats.record_network_latency( start_time_ );
      }
    }
  }

  /// Has acquire completed?
  bool acquired() const { return acquired_; }

  /// Args for incoherent acquire reply
  struct ReplyArgs {
    GlobalAddress< IncoherentAcquirer > reply_address;
    int offset;
  };

  /// Args for incoherent acquire request 
  struct RequestArgs {
    GlobalAddress< T > request_address;
    size_t request_bytes;
    GlobalAddress< IncoherentAcquirer > reply_address;
    int offset;
  };


};

/// Handler for incoherent acquire reply
template< typename T >
static void incoherent_acquire_reply_am( typename IncoherentAcquirer< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size ) {
  DVLOG(5) << "Thread " << CURRENT_THREAD 
           << " received acquire reply to " << args->reply_address
           << " offset " << args->offset
           << " payload size " << payload_size;
  args->reply_address.pointer()->acquire_reply( args->offset, payload, payload_size );
}

/// Handler for incoherent acquire request 
template< typename T >
static void incoherent_acquire_request_am( typename IncoherentAcquirer< T >::RequestArgs * args, 
                                    size_t size, 
                                    void * payload, size_t payload_size ) {
  incoherent_acquirer_stats.count_acquire_ams( args->request_bytes );
  DVLOG(5) << "Thread " << CURRENT_THREAD 
           << " received acquire request to " << args->request_address
           << " size " << args->request_bytes
           << " offset " << args->offset
           << " reply to " << args->reply_address;
  typename IncoherentAcquirer<T>::ReplyArgs reply_args;
  reply_args.reply_address = args->reply_address;
  reply_args.offset = args->offset;
  DVLOG(5) << "Thread " << CURRENT_THREAD 
           << " sending acquire reply to " << args->reply_address
           << " offset " << args->offset
           << " request address " << args->request_address
           << " payload address " << args->request_address.pointer()
           << " payload size " << args->request_bytes;
  Grappa_call_on( args->reply_address.node(), incoherent_acquire_reply_am<T>,
                   &reply_args, sizeof( reply_args ),  
                   args->request_address.pointer(), args->request_bytes );
  DVLOG(5) << "Thread " << CURRENT_THREAD 
           << " sent acquire reply to " << args->reply_address
           << " offset " << args->offset
           << " request address " << args->request_address
           << " payload address " << args->request_address.pointer()
           << " payload size " << args->request_bytes;
}


#endif
