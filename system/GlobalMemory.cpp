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

#include "GlobalMemory.hpp"

GlobalMemory * global_memory = NULL;

/// round up address to 4KB page alignment
/// TODO: why did I do it this way?
size_t round_up_page_size( size_t s ) {
  // TODO: fix this for huge pages
  const size_t page_size = 1 << 12;
  size_t new_s = s;

  if( s < page_size ) {
    new_s = page_size;
  } else {
    size_t diff = s % page_size;
    if( 0 != diff ) {
      new_s = s + page_size - diff;
    }
  }

  DVLOG(5) << "GlobalMemory rounding up from " << s << " to " << new_s;
  return new_s;
}

/// Construct local aspect of global memory
GlobalMemory::GlobalMemory( size_t total_size_bytes )
  : size_per_node_( round_up_page_size( total_size_bytes / Grappa::cores() ) )
  , chunk_( size_per_node_ )
  , allocator_( chunk_.global_pointer(), size_per_node_ * Grappa::cores() )
{ 
  assert( !global_memory );
  global_memory = this;
  DVLOG(1) << "Initialized GlobalMemory with " << size_per_node_ * Grappa::cores() << " bytes of shared heap.";
}
