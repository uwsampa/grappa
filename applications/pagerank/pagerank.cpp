#include "spmv_mult.hpp"

#include <Grappa.hpp>
#include <tasks/DictOut.hpp>
#include <Reducer.hpp>
#include <graph/Graph.hpp>

#include <iostream>

using namespace Grappa;

DEFINE_bool(metrics, false, "Dump metrics to stdout.");

// input size
DEFINE_uint64( nnz_factor, 16, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( scale, 16, "logN dimension of square matrix" );

// input file
DEFINE_string(path, "", "Path to graph source file");
DEFINE_string(format, "bintsv4", "Format of graph source file");

// pagerank options
DEFINE_double( damping, 0.8f, "Pagerank damping factor" );
DEFINE_double( epsilon, 0.001f, "Acceptable error magnitude" );

// runtime statistics
GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, iterations_time, 0); // provides total time, avg iteration time, number of iterations
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, init_pagerank_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, multiply_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, vector_add_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, norm_and_diff_time, 0);

// output statistics (ensure that only core 0 sets this exactly once AFTER `reset` and before `merge_and_print`)
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, pagerank_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, make_graph_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<double>, tuples_to_csr_time, 0);
GRAPPA_DEFINE_METRIC(SimpleMetric<uint64_t>, actual_nnz, 0);


/// calculate the damped matrix dM
void calculate_dM( GlobalAddress<Graph<PagerankVertex>> g, double d ) {
  // TODO
  // cleanup M to make it stochastic
  //for (j in cols)
  //  for (i in rows)
  //    sum+=
  //  if sum == 0 { set all m[,j] to 1/N }
  //  else set m[,j] to m[,j]/sum
  
  forall( g->vs, g->nv, [g,d](PagerankVertex& v) {
    for (auto& w : util::iterate(v->weights, v.nadj)) w *= d;
  });
}


// TODO: v1 and v2 may not be identically distributed,
// so forall_localize over two vectors may not work
// Perhaps we need a way to allocate/distribute an object to be distributed identically
// More adhoc solution is allocate these vectors as array of pairs
// void vsub(v1,v2)



AllReducer<double,collective_add> diff_sum_sq(0.0f);
double two_norm_diff_result;
double two_norm_diff(GlobalAddress<Graph<PagerankVertex>> g, vindex j2, vindex j1) {
  on_all_cores([]{
    diff_sum_sq.reset();
  });
  
  /* This line is the only one that really required the element_pair hack */
  forall(g, [j2,j1](PagerankVertex& v) {
    double diff = v->v[j2] - v->v[j1];
      /* NOTFORPAIR *///VLOG(5) << "diff[" << i << "] = " << *cv2 << " - " << *cv1 << " = " << diff;
      diff_sum_sq.accumulate(diff*diff);
  });
  
  on_all_cores([]{
    two_norm_diff_result = std::sqrt( diff_sum_sq.finish() );
  });
  
  double temp = two_norm_diff_result;
  two_norm_diff_result = 0.0f;
  return temp;
}


#include <cmath>
AllReducer<double,collective_add> sum_sq(0.0f);
double sqrt_total_sum_sq; // instead of a file-global could also pass to on_all_cores but its extra bandwidth

void normalize(GlobalAddress<Graph<PagerankVertex>> g, vindex j) {
  on_all_cores( [] { sum_sq.reset(); } );
  forall(g, [j](PagerankVertex& v){
    double ej = v->v[j];
    sum_sq.accumulate(ej * ej);
  });
  on_all_cores([]{
    double total_sum_sq = sum_sq.finish();
    sqrt_total_sum_sq = std::sqrt( total_sum_sq ); 
    CHECK( total_sum_sq != 0 ) << "Divide by zero will occur";
  });
  VLOG(4) << "normalize sum total = " << sqrt_total_sum_sq;
  
  // normalize
  forall(g, [j](PagerankVertex& v){
    v->v[j] /= sqrt_total_sum_sq;
  });
}

/////////////////////////////
// adding (1-d)/N vec(1) ////
double damp_vector_val;

/////////////////////////

struct pagerank_result {
  vindex which;  // which of the two vectors is V
  uint64_t num_iters;
};

// Iterative method
// R(t+1) = dMR(t) + (1-d)/N vec(1)
pagerank_result pagerank( GlobalAddress<Graph<PagerankVertex>> g, double d, double epsilon ) {
  LOG(INFO) << "version: 'iterative_new'";
  
  // bookeeping for which vector is which
  vindex V = 0;
  vindex LAST_V = 1;

  double t;

  // setup
  double init_start = walltime();
    
  LOG(INFO) << "Calculate dM";
  calculate_dM( g, d );
  
  // if ( m.nv <= 16 ) matrix_out( &m, LOG(INFO), true );

  LOG(INFO) << "Allocate rank vectors";
  // current pagerank vector: initialize to random values on [0,1]
  // (now encoded in Grap<PagerankVertex>)
  call_on_all_cores([]{ srand(0); });
  forall(g, [V](PagerankVertex& v){
    v->v[V] = ((double)rand()/RAND_MAX); //[0,1]
  });
  
  //if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
  normalize( g, V );
  
  // last pagerank vector: initialize to -inf
  forall(g, [LAST_V](PagerankVertex& v) {
    v->v[LAST_V] = -1000.0f;
  });

  LOG(INFO) << "Begin pagerank";
  //if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );

  // set the damping vector 
  auto dv = (1-d)/g->nv;
  on_all_cores([dv]{  damp_vector_val = dv;  });
    
  double init_end = walltime();
  init_pagerank_time += (init_end-init_start);
  
  double delta = 1.0f; // initialize to +inf delta
  uint64_t iter = 0;
  while( delta > epsilon ) {
    double istart, iend;
    istart = walltime();

    LOG(INFO) << "starting iter " << iter << ", delta = " << delta;
    
    // update last_v
    int temp = LAST_V;
    LAST_V = V;
    V = temp;
    
    // initialize target to zero
    forall(g, [V](PagerankVertex& v) {
      v->v[V] = 0.0f;
    });
    
    VLOG(2) << "after initialize";
    
    t = walltime();
    
      // multiply: v = dM*last_v
      spmv_mult(g, LAST_V, V);

    multiply_time += (walltime() - t);
    VLOG(2) << "after spmv_mult";
    
    t = walltime();
    
      // v += (1-d)/N * vec(1)
      forall(g, [V](PagerankVertex& v) {
        v->v[V] += damp_vector_val;
      });
    
    vector_add_time += (walltime()-t);
    
    //if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
   
    t = walltime();
      // normalize: v = v/2norm(v)
      normalize( g, V ); 
      
      iter++;
      delta = two_norm_diff( g, V, LAST_V );
    
    norm_and_diff_time += (walltime()-t);
    
    iend = walltime();
    iterations_time += (iend-istart);
    LOG(INFO) << "-->done (time " << iend-istart <<")";
  }
  
  LOG(INFO) << "ended with delta = " << delta;
    
  // return pagerank
  pagerank_result res;
  res.which = V;
  res.num_iters = iter;
  return res;
}

int main(int argc, char* argv[]) {
  Grappa::init(&argc, &argv);
  Grappa::run([]{
    LOG(INFO) << "starting...";
 
    // to be assigned to stats output
    double make_graph_time_SO, 
           tuples_to_csr_time_SO, 
           pagerank_time_SO;
    uint64_t actual_nnz_SO;

    uint64_t N = (1L<<FLAGS_scale);

    uint64_t desired_nnz = FLAGS_nnz_factor * N;

    // results output
    DictOut resultd;
 
    // initialize rng 
    //init_random(); 
    //userseed = 10;

    double t;
    long userseed = 0xDECAFBAD; // from (prng.c: default seed)


    TupleGraph tg;
    
    if( FLAGS_path.empty() ) {
      tg = TupleGraph::Kronecker(FLAGS_scale, desired_nnz, userseed, userseed);
    } else {
      // load from file
      tg = TupleGraph::Load( FLAGS_path, FLAGS_format );
    }

    t = walltime() - t;
    LOG(INFO) << "make_graph: " << t;
    make_graph_time_SO = t;
  
    t = walltime();
    
    auto g = Graph<PagerankVertex>::create(tg);
    
    tuples_to_csr_time_SO = walltime() - t;

    LOG(INFO) << "tuple->csr: " << tuples_to_csr_time_SO;
    actual_nnz_SO = g->nadj;
    //print_graph( &unweighted_g ); 
    LOG(INFO) << "final matrix has " << static_cast<double>(actual_nnz_SO)/N << " avg nonzeroes/row";

    // add weights to the csr graph
    forall(g->vs, g->nv, [](PagerankVertex& v){
      v->weights = locale_alloc<double>(v.nadj);
      v->v[0] = v->v[1] = 0;
      
      // TODO random
      for (long i=0; i<v.nadj; i++) v->weights[i] = 0.2f;
    });

    // print the matrix if it is small 
    // if ( g.nv <= 16 ) { 
      //matrix_out( &g, std::cerr, false); 
      //matrix_out( &g, std::cout, true );
    // }

    Metrics::reset();
    Metrics::start_tracing();
  
    pagerank_result result;
    t = walltime();
    
      result = pagerank( g, FLAGS_damping, FLAGS_epsilon );
    
    pagerank_time_SO = (walltime()-t);
    
    Metrics::stop_tracing();
    
    // output stats
    make_graph_time   = make_graph_time_SO;
    tuples_to_csr_time = tuples_to_csr_time_SO;
    actual_nnz        = actual_nnz_SO;
    pagerank_time     = pagerank_time_SO;

    Metrics::merge_and_dump_to_file();
    
    if (FLAGS_metrics) Metrics::merge_and_print();
    
    // vector rank = result.ranks;
    // vindex which = result.which;
  
    g->destroy();
    
    // TODO: print out pagerank stats, like mean, median, min, max
    // could also print out random sample of them for verify
    //if ( rank.length <= 16 ) vector_out( &rank, std::cout );
  });
  Grappa::finalize();
}
