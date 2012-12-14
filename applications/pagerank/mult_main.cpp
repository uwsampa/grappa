#include "spmv_mult.hpp"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../graph500/grappa/timer.h"
#include "../graph500/prng.h"

#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Array.hpp"
#include "tasks/DictOut.hpp"
#include <iostream>
#include <fstream>


DEFINE_uint64( nnz_factor, 10, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( logN, 16, "logN dimension of square matrix" );


//////////utility//////////////
LOOP_FUNCTION(start_prof,nid) {
  Grappa_start_profiling();
}

LOOP_FUNCTION(stop_prof,nid) {
  Grappa_stop_profiling();
}
void start_profiling() {
  start_prof f;
  fork_join_custom(&f);
}
void stop_profiling() {
  stop_prof f;
  fork_join_custom(&f);
}
///////////////////////////



void random_weights( double * w ) {
  // TODO random
  *w = 1.0f;
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
  forall_local<double,random_weights>(g.adjweight, g.nadj);

  VLOG(1) << "Allocating src vector";
  vector vec;
  vec.length = N;
  vec.a = Grappa_typed_malloc<double>(N);
  Grappa_memset(vec.a, 1.0f/N, N);

  VLOG(1) << "Allocating dest vector";
  vector target;
  target.length = N;
  target.a = Grappa_typed_malloc<double>(N);
  Grappa_memset(target.a, 0.0f, N);

  //matrix_out( &g, std::cout, false );
  //matrix_out( &g, std::cout, true );

  Grappa_reset_stats_all_nodes();
  VLOG(1) << "Starting multiply... N=" << g.nv;
  TIME(time,
  //  start_profiling();
      spmv_mult(g, vec, target);
  //  stop_profiling();
  );
  Grappa_merge_and_dump_stats();
  LOG(INFO) << "multiply: " << time;
  LOG(INFO) << actual_nnz / time << " nz/s";
  resultd.add( "multiply_time", time );

  std::cout << "MULT" << resultd.toString() << std::endl;    



//  std::cout<<"x=";
//  vector_out( &vec, std::cout );
//  std::cout<<std::endl;
//  std::cout<<"y=";
//  vector_out( &target, std::cout );
//  std::cout<<std::endl;

  // write out an R file to run verification
  std::ofstream verify_file;
  verify_file.open("spmvmult.R");
  //verify_file << "M <- "; R_matrix_out( &g, verify_file ); verify_file << std::endl;
  verify_file << "M <- as.matrix(read.table('verify_matrix',sep=','))" << std::endl << "M <- M[1:dim(M)[1],1:dim(M)[1]]" << std::endl;;
  verify_file << "x <- "; R_vector_out( &vec, verify_file ); verify_file << std::endl;
  verify_file << "y <- "; R_vector_out( &target, verify_file ); verify_file << std::endl;
  verify_file << "yv <- as.vector(t(M %*% x))" << std::endl;
  verify_file << "all(yv == y)" << std::endl;
  verify_file.close();

  std::ofstream verify_matrix;
  verify_matrix.open("verify_matrix");
  matrix_out( &g, verify_matrix, true );
  verify_matrix.close();

}


/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}
