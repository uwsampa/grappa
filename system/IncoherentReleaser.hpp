
#ifndef __INCOHERENT_RELEASER_HPP__
#define __INCOHERENT_RELEASER_HPP__

#include "SoftXMT.hpp"

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

template< typename T >
class IncoherentReleaser {
private:
  bool release_started_;
  bool released_;
  GlobalAddress< T > request_address_;
  size_t count_;
  T * storage_;
  thread * thread_;

public:

  IncoherentReleaser( GlobalAddress< T > request_address, size_t count, T * storage )
    : request_address_( request_address )
    , count_( count )
    , storage_( storage )
    , release_started_( false )
    , released_( false )
    , thread_(NULL)
  { }
    
  void start_release() { 
    if( !release_started_ ) {
            DVLOG(5) << "thread " << current_thread 
              << " issuing release for " << request_address_ 
              << " * " << count_ ;
      release_started_ = true;
      RequestArgs args;
      args.request_address = request_address_;
      args.reply_address = make_global( this );
      SoftXMT_call_on( request_address_.node(), 
                       &incoherent_release_request_am<T>, 
                       &args, sizeof(args),
                       (void *)storage_, sizeof(T) * count_ );
    }
  }

  void block_until_released() {
    if( !released_ ) {
      start_release();
      DVLOG(5) << "thread " << current_thread 
              << " ready to block on " << request_address_ 
              << " * " << count_ ;
      while( !released_ ) {
        DVLOG(5) << "thread " << current_thread 
                << " blocking on " << request_address_ 
                << " * " << count_ ;
        if( !released_ ) {
          thread_ = current_thread;
          SoftXMT_suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "thread " << current_thread 
                 << " woke up for " << request_address_ 
                 << " * " << count_ ;
      }
    }
  }

  void release_reply( ) { 
    if( thread_ ) SoftXMT_wake( thread_ );
    released_ = true;
  }

  bool released() const { return released_; }


  struct ReplyArgs {
    GlobalAddress< IncoherentReleaser > reply_address;
  };
  
  struct RequestArgs {
    GlobalAddress< T > request_address;
    GlobalAddress< IncoherentReleaser > reply_address;
  };
};


template< typename T >
static void incoherent_release_reply_am( typename IncoherentReleaser< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size ) {
  DVLOG(5) << "thread " << current_thread << " received release reply " << args->reply_address;
  args->reply_address.pointer()->release_reply( );
}

template< typename T >
static void incoherent_release_request_am( typename IncoherentReleaser< T >::RequestArgs * args, 
                                    size_t size, 
                                    void * payload, size_t payload_size ) {
  DVLOG(5) << "thread " << current_thread 
           << " received release request to " << args->request_address 
           << " reply to " << args->reply_address;
  memcpy( args->request_address.pointer(), payload, payload_size );
  typename IncoherentReleaser<T>::ReplyArgs reply_args;
  reply_args.reply_address = args->reply_address;
  SoftXMT_call_on( args->reply_address.node(), incoherent_release_reply_am<T>,
                   &reply_args, sizeof( reply_args ), 
                   NULL, 0 );
  DVLOG(5) << "thread " << current_thread 
           << " sent release reply to " << args->reply_address;
}

#endif
