/*
 * Example of:
 * (partial) triangle listing
 *
 * A(x,z) :- R(x,y), R(y,z), R(z,x)
 */


// graph includes
#include "grappa/graph.hpp"

#include "squares.hpp"

// Grappa includes
#include <Grappa.hpp>
#include "MatchesDHT.hpp"
#include <Cache.hpp>
#include <ParallelLoop.hpp>
#include <GlobalCompletionEvent.hpp>
#include <AsyncDelegate.hpp>
#include <Metrics.hpp>
#include <FullEmpty.hpp>




using namespace Grappa;

namespace SquareQuery_ {
  GlobalAddress<Graph<Vertex>> E1_index, E2_index, E3_index, E4_index;
}

using namespace SquareQuery_;

void SquareQuery::preprocessing(std::vector<tuple_graph> relations) {
  // the index on a->[b] is not part of the query; just to clean tuples
  // so it is in the untimed preprocessing step
  
  auto e1 = relations[0];

  FullEmpty<GlobalAddress<Graph<Vertex>>> f1;
  spawn( [&f1,e1] {
      f1.writeXF( Graph<Vertex>::create(e1, /*directed=*/true) );
      });
  auto l_E1_index = f1.readFE();
  
  on_all_cores([=] {
      E1_index = l_E1_index;
  });
}

void SquareQuery::execute(std::vector<tuple_graph> relations) {
  auto e2 = relations[1];
  auto e3 = relations[2];
  auto e4 = relations[3];

  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";

  double start, end;
  start = Grappa::walltime();



  FullEmpty<GlobalAddress<Graph<Vertex>>> f2;
  spawn( [&f2,e2] {
      f2.writeXF( Graph<Vertex>::create(e2, /*directed=*/true) );
      });
  // TODO: the create() calls should be in parallel but,
  // create() uses allreduce, which currently requires a static var :(
  // Fix with a symmetric alloc in allreduce
  auto l_E2_index = f2.readFE();

  FullEmpty<GlobalAddress<Graph<Vertex>>> f3;
  spawn( [&f3,e3] {
      f3.writeXF( Graph<Vertex>::create(e3, /*directed=*/true) );
      });
  auto l_E3_index = f3.readFE();

  FullEmpty<GlobalAddress<Graph<Vertex>>> f4;
  spawn( [&f4,e4] {
      f4.writeXF( Graph<Vertex>::create(e4, /*directed=*/true) );
      });
  auto l_E4_index = f4.readFE();

  //auto l_E1_index = f1.readFE();
  //auto l_E2_index = f2.readFE();
  //auto l_E3_index = f3.readFE();
  //auto l_E4_index = f4.readFE();
 

  // broadcast index addresses to all cores
  on_all_cores([=] {
      E2_index = l_E2_index;
      E3_index = l_E3_index;
      E4_index = l_E4_index;
      });

  total_edges = E1_index->nadj + E2_index->nadj + E3_index->nadj + E4_index->nadj;

  end = Grappa::walltime();

  VLOG(1) << "insertions: " << (e2.nedge+e3.nedge+e4.nedge)/(end-start) << " per sec";
  index_runtime = end - start;

// #if DEBUG
//   printAll(E1_index);
// #endif

  //
  //TODO: there are plenty of corrections/ optimizations to do here
  // 1. captures by reference in the forall_here bodies
  // 2. moving data that is needed only
  // 3. selection by < degree
  //

  // the forall(forall(...)) here is really just a scan of the E1 edge relation
  forall( E1_index->vs, E1_index->nv, [](int64_t ai, Vertex& a) {
      forall_here<async,&impl::local_gce>( 0, a.nadj, [a,ai](int64_t start, int64_t iters) {
        for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
        ir1_count++; // count(E1)
        auto b_ind = a.local_adj[i];
        auto b_ptr = E2_index->vs + b_ind;
        edges_transfered++;
        // lookup b vertex
        spawnRemote(b_ptr.core(), [ai,b_ptr] {
          auto b = *(b_ptr.pointer());
          ir2_count += b.nadj; // count(E1xE2)
          // forall neighbors of b
          forall_here<async,&impl::local_gce>( 0, b.nadj, [ai,b](int64_t start, int64_t iters) {
            for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
            auto c_ind = b.local_adj[i];
            auto c_ptr = E3_index->vs + c_ind;
            edges_transfered++;
            // lookup c vertex
            spawnRemote(c_ptr.core(), [ai,b,c_ptr] {
              auto c = *(c_ptr.pointer());
              ir4_count += c.nadj; // count(E1xE2xE3)
              // forall neighbors of c
              forall_here<async,&impl::local_gce>( 0, c.nadj, [ai,b,c](int64_t start, int64_t iters) {
                for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
                auto d_ind = c.local_adj[i];
                auto d_ptr = E4_index->vs + d_ind;
                edges_transfered++;
                // lookup d vertex
                spawnRemote(d_ptr.core(), [ai,b,c,d_ptr] {
                  auto d = *(d_ptr.pointer());
                  ir6_count += d.nadj; // count(E1xE2xE3xE4)
                  // forall neighbors of d
                  forall_here<async,&impl::local_gce>( 0, d.nadj, [ai,b,c,d](int64_t start, int64_t iters) {
                    for ( int64_t i=start; i<start+iters; i++ ) { //forall_here_async serialized 
                    auto aprime = d.local_adj[i];
                    // check if the a's match
                    if ( aprime == ai ) {
                    results_count++;
                    }
                    }}); // over d->a
                  }); // lookup d
                }}); // over c->d
            }); // lookup c
            }}); // over b->c
        }); // lookup b
        }});}); // over a->b

}


