
#include "Cache.hpp"

Node address2node( void * ) {
  return 0;
}

template< typename T >
void incoherent_acquire_reply_am( typename IncoherentAcquirer< T >::ReplyArgs * args, 
                                  size_t size, 
                                  void * payload, size_t payload_size ) {
  memcpy( args->reply_address->storage_, payload, payload_size );
  args->reply_address->acquired_ = true;
}

template< typename T >
void incoherent_acquire_request_am( typename IncoherentAcquirer< T >::RequestArgs * args, 
                                    size_t size, 
                                    void * payload, size_t payload_size ) {
  typename IncoherentAcquirer<T>::ReplyArgs reply_args;
  reply_args.reply_address = args->reply_address;
  SoftXMT_call_on( args->reply_address.node(), incoherent_acquire_reply_am<T>,
                   &reply_args, sizeof( reply_args),  
                   args->request_address, args->size );
}

template< typename T >
void IncoherentAcquirer<T>::start_acquire() { 
  if( !acquire_started_ ) {
    acquire_started_ = true;
    //IncoherentAcquirer<T>::RequestArgs args;
    RequestArgs args;
    args.request_address = request_address_;
    args.count = count_;
    args.reply_address = make_global( this );
    SoftXMT_call_on( request_address_.node(), &incoherent_acquire_request_am<T>, &args );
  }
}

template< typename T >
void IncoherentAcquirer<T>::block_until_acquired() {
  if( !acquired_ ) {
    start_acquire();
    while( !acquired_ ) {
      SoftXMT_suspend();
    }
  }
}





// void incoherent_release_request_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
//   args->descriptor->done = true;
//   Delegate_wakeup( args->descriptor );
// }

// void incoherent_release_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
//   args->descriptor->done = true;
//   Delegate_wakeup( args->descriptor );
// }

// template< typename T >
// void IncoherentReleaser::start_release() { 
//   if( !release_started_ ) {
//     release_started_ = true;
//     ;
//   }
// }

// template< typename T >
// void IncoherentReleaser::block_until_released() {
//     if( !released_ ) {
//       ;
//       released_ = true;
//     }
//   }

// template< typename T >
// bool IncoherentReleaser::released() {
//   return released_;
// }
