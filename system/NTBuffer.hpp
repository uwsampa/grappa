#pragma once
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

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <tuple>

#include <x86intrin.h>

#include <common.hpp>

#define BUFFER_SIZE (1 << 19)

namespace Grappa {
namespace impl {

class NTBuffer {
  static const int local_buffer_size_ = 64;
  static int initial_offset_;
  static uint8_t * buffer_pool_;
  
  uint8_t localbuf_[ local_buffer_size_ ];

  // Buffer_ must be 64-byte aligned, so we only need 42 bits max of
  // address.
  //
  // Position_ and local_position_ count bytes, and can be combined
  // (masking off the lower three bits for the local part).
  uint8_t * buffer_;
  int position_;
  int local_position_;

  /// copy data into buffer
  inline int actual_enqueue( uint8_t * p, int size ) {
    // now copy in message, one cacheline at a time
    while( size > 0 ) {
      int count = std::min( size, local_buffer_size_ - local_position_ );
      DVLOG(5) << "Copying " << count << " bytes of " << size << " remaining.";
      memcpy( &localbuf_[ local_position_ ], p, count );
      p               += count;
      local_position_ += count;
      size            -= count;

      if( local_position_ == local_buffer_size_ ) {
        flush();
      }
    }
    DVLOG(5) << "After enqueue, position is " << position_ << " and local position is " << local_position_;
    return position_ + local_position_;
  }
  

public:
  NTBuffer()
    : localbuf_()
    , buffer_( nullptr )
    , position_( 0 )
    , local_position_( 0 )
  { }

  /// does buffer have anything in it?
  inline bool empty() const { return position_ == 0 && local_position_ == 0; }

  /// Return total bytes stored in buffer
  inline int size() const { return position_ + local_position_; }

  /// How many empty bytes are remaining in buffer?
  inline int remaining() const { return BUFFER_SIZE - size(); }


  /// get initial offset
  static inline int initial_offset() { return initial_offset_; }
  
  /// set initial offset
  static void set_initial_offset( int bytes ) {
    initial_offset_ = bytes;
  }

  /// create a new buffer
  void new_buffer( ) {
    DVLOG(5) << "Getting new buffer for " << this;

    // if we don't have one in the pool already, make a new one and push it on the pool.
    if( ! buffer_pool_ ) {
      CHECK( posix_memalign( reinterpret_cast<void**>(&buffer_pool_), 64, BUFFER_SIZE ) == 0 )
        << "posix_memalign error: buffer allocation failed";
      // clear out first word for future linked list usage
      *(reinterpret_cast<uint8_t**>(buffer_pool_)) = nullptr;
      DVLOG(5) << this << " allocated buffer " << buffer_pool_;
    }

    // pop a buffer off the pool to use.
    uint8_t * next_in_pool = *(reinterpret_cast<uint8_t**>(buffer_pool_));
    buffer_ = buffer_pool_;
    buffer_pool_ = next_in_pool;
    
    position_ = 0;
  }

  /// flush and detatch buffer from NTBuffer management. 
  std::tuple< void *, int > take_buffer() {
    flush();
#ifdef USE_NT_OPS
    _mm_sfence();
#endif
    auto retval = std::make_tuple( reinterpret_cast<void*>(buffer_), position_ );
    buffer_ = nullptr;
    position_ = 0;
    DCHECK_EQ( local_position_, 0 );
    return retval;
  }

  /// free buffer that was previously taken using take_buffer.
  static void free_buffer( void * buf ) {
    *(reinterpret_cast<uint8_t**>(buf)) = buffer_pool_;
    buffer_pool_ = reinterpret_cast<uint8_t*>( buf );
    //free( buf );
  }

  /// Move any data in temporary cacheline-size buffer to main
  /// buffer. Pad remaining bytes in cache line with zeros.
  ///
  /// Once this is called no more data can be enqueued before the
  /// buffer is taken. Generally there is no need to call this
  /// directly; take_buffer() calls it internally.
  int flush() {
    DVLOG(5) << "Flushing " << this << " with position " << position_ << ", local_position " << local_position_;
    if( local_position_ > 0 ) { // skip unless we have something to write
      // pad remaining bytes with zeros
      for( int i = local_position_; i < local_buffer_size_; ++i ) {
        localbuf_[i] = 0;
      }
      DVLOG(5) << "After padding, " << this << " had position " << position_ << ", local_position " << local_position_;
      
      if( !buffer_ ) {
        new_buffer();
      }

      // write out full cacheline
      __m128i * src = reinterpret_cast<__m128i*>( &localbuf_[0] );
      __m128i * dst = reinterpret_cast<__m128i*>( &buffer_[position_] );
#ifdef USE_NT_OPS
      _mm_stream_si128( dst+0, *(src+0) );
      _mm_stream_si128( dst+1, *(src+1) );
      _mm_stream_si128( dst+2, *(src+2) );
      _mm_stream_si128( dst+3, *(src+3) );
#else
      _mm_store_si128( dst+0, *(src+0) );
      _mm_store_si128( dst+1, *(src+1) );
      _mm_store_si128( dst+2, *(src+2) );
      _mm_store_si128( dst+3, *(src+3) );
#endif

      position_ += local_buffer_size_; // advance to next cacheline of output buffer
      local_position_ = 0;
      DVLOG(5) << "After flush, " << this << " had position " << position_ << ", local_position " << local_position_;
    }
    return position_;
  }

  /// Enqueue data into buffer.
  inline int enqueue( uint8_t * p, int size, bool header = false ) {
    DVLOG(5) << "Enqueuing " << size << " bytes to " << this << " from " << (void*) p << ", position is " << position_ << " and local position is " << local_position_;

    // start with offset if necessary
    if( (initial_offset_) && (position_ == 0) && (local_position_ == 0) ) {
      DVLOG(5) << this << " enqueuing initial offset of " << initial_offset_;
      uint8_t offset[ initial_offset_ ];
      actual_enqueue( &offset[0], initial_offset_ ); // add space for header
    }

    // if we're enqueuing a header, make sure it's 8-byte aligned.
    if( header ) {
      DVLOG(5) << "Checking alignment for header: local position is " << local_position_ << " , rounded " << round_up_to_n<8>( local_position_ );
      int align_bytes = round_up_to_n<8>( local_position_ ) - local_position_;
      if( align_bytes > 0 ) {
        DVLOG(5) << this << " enqueuing " << align_bytes << " alignment bytes.";
        static uint8_t zeros[ 64 ] = { 0 };
        actual_enqueue( &zeros[0], align_bytes );
      }
    }
      
    // Now enqueue the actual data.
    return actual_enqueue( p, size );
  }

} __attribute__((aligned(64)));

/// Adds data to a non-temporal message buffer with appropriate
/// prefetches to avoid polluting the cache. Optionally marks this data
/// as a header to be returned by the nt_get_previous() call.
template< typename T >
static inline int nt_enqueue( NTBuffer * b, T * p, int size, bool header = false ) {
#ifdef USE_NT_OPS
  /// Mark NTBuffer struct as non-temporal
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<uint8_t*>(b)+64, _MM_HINT_NTA );
#endif
  return b->enqueue( reinterpret_cast<uint8_t*>(p), size, header );
}

/// Flushes NTBuffer temporary aggregation buffer to main aggregation
/// in preparation for sending the buffer.
static inline int nt_flush( NTBuffer * b ) {
#ifdef USE_NT_OPS
  /// Mark NTBuffer struct as non-temporal
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<uint8_t*>(b)+64, _MM_HINT_NTA );
#endif
  return b->flush();
}

/// If NTBuffer is non-empty, returns pointer to last enqueued header
/// for use in message compression tests.
static inline void * nt_get_previous( NTBuffer * b ) {
#ifdef USE_NT_OPS
  /// Mark NTBuffer struct as non-temporal
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<uint8_t*>(b)+64, _MM_HINT_NTA );
#endif
  return nullptr; //b->get_previous();
}


} // namespace impl
} // namespace Grappa
