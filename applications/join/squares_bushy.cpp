/*
 * Example of:
 * (partial) triangle listing
 *
 * A(x,z) :- R(x,y), R(y,z), R(z,x)
 */


// graph includes
#include "grappa/graph.hpp"

#include "squares_bushy.hpp"
#include "local_graph.hpp"
#include "utility.hpp"

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

namespace SquareBushyPlan_ {
  GlobalAddress<Graph<Vertex>> E1_index, E2_index, E3_index, E4_index;
  std::vector<Edge> localAssignedEdges_abc;
  std::vector<Edge> localAssignedEdges_cda;
};

using namespace SquareBushyPlan_;


void SquareBushyPlan::preprocessing(std::vector<tuple_graph> relations) {
  // the index on a->[b] is not part of the query; just to clean tuples
  // so it is in the untimed preprocessing step
  
  // for iterating over a-b
  auto e1 = relations[0];

  FullEmpty<GlobalAddress<Graph<Vertex>>> f1;
  spawn( [&f1,e1] {
      f1.writeXF( Graph<Vertex>::create(e1, /*directed=*/true) );
      });
  auto l_E1_index = f1.readFE();
  

  //for iterating over c-d
  auto e3 = relations[2];
  FullEmpty<GlobalAddress<Graph<Vertex>>> f3;
  spawn( [&f3,e3] {
      f3.writeXF( Graph<Vertex>::create(e3, /*directed=*/true) );
      });
  auto l_E3_index = f3.readFE();
      
  
  on_all_cores([=] {
      E1_index = l_E1_index;
      E3_index = l_E3_index;
  });

  
}

int64_t h(int64_t a, int64_t b) {
  const int64_t p = 179425579; // prime
  return (p * a + b) % cores();
}


void SquareBushyPlan::execute(std::vector<tuple_graph> relations) {
  auto e2 = relations[1];
  auto e4 = relations[3];

  // scan tuples and hash join col 1
  VLOG(1) << "Scan tuples, creating index on subject";

  double start, end;
  start = Grappa::walltime();



  // hash on (b)-c
  FullEmpty<GlobalAddress<Graph<Vertex>>> f2;
  spawn( [&f2,e2] {
      f2.writeXF( Graph<Vertex>::create(e2, /*directed=*/true) );
      });
  // TODO: the create() calls should be in parallel but,
  // create() uses allreduce, which currently requires a static var :(
  // Fix with a symmetric alloc in allreduce
  auto l_E2_index = f2.readFE();


  // hash on (d)-a
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
      E4_index = l_E4_index;
      });

  total_edges = E1_index->nadj + E2_index->nadj + E3_index->nadj + E4_index->nadj;

  end = Grappa::walltime();

  VLOG(1) << "insertions: " << (e2.nedge+e4.nedge)/(end-start) << " per sec";
  index_runtime = end - start;

// #if DEBUG
//   printAll(E1_index);
// #endif

  //
  //TODO: there are plenty of corrections/ optimizations to do here
  // 1. captures by reference in the forall_here bodies
  // 2. moving data that is needed only
  // 3. selection by < degree
 
  spawn<&impl::local_gce>([] {
  // finding the abc, and hashing to h(a,c)
  forall<async>( E1_index->vs, E1_index->nv, [](int64_t ai, Vertex& a) {
    VLOG(5) << "a iteration " << ai;
      forall_here<async>( 0, a.nadj, [a,ai](int64_t start, int64_t iters) {
        for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
        ir1_count++; // count(E1)
        auto b_ind = a.local_adj[i];
        auto b_ptr = E2_index->vs + b_ind;
        edges_transfered++;
        // lookup b vertex
        spawnRemote(b_ptr.core(), [ai,b_ptr,b_ind] {
          auto b = *(b_ptr.pointer());
          ir2_count += b.nadj; // count(E1xE2)
          // forall neighbors of b
          forall_here<async,&impl::local_gce>( 0, b.nadj, [ai,b,b_ind](int64_t start, int64_t iters) {
            for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
            auto c_ind = b.local_adj[i];
            auto owner = h(ai, c_ind);
            VLOG(5) << "abc: " << resultStr({ai,b_ind,c_ind});
            edges_transfered++;
            delegate::call<async>( owner, [ai,b,c_ind] {
              localAssignedEdges_abc.push_back( Edge(ai,c_ind) );
            });
            }
          });
        });
        }
     });
  }); 
  VLOG(4) << "abc task exiting";
  }); // private 


  spawn<&impl::local_gce>([] {
  // finding the cda, and hashing to h(a,c)
  forall<async>( E3_index->vs, E3_index->nv, [](int64_t ci, Vertex& c) {
    VLOG(5) << "c iteration " << ci;
      forall_here<async,&impl::local_gce>( 0, c.nadj, [c,ci](int64_t start, int64_t iters) {
        for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
        auto d_ind = c.local_adj[i];
        auto d_ptr = E4_index->vs + d_ind;
        edges_transfered++;
        // lookup d vertex
        spawnRemote(d_ptr.core(), [ci,d_ptr,d_ind] {
          auto d = *(d_ptr.pointer());
          ir3_count += d.nadj; // count(E3xE4)
          // forall neighbors of d
          forall_here<async,&impl::local_gce>( 0, d.nadj, [ci,d,d_ind](int64_t start, int64_t iters) {
            for ( int64_t i=start; i<start+iters; i++ ) { // forall_here_async serialized for
            auto a_ind = d.local_adj[i];
            auto owner = h(a_ind, ci);
            VLOG(5) << "cda: " << resultStr({ci,d_ind,a_ind});
            edges_transfered++;
            delegate::call<async>( owner, [ci,d,a_ind] {
              localAssignedEdges_cda.push_back( Edge(ci, a_ind) );
            });
            }
          });
        });
        }
     });
  }); 
  VLOG(4) << "cda task exiting";
  }); // private 

  impl::local_gce.wait();
  VLOG(1) << "done with lower tree";

  on_all_cores([] {
    std::unordered_set<pair_t> ac_b;

    // build (ac)->b
    for (auto e : localAssignedEdges_abc) {
      VLOG(5) << "final join build(a,c): " << e;
      ac_b.insert( pair_t(e.src, e.dst) );
    }

    // probe with (ac) of cda
    for (auto e : localAssignedEdges_cda) {
      VLOG(5) << "final join probe(c,a): " << e;
      if ( ac_b.end() != ac_b.find( pair_t(e.dst, e.src) ) ) {
        VLOG(5) << "result: " << "(" << e.dst << "," << e.src << ")";
        results_count++;
      }
    }
  }); 



}

