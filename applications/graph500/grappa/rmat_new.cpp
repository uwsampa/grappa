#include "../generator/utils.h"
#include "common.h"
#include <Grappa.hpp>
#include <ForkJoin.hpp>

static mrg_state prng_state;
static tuple_graph tg;
static GlobalAddress<int64_t> iwork;

inline int64_t nrand(int64_t ne) { return 5 * SCALE * ne; }

LOOP_FUNCTOR( setup_rmat, nid, ((tuple_graph,_tg)) ((GlobalAddress<int64_t>,_iwork))) {
  tg = _tg; iwork = _iwork;

  // Initialize random seed
  // Spread the two 64-bit numbers into five nonzero values in the correct range.
  uint64_t seed1 = 2, seed2 = 3;
  uint_fast32_t seed[5];
  make_mrg_seed(seed1, seed2, seed);
  mrg_seed(&prng_state, seed);
  
}

void compute_rmat_edge(packed_edge * e) {
  mrg_skip(&prng_state, 1, nrand(1), 0);
  
  double Rlocal[nrand(1)];
  for (int k2 = 0; k2 < nrand(1); k2++) {
    Rlocal[k2] = mrg_get_double_orig(&prng_state);
  }
  rmat_edge(e, 
} 

void rmat_edgelist(tuple_graph * tg, int64_t scale) {
  double t = Grappa_walltime();

  int64_t NV = 1L << scale;
  GlobalAddress<int64_t> iwork = Grappa_typed_malloc<int64_t>(NV);
  
  { setup_rmat f(*tg, iwork); fork_join_custom(&f); }

  forall_local<packed_edge, random_edge>(

  t = Grappa_walltime() - t;
  VLOG(1) << "randpermute_time: " << t;
}
