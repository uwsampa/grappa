#include "spmv_mult.hpp"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/grappa/timer.h"
#include "../graph500/prng.h"
#include "../graph500/grappa/graph.hpp"

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>
#include <ParallelLoop.hpp>
#include <tasks/DictOut.hpp>
#include <Reducer.hpp>
#include <Statistics.hpp>

#include <iostream>

using namespace Grappa;


// input size
DEFINE_uint64( nnz_factor, 16, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( scale, 16, "logN dimension of square matrix" );

// pagerank options
DEFINE_double( damping, 0.8f, "Pagerank damping factor" );
DEFINE_double( epsilon, 0.001f, "Acceptable error magnitude" );

// runtime statistics
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, iterations_time, 0); // provides total time, avg iteration time, number of iterations
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, init_pagerank_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, multiply_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, vector_add_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, norm_and_diff_time, 0);

// output statistics (ensure that only core 0 sets this exactly once AFTER `reset` and before `merge_and_print`)
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, pagerank_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, make_graph_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<double>, tuples_to_csr_time, 0);
GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, actual_nnz, 0);


weighted_csr_graph mhat;

/// calculate the damped matrix dM
void calculate_dM( GlobalAddress<Graph<WeightedAdjVertex>> g, double d ) {
  // TODO
  // cleanup M to make it stochastic
  //for (j in cols)
  //  for (i in rows)
  //    sum+=
  //  if sum == 0 { set all m[,j] to 1/N }
  //  else set m[,j] to m[,j]/sum
  
  forall_localized( g->vs, g->nv, [g,d](WeightedAdjVertex& v) {
    for (auto& w : util::iterate(v.weights, v.nadj)) w *= d;
  });
}


// TODO: v1 and v2 may not be identically distributed,
// so forall_localize over two vectors may not work
// Perhaps we need a way to allocate/distribute an object to be distributed identically
// More adhoc solution is allocate these vectors as array of pairs
// void vsub(v1,v2)



AllReducer<double,collective_add> diff_sum_sq(0.0f);
double two_norm_diff_result;
double two_norm_diff(vector vs, vindex j2, vindex j1) {
  on_all_cores( [] {
    diff_sum_sq.reset();
  });

  /* This line is the only one that really required the element_pair hack */
  forall_localized( vs.a, vs.length, [j2,j1]( int64_t i, element_pair& ele ) {
    double diff = ele.vp[j2] - ele.vp[j1];
      /* NOTFORPAIR *///VLOG(5) << "diff[" << i << "] = " << *cv2 << " - " << *cv1 << " = " << diff;
      diff_sum_sq.accumulate(diff*diff);
  });

  on_all_cores( [] {
    two_norm_diff_result = std::sqrt( diff_sum_sq.finish() );
  });

  double temp = two_norm_diff_result;
  two_norm_diff_result = 0.0f;
  return temp;
}


#include <cmath>
AllReducer<double,collective_add> sum_sq(0.0f);
double sqrt_total_sum_sq; // instead of a file-global could also pass to on_all_cores but its extra bandwidth

void normalize( vector v, vindex j ) {
  // calculate sum of squares
  on_all_cores( [] { sum_sq.reset(); } );
  forall_localized( v.a, v.length, [j]( int64_t i, element_pair& ele ) {
    //VLOG(5) << "normalize sum += " << ele;
    double ej = ele.vp[j];
    sum_sq.accumulate(ej * ej);
  });
  on_all_cores( [] { 
    double total_sum_sq = sum_sq.finish();
    sqrt_total_sum_sq = std::sqrt( total_sum_sq ); 
    CHECK( total_sum_sq != 0 ) << "Divide by zero will occur";
  });
  VLOG(4) << "normalize sum total = " << sqrt_total_sum_sq;

  // normalize
  forall_localized( v.a, v.length, [j]( int64_t i, element_pair& ele) {
    ele.vp[j] /= sqrt_total_sum_sq;
  });
}

/////////////////////////////
// adding (1-d)/N vec(1) ////
double damp_vector_val;

void add_constant_vector( vector v, vindex j ) {
  forall_localized( v.a, v.length, [j]( int64_t i, element_pair& e ) {
    e.vp[j] += damp_vector_val;
  });
}
/////////////////////////

struct pagerank_result {
  vector ranks;
  vindex which;  // which of the two vectors is V
  uint64_t num_iters;
};

// Iterative method
// R(t+1) = dMR(t) + (1-d)/N vec(1)
pagerank_result pagerank( GlobalAddress<Graph<WeightedAdjVertex>> g, double d, double epsilon ) {
  LOG(INFO) << "version: 'iterative_new'";
  
  // bookeeping for which vector is which
  vindex V = 0;
  vindex LAST_V = 1;

  double time;

  // setup
  double init_start = walltime();
    
    LOG(INFO) << "Calculate dM";
    calculate_dM( g, d );
    
    // if ( m.nv <= 16 ) matrix_out( &m, LOG(INFO), true );

    LOG(INFO) << "Allocate rank vectors";
    // current pagerank vector: initialize to random values on [0,1]
    vector v;
    v.length = g->nv;
    v.a = global_alloc<element_pair>(v.length);
    on_all_cores( [] { srand(0); } );
    forall_localized( v.a, v.length, [V](element_pair& ele ) {
      ele.vp[V] = ((double)rand()/RAND_MAX); //[0,1]
    });

    //if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
    normalize( v, V );

    // last pagerank vector: initialize to -inf
    forall_localized( v.a, v.length, [LAST_V](element_pair& ele ) {
      ele.vp[LAST_V] = -1000.0f;
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
    forall_localized( v.a, v.length, [V](element_pair& ele) {
      ele.vp[V] = 0.0f;
    });
    
    VLOG(0) << "after initialize";

    TIME(time,
      // multiply: v = dM*last_v
      spmv_mult(g, v, LAST_V, V);
    );
    multiply_time += time;
    VLOG(0) << "after spmv_mult";

    TIME(time,
      // v += (1-d)/N * vec(1)
      add_constant_vector( v, V );
    );
    vector_add_time += time;

    //if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
   
    TIME(time,
      // normalize: v = v/2norm(v)
      normalize( v, V ); 

      iter++;
      delta = two_norm_diff( v, V, LAST_V );
    );
    norm_and_diff_time += time;

    iend = Grappa_walltime();
    iterations_time += (iend-istart);
    LOG(INFO) << "-->done (time " << iend-istart <<")";
  }

  LOG(INFO) << "ended with delta = " << delta;
  
  // free the extra vector
  Grappa_free( v.a );

  // return pagerank
  pagerank_result res;
  res.ranks = v;
  res.which = V;
  res.num_iters = iter;
  return res;
}

//void print_graph(csr_graph* g);
//void print_array(const char * name, GlobalAddress<packed_edge> base, size_t nelem, int width);
void user_main( int * ignore ) {
  LOG(INFO) << "starting...";
 
  // to be assigned to stats output
  double make_graph_time_SO, 
         tuples_to_csr_time_SO, 
         pagerank_time_SO;
  uint64_t actual_nnz_SO;

  tuple_graph tg;
  uint64_t N = (1L<<FLAGS_scale);

  uint64_t desired_nnz = FLAGS_nnz_factor * N;

  // results output
  DictOut resultd;
 
  // initialize rng 
  //init_random(); 
  //userseed = 10;

  double time;
  TIME(time, 
    make_graph( FLAGS_scale, desired_nnz, userseed, userseed, &tg.nedge, &tg.edges );
    //print_array("tuples", tg.edges, tg.nedge, 10);
  );
  LOG(INFO) << "make_graph: " << time;
  make_graph_time_SO = time;
  
  time = walltime();
  
  auto g = Graph<WeightedAdjVertex>::create(tg);
    
  tuples_to_csr_time_SO = walltime() - time;
  LOG(INFO) << "tuple->csr: " << tuples_to_csr_time_SO;
  actual_nnz_SO = g->nadj;
  //print_graph( &unweighted_g ); 
  LOG(INFO) << "final matrix has " << static_cast<double>(actual_nnz_SO)/N << " avg nonzeroes/row";

  // add weights to the csr graph
  forall_localized(g->vs, g->nv, [](WeightedAdjVertex& v){
    v.weights = locale_alloc<double>(v.nadj);
    // TODO random
    for (long i=0; i<v.nadj; i++) v.weights[i] = 0.2f;
  });

  // print the matrix if it is small 
  // if ( g.nv <= 16 ) { 
    //matrix_out( &g, std::cerr, false); 
    //matrix_out( &g, std::cout, true );
  // }

  Grappa::Statistics::reset();
  Grappa::Statistics::start_tracing();
  
  pagerank_result result;
  TIME(time,
    result = pagerank( g, FLAGS_damping, FLAGS_epsilon );
  );
  pagerank_time_SO = time;
  
  Grappa::Statistics::stop_tracing();

  // output stats
  make_graph_time   = make_graph_time_SO;
  tuples_to_csr_time = tuples_to_csr_time_SO;
  actual_nnz        = actual_nnz_SO;
  pagerank_time     = pagerank_time_SO;
  Grappa::Statistics::merge_and_print();

  vector rank = result.ranks;
  vindex which = result.which;
  
  g->destroy();
  
  // TODO: print out pagerank stats, like mean, median, min, max
  // could also print out random sample of them for verify
  //if ( rank.length <= 16 ) vector_out( &rank, std::cout );
}

/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}
