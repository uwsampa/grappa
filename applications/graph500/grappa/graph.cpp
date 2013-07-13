#pragma once

#include "graph.hpp"

GlobalAddress<Graph> Graph::create(tuple_graph& tg) {
  auto g = mirrored_global_alloc<Graph>();
  
  // find nv
  forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
    if (e.v0 > g->nv) { g->nv = e.v0; }
    else if (e.v1 > g->nv) { g->nv = e.v1; }
  });
  on_all_cores([g]{
    g->nv = Grappa::allreduce<int64_t,collective_max>(g->nv) + 1;
  });
  
  auto vs = global_alloc<Vertex>(g->nv);
  auto self = g;
  call_on_all_cores([g,vs]{
    new (g.localize()) Graph(g, vs, g->nv);
    VLOG(0) << "nv = " << g->nv;
    g->scratch = locale_alloc<int64_t>(g->nv);
    VLOG(0) << "scratch = " << g->scratch;
    
    memset(g->scratch, 0, sizeof(int64_t)*g->nv);
  });
  
  forall_localized(tg.edges, tg.nedge, [g](packed_edge& e){
    CHECK_LT(e.v0, g->nv); CHECK_LT(e.v1, g->nv);
    g->scratch[e.v0]++;
    g->scratch[e.v1]++;
    // TODO: oops, need to do actual "scatter"? how do I make undirected?
  });
  on_all_cores([g]{ allreduce_inplace<int64_t,collective_add>(g->scratch, g->nv); });
  
  on_all_cores([g]{ VLOG(0) << util::array_str("scratch", g->scratch, g->nv, 25); });
  
  // allocate space for each vertex's adjacencies (+ duplicates)
  forall_localized(g->vs, g->nv, [g](int64_t i, Vertex& v) {
    v.nadj = 0;
    v.local_sz = g->scratch[i];
    if (v.local_sz > 0) v.local_adj = new int64_t[v.local_sz];
    // g->nadj_local += v.nadj;
  });
  VLOG(0) << "after adj allocs";
  VLOG(0) << "nv = " << g->nv;
  
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
  VLOG(0) << "after scatter";
  VLOG(0) << "nv = " << g->nv;
    
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
  VLOG(0) << "after sort";
  
  // compact
  on_all_cores([g]{
    // VLOG(0) << "scratch = " << g->scratch;
    locale_free(g->scratch);
    
    VLOG(0) << "nadj_local = " << g->nadj_local;
    
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
    VLOG(0) << "nv = " << g->nv;
  });
  
  return g;
}
