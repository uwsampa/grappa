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
#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"

///DECLARE_int64( buffer_size );
#define BUFFER_SIZE (1 << 19)


namespace Grappa {
  
/// @addtogroup Synchronization
/// @{
  
namespace impl {

/// Buffer for RDMA messaging. 
class RDMABuffer {
public:
  void * deserializer;

  union {
    struct {
      Core source_ : 16;
      intptr_t next_ : 48;
    };
    intptr_t raw_;
  };

  union {
    struct {
      Core dest_ : 16;
      intptr_t ack_ : 48;
    };
    intptr_t raw2_;
  };

  char data_[ BUFFER_SIZE - sizeof(void*) - sizeof( raw_ ) - sizeof( raw2_ ) - sizeof(CommunicatorContext) ];
  
  CommunicatorContext context;

  
  RDMABuffer()
    : deserializer(NULL)
    , dest_( -1 )
    , next_( 0 )
    , source_( -1 )
    , ack_( 0 )
    , data_()
    , context()
  {
    static_assert( sizeof(*this) == BUFFER_SIZE, "RDMABuffer is not the size I expected for some reason." );
    context.buf = (void*) this;
    context.size = get_max_size();
  }

  inline uint32_t * get_counts() { return reinterpret_cast< uint32_t* >( &data_[0] ); }
  inline char * get_payload() { 
    int payload_offset = Grappa::locale_cores() * sizeof( uint32_t );
    return &data_[payload_offset]; 
  }


  inline char * get_base() { return reinterpret_cast< char * >( this ); }
  inline size_t get_base_size() { 
    int payload_offset = Grappa::locale_cores() * sizeof( uint32_t );
    return &data_[payload_offset] - reinterpret_cast< char * >( this ); 
  }

  // assumes layout makes sense
  inline size_t get_max_size() { return BUFFER_SIZE - get_base_size() - sizeof(CommunicatorContext); }

  inline RDMABuffer * get_ack() { return reinterpret_cast< RDMABuffer * >( ack_ ); }
  inline void set_ack( RDMABuffer * ack ) { ack_ = reinterpret_cast< intptr_t >( ack ); }
  
  inline Core get_source() { return source_; }
  //inline void set_core( Core c ) { LOG(INFO) << this << " changed from " << core_ << " to " << c; core_ = c; }
  inline void set_source( Core c ) { source_ = c; }
  
  inline Core get_dest() { return dest_; }
  //inline void set_core( Core c ) { LOG(INFO) << this << " changed from " << core_ << " to " << c; core_ = c; }
  inline void set_dest( Core c ) { dest_ = c; }
  
  inline RDMABuffer * get_next() { return reinterpret_cast< RDMABuffer * >( next_ ); }
  inline void set_next( RDMABuffer * next ) { next_ = reinterpret_cast< intptr_t >( next ); }
};

}

/// @}

}

