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

