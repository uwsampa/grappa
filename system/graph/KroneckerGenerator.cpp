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
