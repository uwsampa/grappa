
#include "spmv_mult.hpp"

// Grappa
#include <Grappa.hpp>
#include <Cache.hpp>
#include <GlobalCompletionEvent.hpp>
#include <ParallelLoop.hpp>
#include <Delegate.hpp>
#include <AsyncDelegate.hpp>
#include <MessagePool.hpp>


#define XOFF(xoff, index) (xoff)+2*(index)
#define XENDOFF(matrix, index) (xoff)+2*(index)+1

namespace spmv {
  // global: local per Node
  weighted_csr_graph m;
  vector x;
  vector y;
}

using namespace Grappa;

    
// With current low level programming model, the choice to parallelize a loop involves different code

GlobalCompletionEvent mmjoiner;
void spmv_mult( weighted_csr_graph A, vector x, vector y ) {
  // cannot capture in all for loops, so just set on all cores
  on_all_cores( [A,x,y] {
    spmv::m = A;
    spmv::x = x;
    spmv::y = y;
  });

  // forall rows
  forall_global_public<&mmjoiner>( 0, A.nv, []( int64_t start, int64_t iters ) {
    // serialized chunk of rows
    DVLOG(5) << "rows [" << start << ", " << start+iters << ")";
    for (int64_t i=start; i<start+iters; i++ ) {
      // kstart = row_ptr[i], kend = row_ptr[i+1]
      int64_t kbounds[2];
      Incoherent<int64_t>::RO cxoff( XOFF(spmv::m.xoff, i), 2, kbounds );
      int64_t kstart = cxoff[0];
      int64_t kend = cxoff[1];

      // forall non-zero columns (parallel dot product)
      forall_here_async_public<&mmjoiner>( kstart, kend-kstart, [i]( int64_t start, int64_t iters ) {
        double yaccum = 0;

        // serialized chunk of columns
        int64_t j[iters];
        double weights[iters];
        Incoherent<int64_t>::RO cj( spmv::m.xadj + start, iters, j ); cj.start_acquire();
        Incoherent<double>::RO cw( spmv::m.adjweight + start, iters, weights ); cw.start_acquire();
        for( int64_t k = 0; k<iters; k++ ) {
          double vj = Grappa::delegate::read(spmv::x.a + cj[k]);
          yaccum += cw[k] * vj; 
          DVLOG(5) << "yaccum += w["<<start+k<<"]("<<cw[k] <<") * val["<<cj[k]<<"("<<vj<<")"; 
        }

        DVLOG(4) << "y[" << i << "] += " << yaccum; 

        char pool_storage[sizeof(delegate::write_msg_proxy<double>)];  // TODO: trait on call_async not to use pool
        MessagePool pool( pool_storage, sizeof(pool_storage) );
        delegate::increment_async<&mmjoiner>(pool, spmv::y.a+i, yaccum);
        // could force local updates and bulk communication 
        // here instead of relying on aggregation
        // since we have to pay the cost of many increments and replies
      });
    }
  });
}
                               // TODO: if yi.start_release() could guarentee the storage is no longer used
                               //       then this would be nice for no block until all y[start,start+iters) need to be written, although that pressures the network side
                               // Y could actually be like a feed forward delegate that syncs at global join end
// In general, destination arrays that are either associatively accumulated into or written once into can often get by with very weak consistency: sync at very end once value of vector needs to be known valid result.


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
