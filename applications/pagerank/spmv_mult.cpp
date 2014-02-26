#include "spmv_mult.hpp"

using namespace Grappa;

GlobalCompletionEvent mmjoiner;

GlobalAddress<Graph<PagerankVertex>> g;

void spmv_mult( GlobalAddress<Graph<PagerankVertex>> _g, vindex vx, vindex vy ) {
  call_on_all_cores([_g]{ g = _g; });
  CHECK( vx < (1<<3) && vy < (1<<3) );
  // forall rows
  forall<&mmjoiner>(g->vs, g->nv, [vx,vy](int64_t i, PagerankVertex& v){
    for (auto j : v.adj_iter()) {
      auto vjw = v->weights[j];
      struct { int64_t i:60; vindex x:2, y:2; } p = {i, vx, vy};
      delegate::call<async,&mmjoiner>(g->vs+j, [vjw,p](PagerankVertex& vj){
        auto yaccum = vjw * vj->v[p.x];
        delegate::call<async,&mmjoiner>(g->vs+p.i,[yaccum,p](PagerankVertex& vi){
          vi->v[p.y] += yaccum;
        });
      });
    }
  });
}



