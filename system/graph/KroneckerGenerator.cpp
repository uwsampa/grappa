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

#include "graph_generator.h"
#include "utils.h"
#include "TupleGraph.hpp"
#include "ParallelLoop.hpp"

namespace Grappa {

  TupleGraph TupleGraph::Kronecker(int scale, int64_t nedge, 
                                            uint64_t seed1, uint64_t seed2) {    
    static_assert(sizeof(TupleGraph::Edge) == sizeof(packed_edge),
                  "TupleGraph::Edge doesn't match Graph500 generator's.");
    
    TupleGraph tg(nedge);
    
    on_all_cores([tg,scale,seed1,seed2]{
      auto local_base = tg.edges.localize();
      auto local_end = (tg.edges+tg.nedge).localize();
      int64_t local_n = local_end - local_base;
      
      uint_fast32_t seed[5];
      make_mrg_seed(seed1, seed2, seed);
      
      auto start = local_n * mycore();
      auto end   = start + local_n;
      auto packed_edges = reinterpret_cast<packed_edge*>(local_base);
      generate_kronecker_range(seed, scale, start, end, packed_edges);
      
    });
    
    return tg;
  }
  
}
