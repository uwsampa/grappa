

#include "Grappa.hpp"

struct csr_matrix {
  GlobalAddress<int64_t> val, col_ind, row_ptr;
  int64_t nnz; // also stored as last element of col_ind[]
};

struct vector {
  GlobalAddress<int64_t> a;
  int64_t length;
};

struct row_bounds_t {
  int64_t start;
  int64_t end;
};

void row_loop( int64_t start, int64_t iters ) {
  for ( int64_t i=start; i<start+iters; i++ ) {
    // y[i] = 0
    int64_t initval;
    Incoherent::WO<int64_t> yi( y.a+i, 1, &initval );
    *yi = 0;

    // kstart = row_ptr[i], kend = row_ptr[i+1]
    row_bounds_t kbounds;
    Incoherent::RO<row_bounds_t> rp( static_cast<GlobalAddress<row_bounds_t>(m.row_ptr + i), 1, &kbounds );
    kstart = rp->start;
    kend = rp->end;
    rp.start_release();

    // Column loop
    // TODO parallel for, but will have different guarentees on y released
    for ( k = kstart; k<kend; k++ ) {
      int64_t j, mv;
      Incoherent::RO<int64_t> cols_k( m.col_ind + k, 1, &j );
      Incoherent::RO<int64_t> vals_k( m.val + k, 1, &mv );
      yi += *vals_k * x[j];
      cols_k.block_until_released(); // RO so not actually blocking, but descoped eventual release is nice
      vals_k.block_until_released(); // ""
    } 


    yi.block_until_released(); // write out the final y[i]
    rp.block_until_released();
                               // TODO: could also go outside the for loop
                               // TODO: if yi.start_release() could guarentee the storage is no longer used
                               //       then this would be nice for no block until all y[start,start+iters) need to be written, although that pressures the network side
                               // Y could actually be like a feed forward delegate that syncs at global join end
  }
}
// In general, destination arrays that are either associatively accumulated into or written once into can often get by with very weak consistency: sync at very end once value of vector needs to be known valid result.
    
// With current low level programming model, the choice to parallelize a loop involves different code

void sparse_matrix_vector_multiply( csr_matrix * m, vector * x, vector * y ) 
{


