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

#include <infiniband/arch.h>
#include <rdma/rdma_cma.h>

namespace Grappa {
namespace impl {

class Verbs {
  struct ibv_device ** devices;
  int num_devices;
  
  struct ibv_device * device;
  struct ibv_device_attr device_attributes;

  uint8_t port;
  struct ibv_port_attr port_attributes;
  
  struct ibv_context * context;
  struct ibv_pd * protection_domain;

  struct ibv_cq * completion_queue;

  static const int completion_queue_depth = 256;
  static const int send_message_depth = 1; // using depth instead
  static const int receive_message_depth = 1;
  static const int scatter_gather_element_count = 1;
  static const int max_inline_data = 16; // message rate drops from 6M/s to 4M/s at 29 bytes
  static const int max_dest_rd_atomic = 16; // how many outstanding reads/atomic ops are allowed? (remote end of qp, limited by card)
  static const int max_rd_atomic = 16; // how many outstanding reads/atomic ops are allowed? (local end of qp, limited by card)
  static const int min_rnr_timer = 0x12;  // from Mellanox RDMA-Aware Programming manual
  static const int timeout = 0x12;  // from Mellanox RDMA-Aware Programming manual
  static const int retry_count = 6; // from Mellanox RDMA-Aware Programming manual
  static const int rnr_retry = 0; // from Mellanox RDMA-Aware Programming manual

  struct Endpoint {
    uint16_t lid;
    uint32_t qp_num;
    struct ibv_qp * queue_pair;
  };

  std::unique_ptr< Endpoint[] > endpoints;
  // struct ibv_mr * memory_region;
  // void * remote_address;
  // uint32_t remote_key;

  std::unique_ptr< struct ibv_recv_wr[] > bare_receives;

  void initialize_device();

  void connect();

  void finalize_device();

public:
  Verbs()
    : devices( NULL )
    , num_devices( 0 )
    , device( NULL )
    , device_attributes()
    , port( 0 )
    , port_attributes()
    , context( NULL )
    , protection_domain( NULL )
    , completion_queue( NULL )
  { }

  void init();

  void finalize();

  struct ibv_mr * register_memory_region( void * base, size_t size );

  void post_send( Core c, struct ibv_send_wr * wr );
  
  void post_receive( Core c, struct ibv_recv_wr * wr );

  int poll();
};

template< typename T >
struct RDMA_WR {
  struct ibv_sge sge;
  struct ibv_send_wr wr;
  T data;
} __attribute__ ((aligned (64)));

class RDMASharedMemory {
private:
  void * buf;
  size_t size;
  struct ibv_mr * mr;
  std::unique_ptr< uint32_t[] > rkeys;
  Verbs & ib;
public:
  RDMASharedMemory( Verbs & ib )
    : buf( NULL )
    , size( 0 )
    , mr( NULL )
    , rkeys()
    , ib( ib )
  { }

  ~RDMASharedMemory() {
    finalize();
  }
  
  void init( size_t newsize = 1L << 30 );

  void finalize();

  inline void * base() const { return buf; }
  inline int32_t rkey( Core c ) const { return rkeys[c]; }
  inline int32_t lkey() const { return mr->lkey; }
};

}
}
