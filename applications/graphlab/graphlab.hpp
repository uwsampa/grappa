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

////////////////////////////////////////////////////////////////////////
/// GraphLab is an API and runtime system for graph-parallel computation.
/// This is a rough prototype implementation of the programming model to
/// demonstrate using Grappa as a platform for other models.
/// More information on the actual GraphLab system can be found at:
/// graphlab.org.
////////////////////////////////////////////////////////////////////////

#pragma once
#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <Reducer.hpp>
#include <SmallLocalSet.hpp>
#include <GlobalHashMap.hpp>

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <numeric>
using std::unordered_set;
using std::unordered_map;
using std::vector;


using namespace Grappa;
using delegate::call;

static const Core INVALID = -1;

GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, iteration_time);

DECLARE_int32(max_iterations);


/////////////////////////////////////////////////////////
// local helper macros (make sure to un-def them after)
#define LOG_ALL_CORES(NAME, TYPE, WHAT) \
      if (VLOG_IS_ON(2)) { \
        barrier(); \
        if (mycore() == 0) { \
          std::vector<TYPE> lst; lst.resize(cores()); \
          for (Core c = 0; c < cores(); c++) { \
            lst[c] = delegate::call(c, [=]{ \
              return (WHAT); \
            }); \
          } \
          std::cerr << util::array_str<16>(NAME, lst) << "\n"; \
        } \
        barrier(); \
      }
#define PHASE_BEGIN(TITLE) if (mycore() == 0) { VLOG(1) << TITLE; t = walltime(); }
#define PHASE_END() if (mycore() == 0) VLOG(1) << "    (" << walltime()-t << " s)"

/// Base class for using Grappa's GraphLab API.
/// 
/// To use, define your Vertex Program as a subclass of this, providing the Graph
/// type and Gather type, and defining gather, apply, and scatter operations.
/// 
/// @tparam G           Graph type
/// @tparam GatherType  Type used in Gather operation (for use with delta cache)
/// 
template< typename G, typename GatherType >
struct GraphlabVertexProgram {
  using Vertex = typename G::Vertex;
  using Edge = typename G::Edge;
  using Gather = GatherType;

  GatherType cache;

  GraphlabVertexProgram(): cache() {}

  void post_delta(GatherType d) { cache += d; }
  void reset() { cache = GatherType(); }
};

#include "graphlab_naive.hpp"
#include "graphlab_splitv.hpp"

#undef PHASE_BEGIN
#undef PHASE_END
