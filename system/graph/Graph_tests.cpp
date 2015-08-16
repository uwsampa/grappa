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

#include <boost/test/unit_test.hpp>
#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <GlobalVector.hpp>

BOOST_AUTO_TEST_SUITE( Graph_tests );

using namespace Grappa;
using Grappa::wait;

struct VData {
  VertexID parent;
};

struct EData {
  double weight;
};

using MyGraph = Graph<VData,EData>;

GlobalCompletionEvent c;

int64_t count;

DEFINE_int32(scale, 10, "Log2 number of vertices.");

GRAPPA_DEFINE_METRIC(SummarizingMetric<int64_t>, degree, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, edge_weight, 0);

BOOST_AUTO_TEST_CASE( test1 ) {
  init( GRAPPA_TEST_ARGS );
  run([]{
    int64_t total;
    int scale = 10;
    int64_t nv = 1L << FLAGS_scale;
    size_t ne = nv * 16;
    
    TupleGraph tg;
    tg = TupleGraph::Kronecker(scale, ne, 11111, 22222);
    
    BOOST_CHECK_EQUAL(tg.nedge, ne);
    
    // check all vertices are in correct range
    
    forall(tg.edges, tg.nedge, [nv](TupleGraph::Edge& e){
      for (auto v : {e.v0, e.v1}) BOOST_CHECK( v >= 0 && v < nv);
    });
    
    auto g = MyGraph::create(tg);
    
    BOOST_CHECK( g->nv <= nv );
    
    forall(g, [](MyGraph::Vertex& v){ degree += v.nadj; });
    
    ////////////////////////////////////////////
    // make sure adj() iterator gets every edge
    call_on_all_cores([]{ count = 0; });    
    forall(g, [g](MyGraph::Vertex& v){
      auto n = v.nadj;
      forall<async>(adj(g,v), [n](int64_t i){
        CHECK_LT(i, n);
        count++;
      });
    });
    total = reduce<int64_t,collective_add>(&count);
    CHECK_EQ(total, g->nadj);
    
    //////////////////////////////
    // test forall(Vertex&,Edge&)
    call_on_all_cores([]{ count = 0; });
    forall(g, [](MyGraph::Vertex& v, MyGraph::Edge& e){
      count++;
      edge_weight += e->weight;
    });
    
    ///////////////////////////
    // test 'transform'
    struct Data { int64_t parent; double w; };
    auto g2 = g->transform<Data>([](MyGraph::Vertex& v, Data& d){
      d.parent = -1;
      d.w = 1.0 / v.nadj;
    });
    
    using G2 = Graph<Data,EData>;
    
    ///////////////////////////////////
    // check again with custom joiner
    call_on_all_cores([]{ count = 0; });
    forall<&c>(g2, [g2](G2::Vertex& v){
      auto nadj = v.nadj;
      auto nv = g2->nv;
      
      forall<async,&c>(adj(g2,v), [nv](G2::Edge& e){
        auto j = e.id;
        CHECK(j < nv && j >= 0) << "=> " << j << ", " << nv;
        count++;
      });
      
      forall<async,&c>(adj(g2,v), [nadj](int64_t i, G2::Edge& e){
        CHECK(i < nadj && i >= 0) << "=> " << i << ", " << nadj;
        count++;
      });
            
    });
    total = reduce<int64_t,collective_add>(&count);
    CHECK_EQ(total, g->nadj * 2);
    
    //////////////////////////////////////////////////////
    // check again, running forall(adj) on different core
    call_on_all_cores([]{ count = 0; });
    auto q = GlobalVector<int64_t>::create(g->nv);
    
    forall<&c>(g2, [g2,q](VertexID vi, G2::Vertex& v){
      q->push(vi);
    });
    
    CHECK_EQ(q->size(), g->nv);
    
    int64_t _c = 0;
    auto counter = make_global(&_c);
    forall<&c>(q, [g2,counter](int64_t& v){
      auto n = g2->nv;
      forall<async,&c>(adj(g2,g2->vs+v), [counter,n](G2::Edge& e){
        auto vj = e.id;
        CHECK(vj < n && vj >= 0) << "=> " << vj << ", " << n;
        count++;
        delegate::fetch_and_add(counter, 1);
      });
    });
    total = reduce<int64_t,collective_add>(&count);
    CHECK_EQ(total, g->nadj);
    CHECK_EQ(_c, g->nadj);
    
    struct BigData { double v[1024]; };
    auto g3 = g2->transform<BigData>([](G2::Vertex& v, BigData& d){
      for (int i=0; i<1024; i++) { d.v[i] = 0.2; }
    });
    
    forall(g3, [](Graph<BigData,EData>::Vertex& v){
      double total = 0.0;
      for (int i=0; i<1024; i++) { total += v->v[i]; }
      count += (total > 0);
    });
    
    LOG(INFO) << degree;
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}

BOOST_AUTO_TEST_SUITE_END();
