#include "spmv_mult.hpp"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/grappa/timer.h"
#include "../graph500/prng.h"

#include <Grappa.hpp>
#include <GlobalAllocator.hpp>
#include <Array.hpp>
#include <ParallelLoop.hpp>
#include <tasks/DictOut.hpp>
#include <Reducer.hpp>

#include <iostream>

using namespace Grappa;


// input size
DEFINE_uint64( nnz_factor, 10, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( logN, 16, "logN dimension of square matrix" );

// pagerank options
DEFINE_double( damping, 0.8f, "Pagerank damping factor" );
DEFINE_double( epsilon, 0.001f, "Acceptable error magnitude" );



weighted_csr_graph mhat;

/// calculate the damped matrix dM
void calculate_dM( weighted_csr_graph m, double d ) {

  // TODO
  // cleanup M to make it stochastic
  //for (j in cols)
  //  for (i in rows)
  //    sum+=
  //  if sum == 0 { set all m[,j] to 1/N }
  //  else set m[,j] to m[,j]/sum

  forall_localized( m.adjweight, m.nadj, [d]( int64_t i, double& weight ) {
    weight = weight * d;
  });
}


// TODO: v1 and v2 may not be identically distributed,
// so forall_localize over two vectors may not work
// Perhaps we need a way to allocate/distribute an object to be distributed identically
// More adhoc solution is allocate these vectors as array of pairs
// void vsub(v1,v2)



AllReducer<double,collective_add> diff_sum_sq(0.0f);
double two_norm_diff_result;

double two_norm_diff(vector v2, vector v1) {
  on_all_cores( [] {
    diff_sum_sq.reset();
  });

  forall_global_public( 0, v1.length, [v2,v1]( int64_t start, int64_t iters ) {
    for ( int64_t i=start; i<start+iters; i++ ) {
      Incoherent<double>::RO cv1(v1.a+i, 1);
      Incoherent<double>::RO cv2(v2.a+i, 1);
      cv1.start_acquire();
      cv2.start_acquire();

      double diff = *cv2 - *cv1;
      VLOG(5) << "diff[" << i << "] = " << *cv2 << " - " << *cv1 << " = " << diff;
      diff_sum_sq.accumulate(diff*diff);
    }
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

void normalize( vector v ) {
  // calculate sum of squares
  on_all_cores( [] { sum_sq.reset(); } );
  forall_localized( v.a, v.length, []( int64_t i, double& ele ) {
    VLOG(5) << "normalize sum += " << ele;
    sum_sq.accumulate(ele * ele);
  });
  on_all_cores( [] { 
    double total_sum_sq = sum_sq.finish();
    sqrt_total_sum_sq = std::sqrt( total_sum_sq ); 
    CHECK( total_sum_sq != 0 ) << "Divide by zero will occur";
  });
  VLOG(4) << "normalize sum total = " << sqrt_total_sum_sq;

  // normalize
  forall_localized( v.a, v.length, []( int64_t i, double& ele) {
    ele /= sqrt_total_sum_sq;
  });
}

/////////////////////////////
// adding (1-d)/N vec(1) ////
double damp_vector_val;

void add_constant_vector( vector v ) {
  forall_localized( v.a, v.length, []( int64_t i, double& e ) {
    e += damp_vector_val;
  });
}
/////////////////////////

struct pagerank_result {
  vector ranks;
  uint64_t num_iters;
};

// Iterative method
// R(t+1) = dMR(t) + (1-d)/N vec(1)
pagerank_result pagerank( weighted_csr_graph m, double d, double epsilon ) {
  LOG(INFO) << "Calculate dM";
  calculate_dM( m, d );
  
  if ( m.nv <= 16 ) matrix_out( &m, LOG(INFO), true );

  LOG(INFO) << "Allocate rank vectors";
  // current pagerank vector: initialize to random values on [0,1]
  vector v;
  v.length = m.nv;
  v.a = Grappa_typed_malloc<double>(v.length);
  on_all_cores( [] { srand(0); } );
  forall_localized( v.a, v.length, []( int64_t i, double& ele ) {
    ele = ((double)rand()/RAND_MAX); //[0,1]
  });
  
  if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
  normalize( v );

  // last pagerank vector: initialize to -inf
  vector last_v;
  last_v.length = m.nv;
  last_v.a = Grappa_typed_malloc<double>(last_v.length);
  Grappa_memset_local(last_v.a, -1000.0f, last_v.length);

  LOG(INFO) << "Begin pagerank";
  if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
 
  // set the damping vector 
  auto dv = (1-d)/m.nv;
  on_all_cores( [dv] {
    damp_vector_val = dv;
  });
    
  double err;
  uint64_t iter = 0;
  while( (err = two_norm_diff( v, last_v )) > epsilon ) {
    LOG(INFO) << "starting iter " << iter << ", err = " << err;

    // update last_v
    vector temp = last_v;
    last_v = v;
    v = temp;
    
    // initialize target to zero
    Grappa_memset_local(v.a, 0.0f, v.length);

    // multiply: v = dM*last_v
    spmv_mult( m, last_v, v );

    // v += (1-d)/N * vec(1)
    add_constant_vector( v );

    if ( v.length <= 16 ) vector_out( &v, LOG(INFO) );
   
    // normalize: v = v/2norm(v)
    normalize( v ); 

    iter++;
  }

  LOG(INFO) << "ended with err = " << err;
  
  // free the extra vector
  Grappa_free( last_v.a );

  // return pagerank
  pagerank_result res;
  res.ranks = v;
  res.num_iters = iter;
  return res;
}

void user_main( int * ignore ) {
  LOG(INFO) << "starting...";
 
  tuple_graph tg;
  csr_graph unweighted_g;
  uint64_t N = (1L<<FLAGS_logN);

  uint64_t desired_nnz = FLAGS_nnz_factor * N;

  // results output
  DictOut resultd;

  double time;
  /*TODO SEED*/userseed = 10;
  TIME(time, 
    make_graph( FLAGS_logN, desired_nnz, userseed, userseed, &tg.nedge, &tg.edges );
  );
  LOG(INFO) << "make_graph: " << time;
  resultd.add( "make_graph_time", time );

  TIME(time,
    create_graph_from_edgelist(&tg, &unweighted_g);
  );
  LOG(INFO) << "tuple->csr: " << time;
  resultd.add( "tuple_to_csr_time", time );
  int64_t actual_nnz = unweighted_g.nadj;
  resultd.add( "actual_nnz", actual_nnz );
  LOG(INFO) << "final matrix has " << static_cast<double>(actual_nnz)/N << " avg nonzeroes/row";

  // add weights to the csr graph
  weighted_csr_graph g( unweighted_g );
  g.adjweight = Grappa_typed_malloc<double>(g.nadj);
  forall_localized( g.adjweight, g.nadj, [](int64_t i, double& w) {
    // TODO random
    w = 0.2f;
  });

  // print the matrix if it is small 
  if ( g.nv <= 16 ) matrix_out( &g, std::cout, true );

  Grappa_reset_stats_all_nodes();
  pagerank_result result;
  TIME(time,
    result = pagerank( g, FLAGS_damping, FLAGS_epsilon );
  );
  Grappa_merge_and_dump_stats();
  resultd.add( "pagerank_time", time );
  resultd.add( "pagerank_num_iters", result.num_iters );
  vector rank = result.ranks;

  std::cout<<"rank=";
  // TODO: print out pagerank stats, like mean, median, min, max
  // could also print out random sample of them for verify
  if ( rank.length <= 16 ) vector_out( &rank, std::cout );
  std::cout<<std::endl;

 std::cout << "MULT" << resultd.toString() << std::endl;    
}

/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}
