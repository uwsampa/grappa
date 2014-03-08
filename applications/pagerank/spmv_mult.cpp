#include "spmv_mult.hpp"

using namespace Grappa;

GlobalCompletionEvent mmjoiner;

GlobalAddress<Graph<PagerankVertex>> g;

void spmv_mult( GlobalAddress<Graph<PagerankVertex>> _g, vindex vx, vindex vy ) {
  call_on_all_cores([_g]{ g = _g; });
  CHECK( vx < (1<<3) && vy < (1<<3) );
  // forall rows
  forall<&mmjoiner>(g, [vx,vy](int64_t i, PagerankVertex& v){
    auto weights = v->weights;
    auto origin = mycore();
    mmjoiner.enroll(v.nadj);
    struct { int64_t i:44; vindex x:2, y:2; Core origin:16; } p
         = {         i,          vx,  vy,        origin };
    
    forall<async,nullptr>(adj(g,v), [weights,p](int64_t localj, GlobalAddress<PagerankVertex> vj){
      auto vjw = weights[localj];
      delegate::call<async,nullptr>(vj, [vjw,p](PagerankVertex& vj){
        auto yaccum = vjw * vj->v[p.x];
        delegate::call<async,nullptr>(g->vs+p.i,[yaccum,p](PagerankVertex& vi){
          vi->v[p.y] += yaccum;
          mmjoiner.send_completion(p.origin);
        });
      });
    });
  });
}



