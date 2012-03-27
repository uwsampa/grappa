#include "../generator/utils.h"
#include "common.h"
#include <math.h>

#include "SoftXMT.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "rmat.h"

double A = 0.57;
double B = 0.19;
double C = 0.19;
double D = 1.0-(A+B+C);

static mrg_state prng_state_store;

#define NRAND(ne) (5 * SCALE * (ne))

struct func_initialize : public ForkJoinIteration {
  GlobalAddress<int64_t> base_addr;
  int64_t value;
  void operator()(thread * me, int64_t index) {
    DVLOG(1) << "called func_initialize with index = " << index;
    Incoherent<int64_t>::RW c(base_addr+index, 1);
    c[0] = value+index;
  }
};

struct randpermute_func : public ForkJoinIteration {
  GlobalAddress<int64_t> array;
  int64_t nelem;
  mrg_state st;
  void operator()(thread * me, int64_t index) {
    int64_t k = index;
    int64_t place;
    //    elt_type Ak, Aplace;
    //    Ak = take (&array[k]);
    Incoherent<int64_t>::RW Ak(array+k, 1);
    
    mrg_skip(&st, 1, k, 0);
    
    place = k + (int64_t)floor( mrg_get_double_orig(&st) * (nelem - k) );
    if (k != place) {
      assert (place > k);
      assert (place < nelem);
      //      Aplace = take (&array[place]);
      Incoherent<int64_t>::RW Aplace(array+place, 1);
      //      {
      //        int64_t t;
      //        t = array[place];
      //        array[place] = array[k];
      //        array[k] = t;
      //      }
      //      {
      //        elt_type t;
      //        t = Aplace;
      //        Aplace = Ak;
      //        Ak = t;
      //      }
      //      release (&array[place], Aplace);
    }
    //    release (&array[k], Ak);
  }
};

// temporarily just do it serially on one node rather than doing parallel swaps
template<typename T>
static void randpermute(GlobalAddress<T> array, int64_t nelem, mrg_state * restrict st) {
  //  typename Incoherent<T>::RW carray(array, nelem);
  
  for (int64_t k=0; k < nelem; k++) {
    mrg_state new_st = *st;
    int64_t which = k % SoftXMT_nodes();
    mrg_skip(&new_st, 1, k*(which+1), 0);
    int64_t place = k + (int64_t)floor( mrg_get_double_orig(&new_st) * (nelem - k) );
    
    typename Incoherent<T>::RW c_this(array+k, 1);
    typename Incoherent<T>::RW c_place(array+place, 1);
    
    // swap
    T t;
    t = *c_place;
    *c_place = *c_this;
    *c_this = t;
  }
}

struct write_edge_func : public ForkJoinIteration {
  GlobalAddress<int64_t> newlabel;
  GlobalAddress<packed_edge> ij;
  int64_t nedge;
  void operator()(thread * me, int64_t index) {
    Incoherent<packed_edge>::RW edge(ij+index, 1);
    int64_t v0 = (*edge).v0;
    int64_t v1 = (*edge).v1;
    packed_edge new_edge;
    new_edge.v0 = SoftXMT_delegate_read_word(newlabel+v0);
    new_edge.v1 = SoftXMT_delegate_read_word(newlabel+v1);
    *edge = new_edge;
  }
};

static void permute_vertex_labels (GlobalAddress<packed_edge> ij, int64_t nedge, int64_t max_nvtx, mrg_state * restrict st, GlobalAddress<int64_t> newlabel) {
	
  //  OMP("omp for")
  //	for (k = 0; k < max_nvtx; ++k)
  //		newlabel[k] = k;
  {
    func_initialize f;
    f.base_addr = newlabel;
    f.value = 0.0;
    fork_join(current_thread, &f, 0, max_nvtx);
  }
  
  //	randpermute(newlabel, max_nvtx, st);
  //	{
  //    randpermute_func f;
  //      f.array = newlabel;
  //      f.nelem = max_nvtx;
  //      f.st = *st;
  //    fork_join(current_thread, &f, 0, max_nvtx);
  //  }
  randpermute(newlabel, max_nvtx, st); // TODO: doing this serially for now...
	
  VLOG(1) << "done: randpermute";
  
  //	OMP("omp for")
  //	for (k = 0; k < nedge; ++k)
  //		write_edge(&IJ[k],
  //               newlabel[get_v0_from_edge(&IJ[k])],
  //               newlabel[get_v1_from_edge(&IJ[k])]);
  {
    write_edge_func f;
    f.newlabel = newlabel;
    f.ij = ij;
    fork_join(current_thread, &f, 0, nedge);
  }
}

static void rmat_edge(packed_edge *out, int SCALE, double A, double B, double C, double D, const double *rn) {
	size_t rni = 0;
	int64_t i = 0, j = 0;
	int64_t bit = ((int64_t)1) << (SCALE-1);
	
	while (1) {
		const double r = rn[rni++];
		if (r > A) { /* outside quadrant 1 */
			if (r <= A + B) /* in quadrant 2 */
				j |= bit;
			else if (r <= A + B + C) /* in quadrant 3 */
				i |= bit;
			else { /* in quadrant 4 */
				j |= bit;
				i |= bit;
			}
		}
		if (1 == bit) break;
		
		/*
		 Assuming R is in (0, 1), 0.95 + 0.1 * R is in (0.95, 1.05).
		 So the new probabilities are *not* the old +/- 10% but
		 instead the old +/- 5%.
		 */
		A *= 0.95 + rn[rni++]/10;
		B *= 0.95 + rn[rni++]/10;
		C *= 0.95 + rn[rni++]/10;
		D *= 0.95 + rn[rni++]/10;
		/* Used 5 random numbers. */
		
		{
			const double norm = 1.0 / (A + B + C + D);
			A *= norm; B *= norm; C *= norm;
		}
		/* So long as +/- are monotonic, ensure a+b+c+d <= 1.0 */
		D = 1.0 - (A + B + C);
		
		bit >>= 1;
	}
	/* Iterates SCALE times. */
	write_edge(out, i, j);
}

struct random_edges_functor : public ForkJoinIteration {
  GlobalAddress<packed_edge> ij;
  GlobalAddress<int64_t> iwork;
  int64_t nedge;
  mrg_state * prng_state;
  int SCALE;
  void operator()(thread * me, int64_t index) {
    double * restrict Rlocal = (double*)alloca(NRAND(1) * sizeof(double));
    mrg_skip(prng_state, 1, NRAND(1), 0);
    for (int64_t i=0; i < NRAND(1); i++) {
      Rlocal[i] = mrg_get_double_orig(prng_state);
    }
    packed_edge new_edge;
    rmat_edge(&new_edge, this->SCALE, A, B, C, D, Rlocal);
    
    Incoherent<packed_edge>::RW cedge(ij+index, 1);
    *cedge = new_edge;
  }
};

struct random_edges_node_work : public ForkJoinIteration {
  mrg_state local_prng_state;
  GlobalAddress<packed_edge> edges;
  GlobalAddress<int64_t> iwork;
  int64_t nedge;
  int SCALE;
  void operator()(thread * me, int64_t mynode) {
    range_t myblock = blockDist(0, nedge, mynode, SoftXMT_nodes());
    random_edges_functor f;
    f.ij = edges;
    f.iwork = iwork;
    f.nedge = nedge;
    f.prng_state = &local_prng_state;
    f.SCALE = SCALE;
    fork_join_onenode(me, &f, myblock.start, myblock.end);
  }
};

void rmat_edgelist(tuple_graph* grin, int SCALE) {
  uint64_t seed1 = 2, seed2 = 3;
  
  /* Spread the two 64-bit numbers into five nonzero values in the correct
   * range. */
  uint_fast32_t seed[5];
  make_mrg_seed(seed1, seed2, seed);
  mrg_seed(&prng_state_store, seed);
  
  int64_t NV = 1L<<SCALE;
  GlobalAddress<int64_t> iwork = SoftXMT_typed_malloc<int64_t>(NV);
  
  {
    random_edges_node_work f;
    f.edges = grin->edges;
    f.iwork = iwork;
    f.nedge = grin->nedge;
    f.local_prng_state = prng_state_store;
    f.SCALE = SCALE;
    fork_join_custom(current_thread, &f);
  }
  VLOG(1) << "done: random_edges forkjoin";
  
  mrg_skip(&prng_state_store, 1, NRAND(grin->nedge), 0);
  
  permute_vertex_labels(grin->edges, grin->nedge, NV, &prng_state_store, iwork);
  
  VLOG(1) << "done: permute_vertex_labels";
  
  mrg_skip(&prng_state_store, 1, NV, 0);
  
  randpermute(grin->edges, grin->nedge, &prng_state_store);
}
