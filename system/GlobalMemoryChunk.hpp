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

#ifndef __GLOBAL_MEMORY_CHUNK_HPP__
#define __GLOBAL_MEMORY_CHUNK_HPP__

#include "Addressing.hpp"

/// One processes' chunk of the global memory.
class GlobalMemoryChunk
{
private:
  size_t size_;
  void * memory_;

public:
  GlobalMemoryChunk( size_t size );
  ~GlobalMemoryChunk();

  void * local_pointer() const {
    return memory_;
  }

  GlobalAddress< void > global_pointer()  {
    return make_linear( memory_ );
  }
};

namespace Grappa {
namespace impl {
extern void * global_memory_chunk_base;
}
}

#endif
