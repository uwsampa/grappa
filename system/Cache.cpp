
#include "Cache.hpp"

Node address2node( void * ) {
  return 0;
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
