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
    LOG(INFO) << "Hello";
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

