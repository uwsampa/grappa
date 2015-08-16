////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#ifndef __INCOHERENT_RELEASER_HPP__
#define __INCOHERENT_RELEASER_HPP__

#include "Message.hpp"
#include "tasks/TaskingScheduler.hpp"

// forward declare for active message templates
template< typename T >
class IncoherentReleaser;

/// Stats for IncoherentReleaser
class IRMetrics {
  public:
    static void count_release_ams( uint64_t bytes );
};

/// IncoherentReleaser behavior for cache.
template< typename T >
class IncoherentReleaser {
private:
  bool release_started_;
  bool released_;
  GlobalAddress< T > * request_address_;
  size_t * count_;
  T ** pointer_;
  Grappa::Worker * thread_;
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
      if( request_address_->core() == Grappa::mycore() ) {
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
            DVLOG(5) << "Worker " << Grappa::current_worker() 
              << " issuing release for " << *request_address_ 
              << " * " << *count_ ;
      release_started_ = true;
      size_t total_bytes = *count_ * sizeof(T);

      // allocate enough requests/messages that we don't run out
      size_t nmsg = total_bytes / block_size + 2;
      size_t msg_size = sizeof(Grappa::PayloadMessage<RequestArgs>);
      
      do_release();
    }
  }
  
  void do_release() {
    size_t total_bytes = *count_ * sizeof(T);
    
    RequestArgs args;
    args.request_address = *request_address_;
    DVLOG(5) << "Computing request_bytes from block_max " << request_address_->first_byte().block_max() << " and " << *request_address_;
    args.reply_address = make_global( this );
    size_t offset = 0;
    size_t request_bytes = 0;

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

      Grappa::send_heap_message(args.request_address.core(),
        [args](void * payload, size_t payload_size) {
          IRMetrics::count_release_ams( payload_size );
          DVLOG(5) << "Worker " << Grappa::current_worker()
          << " received release request to " << args.request_address
          << " reply to " << args.reply_address;
          memcpy( args.request_address.pointer(), payload, payload_size );
    
          auto reply_address = args.reply_address;
          Grappa::send_heap_message(args.reply_address.core(), [reply_address]{
            DVLOG(5) << "Worker " << Grappa::current_worker() << " received release reply to " << reply_address;
            reply_address.pointer()->release_reply();
          });
    
          DVLOG(5) << "Worker " << Grappa::current_worker()
          << " sent release reply to " << reply_address;
        },
        (char*)(*pointer_) + offset, request_bytes
      );

      // TODO: change type so we don't screw with pointer like this
      args.request_address = GlobalAddress<T>::Raw( args.request_address.raw_bits() + request_bytes );
    }
    DVLOG(5) << "release started for " << args.request_address;
    
    // blocks here waiting for messages to be sent
  }

  void block_until_released() {
    if( !released_ ) {
      start_release();
#ifdef VTRACE_FULL
      VT_TRACER("incoherent block_until_released");
#endif
      DVLOG(5) << "Worker " << Grappa::current_worker() 
              << " ready to block on " << *request_address_ 
              << " * " << *count_ ;
      while( !released_ ) {
        DVLOG(5) << "Worker " << Grappa::current_worker() 
                << " blocking on " << *request_address_ 
                << " * " << *count_ ;
        if( !released_ ) {
          thread_ = Grappa::current_worker();
          Grappa::suspend();
          thread_ = NULL;
        }
        DVLOG(5) << "Worker " << Grappa::current_worker() 
                 << " woke up for " << *request_address_ 
                 << " * " << *count_ ;
      }
    }
  }

  void release_reply( ) { 
    DVLOG(5) << "Worker " << Grappa::current_worker() 
             << " received release reply ";
    ++response_count_;
    if ( response_count_ == num_messages_ ) {
      released_ = true;
      if( thread_ != NULL ) {
        DVLOG(5) << "Worker " << Grappa::current_worker() 
                 << " waking Worker " << thread_;
        Grappa::wake( thread_ );
      }
    }
  }

  bool released() const { return released_; }
  
  struct RequestArgs {
    GlobalAddress< T > request_address;
    GlobalAddress< IncoherentReleaser > reply_address;
  };
};

#endif
