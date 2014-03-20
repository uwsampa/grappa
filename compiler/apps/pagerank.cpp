#include <Grappa.hpp>
#include <graph/Graph.hpp>
#include <Reducer.hpp>

#ifdef __GRAPPA_CLANG__
#include <Primitive.hpp>
#endif

using namespace Grappa;
using delegate::call;
using namespace std;

using vindex = int;

struct PagerankData {
  double * weights;
  double v[2];
};
using PagerankVertex = Vertex<PagerankData>;
using PagerankGraph = Graph<PagerankVertex>;

SymmetricAddress<PagerankGraph> ga;
GlobalCompletionEvent phaser;

/////////////////////////////
// adding (1-d)/N vec(1) ////
double damp_vector_val;


#include <cmath>
AllReducer<double,collective_add> sum_sq(0.0f);
double sqrt_total_sum_sq; // instead of a file-global could also pass to on_all_cores but its extra bandwidth

void normalize(vindex j) {
  on_all_cores( [] { sum_sq.reset(); } );
  forall(ga, [j](PagerankVertex& v){
    double ej = v->v[j];
    sum_sq.accumulate(ej * ej);
  });
  on_all_cores([]{
    double total_sum_sq = sum_sq.finish();
    sqrt_total_sum_sq = sqrt( total_sum_sq );
    CHECK( total_sum_sq != 0 ) << "Divide by zero will occur";
  });
  VLOG(4) << "normalize sum total = " << sqrt_total_sum_sq;
  
  // normalize
  forall(ga, [j](PagerankVertex& v){
    v->v[j] /= sqrt_total_sum_sq;
  });
}

AllReducer<double,collective_add> diff_sum_sq(0.0f);
double two_norm_diff_result;
double two_norm_diff(vindex j2, vindex j1) {
  on_all_cores([]{
    diff_sum_sq.reset();
  });
  
  /* This line is the only one that really required the element_pair hack */
  forall(ga, [j2,j1](PagerankVertex& v) {
    double diff = v->v[j2] - v->v[j1];
    /* NOTFORPAIR *///VLOG(5) << "diff[" << i << "] = " << *cv2 << " - " << *cv1 << " = " << diff;
    diff_sum_sq.accumulate(diff*diff);
  });
  
  on_all_cores([]{
    two_norm_diff_result = sqrt( diff_sum_sq.finish() );
  });
  
  double temp = two_norm_diff_result;
  two_norm_diff_result = 0.0f;
  return temp;
}

void spmv_mult(vindex vx, vindex vy) {
  CHECK( vx < (1<<3) && vy < (1<<3) );
  // forall rows
  forall<&phaser>(ga, [vx,vy](int64_t i, PagerankVertex& v){
    auto origin = mycore();
    phaser.enroll(v.nadj);
    
#ifdef __GRAPPA_CLANG__
    forall<async,nullptr>(adj(ga,v), [=](int64_t localj, VertexID j){
      auto& vi = as_ptr(ga->vertices())[i];
      auto& vj = as_ptr(ga->vertices())[j];
      vi->v[vy] += vi->weights[localj] * vj->v[vx];
      phaser.send_completion(origin);
    });
#else
    auto weights = v->weights;
    struct { int64_t i:44; vindex x:2, y:2; Core origin:16; } p
         = {         i,          vx,  vy,        origin };
    
    forall<async,nullptr>(adj(ga,v), [weights,p](int64_t localj, GlobalAddress<PagerankVertex> vj){
      auto vjw = weights[localj];
      call<async,nullptr>(vj, [vjw,p](PagerankVertex& vj){
        auto yaccum = vjw * vj->v[p.x];
        call<async,nullptr>(ga->vs+p.i,[yaccum,p](PagerankVertex& vi){
          vi->v[p.y] += yaccum;
          phaser.send_completion(p.origin);
        });
      });
    });
#endif
    
  });

}

DEFINE_bool(metrics, false, "Dump metrics to stdout.");

// input size
DEFINE_uint64( nnz_factor, 16, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( scale, 16, "logN dimension of square matrix" );

// pagerank options
DEFINE_double( damping, 0.8f, "Pagerank damping factor" );
DEFINE_double( epsilon, 0.001f, "Acceptable error magnitude" );

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, iteration_time, 0);
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, spmv_time, 0);

// output statistics
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, pagerank_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, make_graph_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuples_to_csr_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, actual_nnz, 0);


int main(int argc, char* argv[]) {
  init(&argc, &argv);
  run([]{
    double t;
    uint64_t N = (1L<<FLAGS_scale);
    uint64_t desired_nnz = FLAGS_nnz_factor * N;

    GRAPPA_TIME_REGION(make_graph_time) {
      long userseed = 0xDECAFBAD; // from (prng.c: default seed)
      auto tg = TupleGraph::Kronecker(FLAGS_scale, desired_nnz, userseed, userseed);
      auto _g = PagerankGraph::create( tg );
      call_on_all_cores([=]{ ga = _g; });
    }
    cerr << make_graph_time << "\n";
    
    ////////////////////
    // initialize graph
    forall(ga, [](PagerankVertex& v){
      // set both vectors = 0
      v->v[0] = v->v[1] = 0;
      
      // add weights
      v->weights = locale_alloc<double>(v.nadj);
      // TODO: random
      for (long i=0; i<v.nadj; i++) v->weights[i] = 0.2f;
    });
    
    //////////////////
    // do pagerank
    t = walltime();
    
    double d = FLAGS_damping;
    double epsilon = FLAGS_epsilon;
    
    vindex V = 0;
    vindex LAST_V = 1;
    
    // calculate dM
    forall(ga, [d](PagerankVertex& v) {
      for (int i=0; i<v.nadj; i++) v->weights[i] *= d;
    });
    
    // init vectors
    call_on_all_cores([]{ srand(0); });
    forall(ga, [=](PagerankVertex& v){
      v->v[V] = ((double)rand()/RAND_MAX); //[0,1]
    });

    normalize(V);

    forall(ga, [=](PagerankVertex& v){
      v->v[LAST_V] = -1000.0f;
    });
    
    // set the damping vector
    auto dv = (1-d)/ga->nv;
    call_on_all_cores([dv]{  damp_vector_val = dv;  });
    
    double delta = 1.0f; // initialize to +inf delta
    uint64_t iter = 0;
    while( delta > epsilon ) GRAPPA_TIME_REGION(iteration_time) {
      VLOG(0) << "iteration " << iter;
      
      // update last_v
      swap(LAST_V, V);
      
      // initialize target to zero
      forall(ga, [V](PagerankVertex& v) {
        v->v[V] = 0.0f;
      });
      
      GRAPPA_TIME_REGION(spmv_time) {
        // multiply: v = dM*last_v
        spmv_mult(LAST_V, V);
      }
      
      // v += (1-d)/N * vec(1)
      forall(ga, [V](PagerankVertex& v) {
        v->v[V] += damp_vector_val;
      });
      
      // normalize: v = v/2norm(v)
      normalize(V);
      
      iter++;
      delta = two_norm_diff(V, LAST_V);
      VLOG(0) << "delta = " << delta;
    }
    
    pagerank_time = walltime()-t;

    if (FLAGS_metrics) Metrics::merge_and_print();
    else {
      cerr << pagerank_time << "\n";
      iteration_time.pretty(cerr) << "\n";
      spmv_time.pretty(cerr) << "\n";
    }
    Metrics::merge_and_dump_to_file();
    
  });
  finalize();
}
