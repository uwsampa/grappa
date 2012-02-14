
#ifndef __INCOHERENT_ACQUIRER_HPP__
#define __INCOHERENT_ACQUIRER_HPP__

// forward declare for active message templates
template< typename T >
class IncoherentAcquirer;

template< typename T >
static void incoherent_acquire_reply_am( typename IncoherentAcquirer< T >::ReplyArgs * args, 
                                         size_t size, 
                                         void * payload, size_t payload_size ) {
  if (DEBUG_CACHE) std::cout << "thread " << current_thread << " received acquire reply" << std::endl;
  args->reply_address.pointer()->acquire_reply( payload, payload_size );
}

template< typename T >
static void incoherent_acquire_request_am( typename IncoherentAcquirer< T >::RequestArgs * args, 
                                    size_t size, 
                                    void * payload, size_t payload_size ) {
  if (DEBUG_CACHE) std::cout << "thread " << current_thread << " received acquire request" << std::endl;
  typename IncoherentAcquirer<T>::ReplyArgs reply_args;
  reply_args.reply_address = args->reply_address;
  SoftXMT_call_on( args->reply_address.node(), incoherent_acquire_reply_am<T>,
                   &reply_args, sizeof( reply_args),  
                   args->request_address.pointer(), args->count * sizeof( T ) );
  SoftXMT_flush( args->reply_address.node() );
  if (DEBUG_CACHE) std::cout << "thread " << current_thread << " sent acquire reply" << std::endl;
}

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
      if (DEBUG_CACHE) std::cout << "thread " << current_thread << " issuing acquire" << std::endl;
      acquire_started_ = true;
      //IncoherentAcquirer<T>::RequestArgs args;
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
      if (DEBUG_CACHE) std::cout << "thread " << current_thread << " ready to block" << std::endl;
      while( !acquired_ ) {
        if (DEBUG_CACHE) std::cout << "thread " << current_thread << " blocking" << std::endl;
        thread_ = current_thread;
        SoftXMT_suspend();
      }
    }
  }

  void acquire_reply( void * payload, size_t payload_size ) { 
    memcpy( storage_, payload, payload_size );
    SoftXMT_wake( thread_ );
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


#endif
