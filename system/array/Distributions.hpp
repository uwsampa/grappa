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

namespace Grappa {

namespace Distribution {

struct Block {
  size_t element_size;
  size_t size;
  size_t per_core;
  size_t max;

  Block(): element_size(0), size(0), per_core(0), max(0) {}
  void set(size_t elem_size, size_t n) {
    max = n;
    element_size = elem_size;
    size_t cores = Grappa::cores();
    per_core = n / cores;
    size = per_core * element_size;
  }
  size_t core_offset_for_index( size_t index ) {
    return index / per_core;   // remember to mod
  }
  size_t offset_for_index( size_t index ) {
    return index % per_core;
  }
};

template< int block_size >
struct BlockCyclic {

  size_t element_size;
  size_t size;
  size_t per_core;
  
  BlockCyclic(): element_size(0), size(0), per_core(0) {}
  void set(size_t elem_size, size_t n) {
    element_size = elem_size;
    size_t cores = Grappa::cores();
    per_core = (n / block_size) / cores;
    size = per_core * element_size;
  }
  size_t core_offset_for_index( size_t index ) {
    return index / block_size;  // remember to mod
  }
  size_t offset_for_index( size_t index ) {
    return index % block_size;
  }
};

struct Local {

  size_t element_size;
  size_t size;
  size_t per_core;
  
  Local(): element_size(0), size(0), per_core(0) {}
  void set(size_t elem_size, size_t n) {
    element_size = elem_size;
    per_core = n;
    size = per_core * element_size;
  }
  size_t core_offset_for_index( size_t index ) {
    return 0;
  }
  size_t offset_for_index( size_t index ) {
    return index;
  }
};

}

}
