
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __INCOHERENT_RELEASER_HPP__
#define __INCOHERENT_RELEASER_HPP__

#include "Grappa.hpp"
#include "Message.hpp"

#ifdef VTRACE
#include <vt_user.h>
#endif

// forward declare for active message templates
template< typename T >
class IncoherentReleaser;


template< typename T >
static void incoherent_release_reply_am( typename IncoherentReleaser< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size );

template< typename T >
static void incoherent_release_request_am( typename IncoherentReleaser< T >::RequestArgs * args, 
                                           size_t size, 
                                           void * payload, size_t payload_size );

/// Stats for IncoherentReleaser
class IRStatistics {
  private:
    uint64_t release_ams;
    uint64_t release_ams_bytes;
#ifdef VTRACE_SAMPLED
    unsigned ir_grp_vt;
    unsigned release_ams_ev_vt;
    unsigned release_ams_bytes_ev_vt;
#endif

  public:
    IRStatistics();
    void reset();

    inline void count_release_ams( uint64_t bytes ) {
      release_ams++;
      release_ams_bytes+=bytes;
    }

    void dump( std::ostream& o, const char * terminator );
    void sample();
    void profiling_sample();
    void merge(const IRStatistics * other);
};

extern IRStatistics incoherent_releaser_stats;


/// IncoherentReleaser behavior for cache.
template< typename T >
class IncoherentReleaser {
private:
  bool release_started_;
  bool released_;
  GlobalAddress< T > * request_address_;
  size_t * count_;
  T ** pointer_;
  Thread * thread_;
  int num_messages_;
  int response_count_;


public:

  IncoherentReleaser( GlobalAddress< T > * request_address, size_t * count, T ** pointer )
    : request_address_( request_address )
    , count_( count )
    , pointer_( pointer )
    , release_started_( false )
    , released_( false )
    , thread_(NULL)
    , num_messages_(0)
    , response_count_(0)
  { 
    reset();
  }
    
  void reset( ) {
    CHECK( !release_started_ || released_ ) << "inconsistent state for reset";
    release_started_ = false;
    released_ = false;
    thread_ = NULL;
    num_messages_ = 0;
    response_count_ = 0;
    if( *count_ == 0 ) {
      DVLOG(5) << "Zero-length release";
      release_started_ = true;
      released_ = true;
    } else if( request_address_->is_2D() ) {
      num_messages_ = 1;
      if( request_address_->node() == Grappa_mynode() ) {
        release_started_ = true;
        released_ = true;
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
      DVLOG(5) << " address + count -1 block max is " << (*request_address_ + *count_ - 1).block_max();
      DVLOG(5) << " difference is " << ( (*request_address_ + *count_ - 1).block_max() - request_address_->block_min() );
      DVLOG(5) << " multiplied difference is " << ( (*request_address_ + *count_ - 1).block_max() - request_address_->block_min() ) * sizeof(T);
      DVLOG(5) << " address block min " << request_address_->block_min();
      DVLOG(5) << "Straddle: diff is " << byte_diff << " bs " << block_size;
      num_messages_ = byte_diff / block_size;
    }

    if( num_messages_ > 1 ) DVLOG(5) << "****************************** MULTI BLOCK CACHE REQUEST ******************************";

    DVLOG(5) << "Detecting straddle for sizeof(T):" << sizeof(T)
             << " count:" << *count_
             << " num_messages_:" << num_messages_
             << " request_address:" << *request_address_;
  }

  void start_release() { 
    if( !release_started_ ) {
#ifdef VTRACE_FULL
      VT_TRACER("incoherent start_release");
#endif
            DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " issuing release for " << *request_address_ 
              << " * " << *count_ ;
      release_started_ = true;
      RequestArgs args;
      args.request_address = *request_address_;
      DVLOG(5) << "Computing request_bytes from block_max " << request_address_->first_byte().block_max() << " and " << *request_address_;
      args.reply_address = make_global( this );
      size_t offset = 0;  
      size_t request_bytes = 0;
      size_t total_bytes = *count_ * sizeof(T);

      // allocate enough requests/messages that we don't run out
      size_t nmsg = total_bytes / block_size + 2;
      RequestArgs arg_array[nmsg];
      Grappa::ExternalPayloadMessage<RequestArgs> msg_array[nmsg];
      
      for( size_t i = 0;
           offset < total_bytes; 
           offset += request_bytes, i++) {

        request_bytes = args.request_address.first_byte().block_max() - args.request_address.first_byte();

        if( request_bytes > total_bytes - offset ) {
          request_bytes = total_bytes - offset;
        }

        DVLOG(5) << "sending release request with " << request_bytes
                 << " of total bytes = " << *count_ * sizeof(T)
                 << " to " << args.request_address;

        arg_array[i] = args;
        new (msg_array+i) Grappa::ExternalPayloadMessage<RequestArgs>(arg_array[i].request_address.node(), &arg_array[i], ((char*)(*pointer_)) + offset, request_bytes);
        msg_array[i].enqueue();
//        Grappa_call_on( args.request_address.node(), &incoherent_release_request_am<T>,
//                         &args, sizeof( args ),
//                         ((char*)(*pointer_)) + offset, request_bytes);

	// TODO: change type so we don't screw with pointer like this
        args.request_address = GlobalAddress<T>::Raw( args.request_address.raw_bits() + request_bytes );
      }
      DVLOG(5) << "release started for " << args.request_address;
      
      // blocks here waiting for messages to be sent
    }
  }

  void block_until_released() {
    if( !released_ ) {
      start_release();
#ifdef VTRACE_FULL
      VT_TRACER("incoherent block_until_released");
#endif
      DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " ready to block on " << *request_address_ 
              << " * " << *count_ ;
      while( !released_ ) {
        DVLOG(5) << "Thread " << CURRENT_THREAD 
                << " blocking on " << *request_address_ 
                << " * " << *count_ ;
        if( !released_ ) {
          thread_ = CURRENT_THREAD;
          Grappa_suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "Thread " << CURRENT_THREAD 
                 << " woke up for " << *request_address_ 
                 << " * " << *count_ ;
      }
    }
  }

  void release_reply( ) { 
    DVLOG(5) << "Thread " << CURRENT_THREAD 
             << " received release reply ";
    ++response_count_;
    if ( response_count_ == num_messages_ ) {
      released_ = true;
      if( thread_ != NULL ) {
        DVLOG(5) << "Thread " << CURRENT_THREAD 
                 << " waking Thread " << thread_;
        Grappa_wake( thread_ );
      }
    }
  }

  bool released() const { return released_; }


  struct ReplyArgs {
    GlobalAddress< IncoherentReleaser > reply_address;
    void operator()() {
      DVLOG(5) << "Thread " << CURRENT_THREAD << " received release reply to " << reply_address;
      reply_address.pointer()->release_reply( );
    }
  };
  
  struct RequestArgs {
    GlobalAddress< T > request_address;
    GlobalAddress< IncoherentReleaser > reply_address;
    
    void operator()(void * payload, size_t payload_size) {
      incoherent_releaser_stats.count_release_ams( payload_size );
      DVLOG(5) << "Thread " << CURRENT_THREAD
      << " received release request to " << request_address
      << " reply to " << reply_address;
      memcpy( request_address.pointer(), payload, payload_size );
      ReplyArgs reply_args;
      reply_args.reply_address = reply_address;
      
      Grappa::send_heap_message(reply_address.node(), reply_args);
      
      DVLOG(5) << "Thread " << CURRENT_THREAD
      << " sent release reply to " << reply_address;
    }
  };
};

#endif
