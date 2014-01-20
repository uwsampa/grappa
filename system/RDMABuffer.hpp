// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#pragma once

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "Communicator.hpp"

///DECLARE_int64( buffer_size );
#define BUFFER_SIZE (1 << 16)


namespace Grappa {
  
/// @addtogroup Synchronization
/// @{
  
namespace impl {

/// Buffer for RDMA messaging. 
class RDMABuffer {
private:
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

  char data_[ BUFFER_SIZE - sizeof( raw_ ) - sizeof( raw2_ ) ];
  
public:
  RDMABuffer()
    : dest_( -1 )
    , next_( 0 )
    , source_( -1 )
    , ack_( 0 )
    , data_()
  { 
    static_assert( sizeof(*this) == BUFFER_SIZE, "RDMABuffer is not the size I expected for some reason." );
  }
  
  inline uint16_t * get_counts() { return reinterpret_cast< uint16_t* >( &data_[0] ); }
  inline char * get_payload() { 
    int payload_offset = Grappa::locale_cores() * sizeof( uint16_t );
    return &data_[payload_offset]; 
  }


  inline char * get_base() { return reinterpret_cast< char * >( this ); }
  inline size_t get_base_size() { 
    int payload_offset = Grappa::locale_cores() * sizeof( uint16_t );
    return &data_[payload_offset] - reinterpret_cast< char * >( this ); 
  }

  // assumes layout makes sense
  inline size_t get_max_size() { return BUFFER_SIZE - get_base_size(); }

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

