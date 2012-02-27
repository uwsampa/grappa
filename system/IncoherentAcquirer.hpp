
#ifndef __INCOHERENT_ACQUIRER_HPP__
#define __INCOHERENT_ACQUIRER_HPP__

#include "SoftXMT.hpp"

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

template< typename T >
class IncoherentAcquirer {
private:
  bool acquire_started_;
  bool acquired_;
  GlobalAddress< T > request_address_;
  size_t count_;
  T * storage_;
  thread * thread_;

public:

  IncoherentAcquirer( GlobalAddress< T > request_address, size_t count, T * storage )
    : request_address_( request_address )
    , count_( count )
    , storage_( storage )
    , acquire_started_( false )
    , acquired_( false )
    , thread_(NULL)
  { }
    
  void start_acquire() { 
    if( !acquire_started_ ) {
      DVLOG(5) << "thread " << current_thread 
              << " issuing acquire for " << request_address_ 
              << " * " << count_ ;
      acquire_started_ = true;
      RequestArgs args;
      args.request_address = request_address_;
      args.count = count_;
      args.reply_address = make_global( this );
      SoftXMT_call_on( request_address_.node(), &incoherent_acquire_request_am<T>, &args );
    }
  }

  void block_until_acquired() {
    if( !acquired_ ) {
      start_acquire();
      DVLOG(5) << "thread " << current_thread 
              << " ready to block on " << request_address_ 
              << " * " << count_ ;
      while( !acquired_ ) {
      DVLOG(5) << "thread " << current_thread 
              << " blocking on " << request_address_ 
              << " * " << count_ ;
        thread_ = current_thread;
        SoftXMT_suspend();
        thread_ = NULL;
        DVLOG(5) << "thread " << current_thread 
                 << " woke up for " << request_address_ 
                 << " * " << count_ ;
      }
    }
  }

  void acquire_reply( void * payload, size_t payload_size ) { 
  DVLOG(5) << "thread " << current_thread 
           << " copying reply payload of " << payload_size
           << " and waking thread " << thread_;
    memcpy( storage_, payload, payload_size );
    if ( thread_ != NULL ) SoftXMT_wake( thread_ );
    acquired_ = true;
  }

  bool acquired() const { return acquired_; }


  struct ReplyArgs {
    GlobalAddress< IncoherentAcquirer > reply_address;
  };

  struct RequestArgs {
    GlobalAddress< T > request_address;
    size_t count;
    GlobalAddress< IncoherentAcquirer > reply_address;
  };


};

template< typename T >
static void incoherent_acquire_reply_am( typename IncoherentAcquirer< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size ) {
  DVLOG(5) << "thread " << current_thread 
           << " received acquire reply to " << args->reply_address
           << " payload size " << payload_size;
  args->reply_address.pointer()->acquire_reply( payload, payload_size );
}

template< typename T >
static void incoherent_acquire_request_am( typename IncoherentAcquirer< T >::RequestArgs * args, 
                                    size_t size, 
                                    void * payload, size_t payload_size ) {
  DVLOG(5) << "thread " << current_thread 
           << " received acquire request to " << args->request_address
           << " count " << args->count
           << " reply to " << args->reply_address;
  typename IncoherentAcquirer<T>::ReplyArgs reply_args;
  reply_args.reply_address = args->reply_address;
  SoftXMT_call_on( args->reply_address.node(), incoherent_acquire_reply_am<T>,
                   &reply_args, sizeof( reply_args),  
                   args->request_address.pointer(), args->count * sizeof( T ) );
  DVLOG(5) << "thread " << current_thread 
           << " sent acquire reply to " << args->reply_address
           << " payload size " << args->count * sizeof( T );
}


#endif
