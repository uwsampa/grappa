#pragma once
////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
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
  NTBuffer * next_mru_;
  NTBuffer * prev_mru_;
  
public:
  NTBuffer()
    : localbuf_()
    , buffer_( nullptr )
    , position_( 0 )
    , local_position_( 0 )
    , next_mru_( nullptr )
    , prev_mru_( nullptr )
  { }

  static void set_initial_offset( int words ) {
    initial_offset = words;
  }

  inline NTBuffer * get_next_mru() const { return next_mru_; }

  inline void maybe_update_mru( NTBuffer ** mru_root ) {
    DVLOG(5) << "Maybe adding " << this << " to mru list starting at " << *mru_root;
    if( (*mru_root != this) && (prev_mru_ == nullptr) ) { // not already in list
      next_mru_ = *mru_root;
      *mru_root = this;
    }
  }

  inline void remove_from_mru( NTBuffer ** mru_root ) {
    DVLOG(5) << "Removing " << this << " from mru list starting at " << *mru_root;
    if( *mru_root == this ) {
      *mru_root = next_mru_;
    }
    if( next_mru_ ) {
      _mm_prefetch( &(next_mru_->prev_mru_), _MM_HINT_NTA );
      next_mru_->prev_mru_ = prev_mru_;
      next_mru_ = nullptr;
    }
    if( prev_mru_ ) {
      _mm_prefetch( &(prev_mru_->next_mru_), _MM_HINT_NTA );
      prev_mru_->next_mru_ = next_mru_;
      prev_mru_ = nullptr;
    }
  }

  void new_buffer( ) {
    posix_memalign( reinterpret_cast<void**>(&buffer_), 64, BUFFER_SIZE );
    position_ = 0;
  }
  
  std::tuple< void *, int > take_buffer() {
    flush();
    _mm_sfence();
    auto retval = std::make_tuple( reinterpret_cast<void*>(buffer_), position_ * sizeof(uint64_t) );
    buffer_ = nullptr;
    position_ = 0;
    return retval;
  }
  
  int flush() {
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
      _mm_stream_si128( dst+0, *(src+0) );
      _mm_stream_si128( dst+1, *(src+1) );
      _mm_stream_si128( dst+2, *(src+2) );
      _mm_stream_si128( dst+3, *(src+3) );

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
    DVLOG(5) << "Enqueuing " << word_size << " words from " << word_p << ", position is " << position_ << " and local position is " << local_position_;

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
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<char*>(b)+64, _MM_HINT_NTA );
  DCHECK_EQ( reinterpret_cast<uint64_t>(p) % 8, 0 ) << "Pointer must be 8-byte aligned";
  DCHECK_EQ( size % 8, 0 ) << "Size must be a multiple of 8";
  return b->enqueue( reinterpret_cast<uint64_t*>(p), size/8 );
}

static inline int nt_flush( NTBuffer * b ) {
  _mm_prefetch( b, _MM_HINT_NTA );
  _mm_prefetch( reinterpret_cast<char*>(b)+64, _MM_HINT_NTA );
  return b->flush();
}

} // namespace impl
} // namespace Grappa
