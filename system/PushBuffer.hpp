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

#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"

/// A simple class for staging appends to a global array. Created for use in adding things 
/// to the frontier in BFS. The idea is to have a PushBuffer local to each node (in a 
/// global variable) that can be appended to. Once the buffer reaches capacity, a single 
/// fetch_add is used to allocate space in the global array, and a large Cache 
/// Incoherent::WO release is used to save everything into the global array.
///
/// @tparam NBUFS Supports having multiple outstanding buffers to prevent having to wait 
///               for release to finish.
template<typename T, size_t BUFSIZE=(1L<<14), size_t NBUFS=4 >
struct PushBuffer {
  GlobalAddress<T> target_array;
  GlobalAddress<int64_t> shared_index;
  T* buf[NBUFS];
  int curr_buf;
  int64_t curr_size[NBUFS];
  
  PushBuffer(): curr_buf(0) {
    for (int i=0; i<NBUFS; i++) buf[i] = nullptr;
  }
  ~PushBuffer() {
    flush();
    for (int i=0; i<NBUFS; i++) Grappa::impl::locale_shared_memory.deallocate(buf[i]);
  }
  void push(const T& o) {
    CHECK(target_array.pointer() != NULL) << "buffer not initialized!";
    buf[curr_buf][curr_size[curr_buf]] = o;
    curr_size[curr_buf]++;
    CHECK(curr_size[curr_buf] <= BUFSIZE);
    if (curr_size[curr_buf] == BUFSIZE) {
      flush();
    }
  }
  void setup(GlobalAddress<T> _target_array, GlobalAddress<int64_t> _shared_index) {
    if (buf[0] == nullptr) {
      for (int64_t i=0; i<NBUFS; i++) {
        buf[i] = static_cast<T*>(Grappa::impl::locale_shared_memory.allocate(BUFSIZE * sizeof(T)));
        curr_size[i] = 0;
      }
    }
    
    target_array = _target_array;
    shared_index = _shared_index;
    for (int64_t i=0; i<NBUFS; i++) curr_size[i] = 0;
    curr_buf = 0;
  }
  void flush() {
    int save_buf = curr_buf;
    int64_t save_size = curr_size[curr_buf];
    
    while (curr_size[curr_buf] != 0) {
      curr_buf = (curr_buf + 1) % NBUFS;
      CHECK(curr_buf != save_buf) << "out of buffers in PushBuffer!";
    }
    
    if (save_size > 0) {
      int64_t offset = Grappa::delegate::fetch_and_add(shared_index, save_size);
      typename Incoherent<T>::WO c(target_array+offset, save_size, buf[save_buf]);
      c.block_until_released();
    }
    curr_size[save_buf] = 0;
  }
};

