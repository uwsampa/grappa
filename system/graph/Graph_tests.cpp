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

#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <graph/Graph.hpp>

BOOST_AUTO_TEST_SUITE( Graph_tests );

using namespace Grappa;
using Grappa::wait;

int64_t count;

GRAPPA_DEFINE_METRIC(SummarizingMetric<int64_t>, degree, 0);

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    
    int scale = 10;
    int64_t nv = 1L << scale;
    size_t ne = nv * 8;
    
    auto tg = TupleGraph::Kronecker(scale, ne, 11111, 22222);
    
    BOOST_CHECK_EQUAL(tg.nedge, ne);
    
    // check all vertices are in correct range
    
    forall(tg.edges, tg.nedge, [nv](TupleGraph::Edge& e){
      for (auto v : {e.v0, e.v1}) BOOST_CHECK( v >= 0 && v < nv);
    });
    
    auto g = Graph<>::create(tg);
    
    BOOST_CHECK( g->nv <= nv );
    
    forall(g, [](Vertex& v){ degree += v.nadj; });
    
    call_on_all_cores([]{ count = 0; });
    forall(g, [](int64_t src, int64_t dst){
      if ( src == dst ) count++;
    });
    
    LOG(INFO) << "self edges: " << reduce<int64_t,collective_add>(&count);
    
    LOG(INFO) << degree;
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
