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
      struct { int64_t i:44; vindex x:2, y:2; Core origin:16; } p
           = {         i,          vx,  vy,        mycore() };
      
      mmjoiner.enroll(1);
      delegate::call<async,nullptr>(g->vs+j, [vjw,p](PagerankVertex& vj){
        auto yaccum = vjw * vj->v[p.x];
        delegate::call<async,nullptr>(g->vs+p.i,[yaccum,p](PagerankVertex& vi){
          vi->v[p.y] += yaccum;
          mmjoiner.send_completion(p.origin);
        });
      });
    }
  });
}



