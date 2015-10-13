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

#define BUFFER_SIZE (1 << 19)

namespace Grappa {
namespace impl {

class NTBuffer {
  static const int local_buffer_size = 8;
  static const int last_position = 7;
  static int initial_offset;
  
  uint64_t localbuf_[ local_buffer_size ];

  // TODO: squeeze these all into one 64-bit word (the last word of the buffer)
  //
  // Buffer_ must be 64-byte aligned, so we only need 42 bits max of
  // address.
  //
  // Position_ and local_position_ count 8-byte words, and can be
  // combined (masking off the lower three bits for the local part).
  //
  // (or if 64 bits is not enough, messages could be multiples of 16 bytes instead of 8)
  uint64_t * buffer_;
  int position_;
  int local_position_;
  
public:
  NTBuffer()
    : localbuf_()
    , buffer_( nullptr )
    , position_( 0 )
    , local_position_( 0 )
  { }

  inline bool empty() const { return position_ == 0 && local_position_ == 0; }
  
  static void set_initial_offset( int words ) {
    initial_offset = words;
  }

  void new_buffer( ) {
    DVLOG(5) << "Allocating new buffer for " << this;
    CHECK( posix_memalign( reinterpret_cast<void**>(&buffer_), 64, BUFFER_SIZE ) == 0 )
      << "posix_memalign error: buffer allocation failed";
    position_ = 0;
  }
  
  std::tuple< void *, int > take_buffer() {
    flush();
#ifdef USE_NT_OPS
    _mm_sfence();
#endif
    auto retval = std::make_tuple( reinterpret_cast<void*>(buffer_), position_ * sizeof(uint64_t) );
    buffer_ = nullptr;
    position_ = 0;
    DCHECK_EQ( local_position_, 0 );
    return retval;
  }
  
  int flush() {
    DVLOG(5) << "Flushing " << this << " with position " << position_ << ", local_position " << local_position_;
    if( local_position_ > 0 ) { // skip unless we have something to write
      for( int i = local_position_; i < local_buffer_size; ++i ) {
        localbuf_[i] = 0;
      }
      
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

      position_ += local_buffer_size; // advance to next cacheline of output buffer
      local_position_ = 0;
    }
    return position_ * sizeof(uint64_t);
  }

  inline int actual_enqueue( uint64_t * word_p, int word_size ) {
    // now copy in message
    while( word_size > 0 ) {
      localbuf_[ local_position_ ] = *word_p;
      ++word_p;
      --word_size;
      ++local_position_;

      if( local_position_ == local_buffer_size ) {
        flush();
      }
    }
    DVLOG(5) << "After enqueue, position is " << position_ << " and local position is " << local_position_;
    return (position_ + local_position_) * sizeof(uint64_t);
  }
  
  inline int enqueue( uint64_t * word_p, int word_size ) {
    DVLOG(5) << "Enqueuing " << word_size << " words to " << this << " from " << word_p << ", position is " << position_ << " and local position is " << local_position_;

    // start with offset if necessary
    if( (position_ == 0) && (local_position_ == 0) ) {
      uint64_t offset[ initial_offset ];
      actual_enqueue( &offset[0], initial_offset ); // add space for header
    }
    return actual_enqueue( word_p, word_size );
  }

} __attribute__((aligned(64)));

template< typename T >
static inline int nt_enqueue( NTBuffer * b, T * p, int size ) {
#ifdef USE_NT_OPS
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<char*>(b)+64, _MM_HINT_NTA );
#endif
  DCHECK_EQ( reinterpret_cast<uint64_t>(p) % 8, 0 ) << "Pointer must be 8-byte aligned";
  DCHECK_EQ( size % 8, 0 ) << "Size must be a multiple of 8";
  return b->enqueue( reinterpret_cast<uint64_t*>(p), size/8 );
}

static inline int nt_flush( NTBuffer * b ) {
#ifdef USE_NT_OPS
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<char*>(b)+64, _MM_HINT_NTA );
#endif
  return b->flush();
}

} // namespace impl
} // namespace Grappa
