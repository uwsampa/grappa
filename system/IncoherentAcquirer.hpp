
#ifndef __INCOHERENT_ACQUIRER_HPP__
#define __INCOHERENT_ACQUIRER_HPP__

#include "SoftXMT.hpp"
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

class IAStatistics {
  private:
    uint64_t acquire_ams;
    uint64_t acquire_ams_bytes;
#ifdef VTRACE_SAMPLED
    unsigned ia_grp_vt;
    unsigned acquire_ams_ev_vt;
    unsigned acquire_ams_bytes_ev_vt;
#endif

  public:
    IAStatistics();
    void reset();

    inline void count_acquire_ams( uint64_t bytes ) {
      acquire_ams++;
      acquire_ams_bytes+=bytes;
    }

    void dump();
    void sample();
    void profiling_sample();
    void merge(IAStatistics * other);
};

extern IAStatistics incoherent_acquirer_stats;

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
    if( count_ == 0 ) {
      DVLOG(5) << "Zero-length acquire";
      *pointer_ = NULL;
      acquire_started_ = true;
      acquired_ = true;
    } else if( request_address_->is_2D() ) {
      num_messages_ = 1;
      if( request_address_->node() == SoftXMT_mynode() ) {
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
      ptrdiff_t byte_diff = ( (*request_address_ + *count_ - 1).block_max() - 
      			      request_address_->block_min() );
      ptrdiff_t block_diff = byte_diff / block_size;
      DVLOG(5) << "Straddle: address block max is " << request_address_->block_max();
      DVLOG(5) << " address + count block max is " << (*request_address_ + *count_).block_max();
      DVLOG(5) << " address block min " << request_address_->block_min();
      DVLOG(5) << "Straddle: diff is " << byte_diff << " div " << block_diff << " bs " << block_size;
      num_messages_ = block_diff;
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
      DVLOG(5) << "Computing request_bytes from block_max " << request_address_->block_max() << " and " << *request_address_;
      args.reply_address = make_global( this );
      args.offset = 0;  

      for( size_t total_bytes = *count_ * sizeof(T);
           args.offset < total_bytes; 
           args.offset += args.request_bytes) {

        args.request_bytes = args.request_address.block_max() - args.request_address;
        if( args.request_bytes > total_bytes - args.offset ) {
          args.request_bytes = total_bytes - args.offset;
        }

        DVLOG(5) << "sending acquire request for " << args.request_bytes
                 << " of total bytes = " << *count_ * sizeof(T)
                 << " from " << args.request_address;

        SoftXMT_call_on( args.request_address.node(), &incoherent_acquire_request_am<T>, &args );

        args.request_address += args.request_bytes / sizeof(T);
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
      while( !acquired_ ) {
      DVLOG(5) << "Thread " << CURRENT_THREAD 
              << " blocking on " << *request_address_ 
              << " * " << *count_ ;
        if( !acquired_ ) {
          thread_ = CURRENT_THREAD;
          SoftXMT_suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "Thread " << CURRENT_THREAD 
                 << " woke up for " << *request_address_ 
                 << " * " << *count_ ;
      }
    }
  }

  void acquire_reply( size_t offset, void * payload, size_t payload_size ) { 
    DVLOG(5) << "Thread " << CURRENT_THREAD 
             << " copying reply payload of " << payload_size
             << " and waking Thread " << thread_;
    memcpy( ((char*)(*pointer_)) + offset, payload, payload_size );
    ++response_count_;
    if ( response_count_ == num_messages_ ) {
      acquired_ = true;
      if( thread_ != NULL ) {
        SoftXMT_wake( thread_ );
      }
    }
  }

  bool acquired() const { return acquired_; }


  struct ReplyArgs {
    GlobalAddress< IncoherentAcquirer > reply_address;
    int offset;
  };

  struct RequestArgs {
    GlobalAddress< T > request_address;
    size_t request_bytes;
    GlobalAddress< IncoherentAcquirer > reply_address;
    int offset;
  };


};

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
  SoftXMT_call_on( args->reply_address.node(), incoherent_acquire_reply_am<T>,
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
