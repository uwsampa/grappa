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
