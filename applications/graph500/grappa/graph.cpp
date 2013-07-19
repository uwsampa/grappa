#pragma once
#include "graph.hpp"

// #define USE_MPI3_COLLECTIVES
#undef USE_MPI3_COLLECTIVES
#ifdef USE_MPI3_COLLECTIVES
#include <mpi.h>
#endif

GlobalAddress<Graph> Graph::create(tuple_graph& tg) {
  double t;
  auto g = mirrored_global_alloc<Graph>();
  
  // find nv
      t = walltime();
  forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
    if (e.v0 > g->nv) { g->nv = e.v0; }
    else if (e.v1 > g->nv) { g->nv = e.v1; }
  });
  on_all_cores([g]{
    g->nv = Grappa::allreduce<int64_t,collective_max>(g->nv) + 1;
  });
      VLOG(1) << "find_nv_time: " << walltime() - t;
  
  auto vs = global_alloc<Vertex>(g->nv);
  auto self = g;
  on_all_cores([g,vs]{
    new (g.localize()) Graph(g, vs, g->nv);
    
#ifdef SMALL_GRAPH
    // g->scratch = locale_alloc<int64_t>(g->nv);
    if (locale_mycore() == 0) g->scratch = locale_alloc<int64_t>(g->nv);
    barrier();
    if (locale_mycore() == 0) {
      memset(g->scratch, 0, sizeof(int64_t)*g->nv);
    } else {
      g->scratch = delegate::call(mylocale()*locale_cores(), [g]{ return g->scratch; });
    }
    VLOG(0) << "locale = " << mylocale() << ", scratch = " << g->scratch;
#endif
  });
                                                            t = walltime();
  forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
    CHECK_LT(e.v0, g->nv); CHECK_LT(e.v1, g->nv);
#ifdef SMALL_GRAPH
    // g->scratch[e.v0]++;
    // g->scratch[e.v1]++;
    __sync_fetch_and_add(g->scratch+e.v0, 1);
    __sync_fetch_and_add(g->scratch+e.v1, 1);
#else    
    auto count = [](GlobalAddress<Vertex> v){
      delegate::call_async(*shared_pool, v.core(), [v]{ v->local_sz++; });
    };
    count(g->vs+e.v0);
    count(g->vs+e.v1);
#endif
  });
  VLOG(1) << "count_time: " << walltime() - t;
  
#ifdef SMALL_GRAPH
  t = walltime();  
#ifdef USE_MPI3_COLLECTIVES
  on_all_cores([g]{
    MPI_Request r; int done;
    MPI_Iallreduce(MPI_IN_PLACE, g->scratch, g->nv, MPI_INT64_T, MPI_SUM, MPI_COMM_WORLD, &r);
    do {
      MPI_Test( &r, &done, MPI_STATUS_IGNORE );
      if(!done) { Grappa_yield(); }
    } while(!done);
  });
#else
  on_all_cores([g]{ allreduce_inplace<int64_t,collective_add>(g->scratch, g->nv); });
#endif // USE_MPI3_COLLECTIVES
  VLOG(1) << "allreduce_inplace_time: " << walltime() - t;  
  // on_all_cores([g]{ VLOG(5) << util::array_str("scratch", g->scratch, g->nv, 25); });
#endif // SMALL_GRAPH  
  
  // allocate space for each vertex's adjacencies (+ duplicates)
  forall_localized(g->vs, g->nv, [g](int64_t i, Vertex& v) {
#ifdef SMALL_GRAPH
    // adjust b/c allreduce didn't account for having 1 instance per locale
    v.local_sz = g->scratch[i] / locale_cores();
#endif
    
    v.nadj = 0;
    if (v.local_sz > 0) v.local_adj = new int64_t[v.local_sz];
  });
  VLOG(3) << "after adj allocs";
  
  // scatter
  forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
    auto scatter = [g](int64_t vi, int64_t adj) {
      auto vaddr = g->vs+vi;
      delegate::call_async(*shared_pool, vaddr.core(), [vaddr,adj]{
        auto& v = *vaddr.pointer();
        v.local_adj[v.nadj++] = adj;
      });
    };
    scatter(e.v0, e.v1);
    scatter(e.v1, e.v0);
  });
  VLOG(3) << "after scatter, nv = " << g->nv;
  
  // sort & de-dup
  forall_localized(g->vs, g->nv, [g](int64_t vi, Vertex& v){
    CHECK_EQ(v.nadj, v.local_sz);
    std::sort(v.local_adj, v.local_adj+v.nadj);
    
    int64_t tail = 0;
    for (int64_t i=0; i<v.nadj; i++, tail++) {
      v.local_adj[tail] = v.local_adj[i];
      while (v.local_adj[tail] == v.local_adj[i+1]) i++;
    }
    v.nadj = tail;
    // VLOG(0) << "<" << vi << ">" << util::array_str("", v.local_adj, v.nadj);
    g->nadj_local += v.nadj;
  });
  VLOG(3) << "after sort";
  
  // compact
  on_all_cores([g]{
#ifdef SMALL_GRAPH
    if (locale_mycore() == 0) locale_free(g->scratch);
#endif
    
    VLOG(2) << "nadj_local = " << g->nadj_local;
    
    // allocate storage for local vertices' adjacencies
    g->adj_buf = locale_alloc<int64_t>(g->nadj_local);
    // compute total nadj
    g->nadj = allreduce<int64_t,collective_add>(g->nadj_local);
    
    int64_t * adj = g->adj_buf;
    for (Vertex& v : iterate_local(g->vs, g->nv)) {
      Grappa::memcpy(adj, v.local_adj, v.nadj);
      if (v.local_sz > 0) delete[] v.local_adj;
      v.local_sz = v.nadj;
      v.local_adj = adj;
      adj += v.nadj;
    }
    CHECK_EQ(adj - g->adj_buf, g->nadj_local);
  });
  
  return g;
}

