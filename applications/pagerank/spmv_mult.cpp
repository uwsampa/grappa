
#include "spmv_mult.hpp"

// Grappa
#include "Grappa.hpp"
#include "Cache.hpp"
#include "GlobalTaskJoiner.hpp"
#include "ForkJoin.hpp"
#include "Delegate.hpp"


#define XOFF(xoff, index) (xoff)+2*(index)
#define XENDOFF(matrix, index) (xoff)+2*(index)+1

namespace spmv {
  // global: local per Node
  weighted_csr_graph m;
  vector v;
  vector y;
}

void inner_loop( int64_t start, int64_t iters, GlobalAddress<int64_t> targetIndex_h ) {
  // TODO don't use hack
  int64_t targetIndex = reinterpret_cast<int64_t>(targetIndex_h.pointer());

  double yaccum = 0;
  int64_t j[iters];
  double weights[iters];
  Incoherent<int64_t>::RO cj( spmv::m.xadj + start, iters, j ); cj.start_acquire();
  Incoherent<double>::RO cw( spmv::m.adjweight + start, iters, weights ); cw.start_acquire();
  for( int64_t k = 0; k<iters; k++ ) {
    double vj;
    
    Grappa_delegate_read(spmv::v.a + cj[k], &vj);
    yaccum += cw[k] * vj; 
    DVLOG(5) << "yaccum += w["<<start+k<<"]("<<cw[k] <<") * val["<<cj[k]<<"("<<vj<<")"; 
  }

  DVLOG(4) << "y[" << targetIndex << "] += " << yaccum; 
  ff_delegate_add<double>( spmv::y.a+targetIndex, yaccum );
}

void row_loop( int64_t start, int64_t iters ) {
  //VLOG(1) << "rows [" << start << ", " << start+iters << ")";
  
  for ( int64_t i=start; i<start+iters; i++ ) {
    // kstart = row_ptr[i], kend = row_ptr[i+1]
    int64_t kbounds[2];
    Incoherent<int64_t>::RO cxoff( XOFF(spmv::m.xoff, i), 2, kbounds );
    int64_t kstart = cxoff[0];
    int64_t kend = cxoff[1];

    // TODO: don't use hack to wrap i
    async_parallel_for<int64_t,inner_loop, joinerSpawn_hack<int64_t,inner_loop,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(kstart, kend-kstart, make_global(reinterpret_cast<int64_t*>(i)));
    
                               // TODO: if yi.start_release() could guarentee the storage is no longer used
                               //       then this would be nice for no block until all y[start,start+iters) need to be written, although that pressures the network side
                               // Y could actually be like a feed forward delegate that syncs at global join end
  }
}
// In general, destination arrays that are either associatively accumulated into or written once into can often get by with very weak consistency: sync at very end once value of vector needs to be known valid result.
    
// With current low level programming model, the choice to parallelize a loop involves different code


DEFINE_bool(row_distribute, true, "nodes begin with equal number of row tasks");

LOOP_FUNCTOR( matrix_mult_f, nid, ((weighted_csr_graph,matrix)) ((vector,src)) ((vector,targ)) ) {
  spmv::m = matrix;
  spmv::v = src;
  spmv::y = targ;

  global_joiner.reset();
  if ( FLAGS_row_distribute ) {
    range_t r = blockDist(0, spmv::m.nv, Grappa_mynode(), Grappa_nodes());
    async_parallel_for<row_loop, joinerSpawn<row_loop,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(r.start, r.end-r.start);
  } else {
    if ( nid == 0 ) {
    async_parallel_for<row_loop, joinerSpawn<row_loop,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT>(0, spmv::m.nv);
    }
  }
  global_joiner.wait();
}

void spmv_mult( weighted_csr_graph A, vector x, vector y ) {
  matrix_mult_f f; f.matrix=A; f.src=x; f.targ=y; fork_join_custom(&f);
}


// only good for small matrices; write out in dense format
void matrix_out( weighted_csr_graph * g, std::ostream& o, bool dense ) {
  for ( int64_t i = 0; i<g->nv; i++ ) {
    int64_t kbounds[2];
    Incoherent<int64_t>::RO cxoff( XOFF(g->xoff, i), 2, kbounds );
    const int64_t kstart = cxoff[0];
    const int64_t kend = cxoff[1];

    int64_t row[kend-kstart];
    Incoherent<int64_t>::RO crow( g->xadj + kstart, kend-kstart, row );

    double weights[kend-kstart];
    Incoherent<double>::RO cweights( g->adjweight + kstart, kend-kstart, weights );

    if (dense) {  
      int64_t next_col = 0;
      for ( int64_t j = 0; j<g->nv; j++ ) {
        if ( next_col < (kend-kstart) && crow[next_col] == j ) {
          o << cweights[next_col] << ", ";
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



void R_matrix_out( weighted_csr_graph * g, std::ostream& o) {
  o << "matrix(data=c("; 

  for ( int64_t i = 0; i<g->nv; i++ ) {
    int64_t kbounds[2];
    Incoherent<int64_t>::RO cxoff( XOFF(g->xoff, i), 2, kbounds );
    const int64_t kstart = cxoff[0];
    const int64_t kend = cxoff[1];

    int64_t row[kend-kstart];
    double weights[kend-kstart];
    Incoherent<int64_t>::RO crow( g->xadj + kstart, kend-kstart, row );
    Incoherent<double>::RO cw( g->adjweight + kstart, kend-kstart, weights );

    int64_t next_col = 0;
    for ( int64_t j = 0; j<g->nv; j++ ) {
      if ( next_col < (kend-kstart) && crow[next_col] == j ) {
        o << cw[next_col];
        next_col++;
      } else {
        o << "0"; 
      }
      if (!(j==g->nv-1 && i==g->nv-1)) o <<", ";
    }
    CHECK (next_col==kend-kstart) << "mismatch, did not print all entries; next_col="<<next_col<<" kend-kstart=" <<kend-kstart;
  }
  o << ")";
  o << ", nrow=" << g->nv;
  o << ", ncol=" << g->nv;
  o << ")";
}

void vector_out( vector * v, std::ostream& o ) {
  Incoherent<double>::RO cv( v->a, v->length );
  for ( uint64_t i=0; i<v->length; i++ ) {
    o << cv[i] << ", ";
  }
}

void R_vector_out( vector * v, std::ostream& o ) {
  o << "c(";
  Incoherent<double>::RO cv( v->a, v->length );
  for ( uint64_t i=0; i<v->length-1; i++ ) {
    o << cv[i] << ", ";
  }
  o << cv[v->length-1];
  o << ")";
}
