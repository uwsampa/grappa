#pragma once

// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
      Core core_ : 16;
      intptr_t next_ : 48;
    };
    intptr_t raw_;
  };
  RDMABuffer * ack_;
  
  char data_[ BUFFER_SIZE - sizeof( raw_ ) - sizeof( ack_ ) ];
  
  friend class RDMABufferList;
  
public:
  RDMABuffer()
    : core_( -1 )
    , next_( 0 )
    , ack_( NULL )
    , data_()
  { 
    static_assert( sizeof(*this) == BUFFER_SIZE, "RDMABuffer is not the size I expected for some reason." );
  }
  
  inline size_t get_max_size() { return BUFFER_SIZE - sizeof( raw_ ) - sizeof( ack_ ); }
  inline char * get_payload() { return &data_[0]; }
  inline char * get_base() { return reinterpret_cast< char * >( this ); }

  inline RDMABuffer * get_ack() { return ack_; }
  inline RDMABuffer * set_ack( RDMABuffer * ack ) { ack_ = ack; }
  
  inline Core get_core() { return core_; }
  //inline void set_core( Core c ) { LOG(INFO) << this << " changed from " << core_ << " to " << c; core_ = c; }
  inline void set_core( Core c ) { core_ = c; }
  
  inline RDMABuffer * get_next() { return reinterpret_cast< RDMABuffer * >( next_ ); }
  inline void set_next( RDMABuffer * next ) { next_ = reinterpret_cast< intptr_t >( next ); }
};

}

/// @}

}

