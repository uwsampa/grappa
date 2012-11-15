// graph500/grappa/
// XXX shouldn't have to include this first: common.h and oned_csr.h have cyclic dependency
#include "../graph500/grappa/common.h"
#include "../graph500/grappa/oned_csr.h"
#include "../graph500/grappa/timer.h"

// graph500/
#include "../graph500/generator/make_graph.h"
#include "../graph500/generator/utils.h"
#include "../prng.h"

// Grappa
#include "Grappa.hpp"
#include "Cache.hpp"
#include "GlobalTaskJoiner.hpp"
#include "ForkJoin.hpp"
#include "Delegate.hpp"
#include "GlobalAllocator.hpp"
#include "tasks/DictOut.hpp"

#include <iostream>

#define XOFF(xoff, index) (xoff)+2*(index)
#define XENDOFF(matrix, index) (xoff)+2*(index)+1

struct vector {
  GlobalAddress<double> a;
  uint64_t length;
};

// global: local per Node
csr_graph m;
vector v;
vector y;

void row_loop( int64_t start, int64_t iters ) {
  //VLOG(1) << "rows [" << start << ", " << start+iters << ")";
  
  double y_stor[iters];
  Incoherent<double>::WO cy( y.a+start, iters, y_stor );
  for ( int64_t i=start; i<start+iters; i++ ) {
    // y[i] = 0
    cy[i-start] = 0;

    // kstart = row_ptr[i], kend = row_ptr[i+1]
    int64_t kbounds[2];
    Incoherent<int64_t>::RO cxoff( XOFF(m.xoff, i), 2, kbounds );
    const int64_t kstart = cxoff[0];
    const int64_t kend = cxoff[1];

    // Column loop
    // TODO parallel for, but will have different guarentees on y released
    for ( int64_t k = kstart; k<kend; k++ ) {
      int64_t j, mv;
      // TODO: hoist cache
      Incoherent<int64_t>::RO cj( m.xadj + k, 1, &j );
      // TODO: currently assumes nonzeroes are 1's
      //Incoherent<int64_t>::RO vals_k( m.val + k, 1, &mv );
      //yi += *vals_k * x[j];
      double vj;
      Grappa_delegate_read(v.a + *cj, &vj);   
      //VLOG(1) << "v[*cj]=v[" << *cj << "]="<<vj;
      cy[i-start] += 1.0f * vj;
    } 
                               // TODO: if yi.start_release() could guarentee the storage is no longer used
                               //       then this would be nice for no block until all y[start,start+iters) need to be written, although that pressures the network side
                               // Y could actually be like a feed forward delegate that syncs at global join end
  }
}
// In general, destination arrays that are either associatively accumulated into or written once into can often get by with very weak consistency: sync at very end once value of vector needs to be known valid result.
    
// With current low level programming model, the choice to parallelize a loop involves different code


DEFINE_bool(row_distribute, true, "nodes begin with equal number of row tasks");

LOOP_FUNCTOR( matrix_mult_f, nid, ((csr_graph,matrix)) ((vector,src)) ((vector,targ)) ) {
  m = matrix;
  v = src;
  y = targ;

  global_joiner.reset();
  if ( FLAGS_row_distribute ) {
    range_t r = blockDist(0, m.nv, Grappa_mynode(), Grappa_nodes());
    async_parallel_for<row_loop, joinerSpawn<row_loop,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(r.start, r.end-r.start);
  } else {
    if ( nid == 0 ) {
    async_parallel_for<row_loop, joinerSpawn<row_loop,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(0, m.nv);
    }
  }
  global_joiner.wait();
}

// only good for small matrices; write out in dense format
void matrix_out( csr_graph * g, std::ostream& o, bool dense ) {
  for ( int64_t i = 0; i<g->nv; i++ ) {
    int64_t kbounds[2];
    Incoherent<int64_t>::RO cxoff( XOFF(g->xoff, i), 2, kbounds );
    const int64_t kstart = cxoff[0];
    const int64_t kend = cxoff[1];

    int64_t row[kend-kstart];
    Incoherent<int64_t>::RO crow( g->xadj + kstart, kend-kstart, row );

    if (dense) {  
      int64_t next_col = 0;
      for ( int64_t j = 0; j<g->nv; j++ ) {
        if ( next_col < (kend-kstart) && crow[next_col] == j ) {
          o << "1, ";
          next_col++;
        } else {
          o << "0, "; 
        }
      }
      o << "\n";
      CHECK (next_col==kend-kstart) << "mismatch, did not print all entries; next_col="<<next_col<<" kend-kstart=" <<kend-kstart;
    } else {
      o << "v=" << i << ";; ";
      for ( int64_t ri=0; ri<kend-kstart; ri++ ) {
        o << crow[ri] << ",";
      }
      o << "\n";
    }
  }
}

void vector_out( vector * v, std::ostream& o ) {
  Incoherent<double>::RO cv( v->a, v->length );
  for ( uint64_t i=0; i<v->length; i++ ) {
    o << cv[i] << ", ";
  }
}

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

DEFINE_uint64( nnz_factor, 10, "Approximate number of non-zeros per matrix row" );
DEFINE_uint64( logN, 16, "logN dimension of square matrix" );

void user_main( int * ignore ) {
  LOG(INFO) << "starting...";
 
  tuple_graph tg;
  csr_graph g;
  uint64_t N = (1L<<FLAGS_logN);

  uint64_t desired_nnz = FLAGS_nnz_factor * N;

  // results output
  DictOut resultd;

  double time;
  TIME(time, 
    make_graph( FLAGS_logN, desired_nnz, userseed, userseed, &tg.nedge, &tg.edges );
  );
  LOG(INFO) << "make_graph: " << time;
  resultd.add( "make_graph_time", time );

  TIME(time,
    create_graph_from_edgelist(&tg, &g);
  );
  LOG(INFO) << "tuple->csr: " << time;
  resultd.add( "tuple_to_csr_time", time );
  int64_t actual_nnz = g.nadj;
  resultd.add( "actual_nnz", actual_nnz );
  LOG(INFO) << "final matrix has " << static_cast<double>(actual_nnz)/N << " avg nonzeroes/row";

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
    matrix_mult_f f; f.matrix=g; f.src=vec; f.targ=target; fork_join_custom(&f);
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

}


    

/// Main() entry
int main (int argc, char** argv) {
    Grappa_init( &argc, &argv ); 
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );
    CHECK( Grappa_done() == true ) << "Grappa not done before scheduler exit";
    Grappa_finish( 0 );
}



