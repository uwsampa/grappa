#include "../generator/utils.h"
#include "common.h"
#include <math.h>

#include "SoftXMT.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "rmat.h"
#include "timer.h"
#include "options.h"


static mrg_state prng_state_store;
mrg_state * prng_state;

#define NRAND(ne) (5 * SCALE * (ne))

LOOP_FUNCTOR( func_set_seq, index, 
  (( GlobalAddress<int64_t>, base_addr))
  (( int64_t, value)) )
{ //    DVLOG(3) << "called func_initialize with index = " << index;
  Incoherent<int64_t>::RW c(base_addr+index, 1);
  c[0] = value+index;
}

#if 0 /* disabled parallel randpermute for faster builds */

struct randpermute_func : public ForkJoinIteration {
  GlobalAddress<int64_t> array;
  int64_t nelem;
  mrg_state st;
  void operator()(int64_t index) {
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

template< typename T >
struct swap_desc {
  GlobalAddress<T> addr1;
  GlobalAddress<T> addr2;
  GlobalAddress<int64_t> fb1;
  GlobalAddress<int64_t> fb2;
  GlobalAddress<Thread> caller;
  GlobalAddress<bool> done;
  int64_t k;
};

/// Note: do not call in am handler.
inline void wait_for_full(int64_t * fullptr) {
  while (*fullptr == 0) SoftXMT_yield(); // wait for other swapper to finish
  CHECK(*fullptr == 1) << "*fullptr == " << *fullptr;
}

static void am_swap_done(GlobalAddress<bool>* done, size_t sz, GlobalAddress<Thread>* caller, size_t psz) {
  CHECK(done->node() == SoftXMT_mynode());
  CHECK(caller->node() == SoftXMT_mynode());
  
  *done->pointer() = true;
  SoftXMT_wake(caller->pointer());
}

//template< typename T >
//static void am_swap_3(swap_desc<T> * desc, size_t sz, T* payload, size_t psz) {
////  assert(desc->addr1.node() == SoftXMT_mynode());
//  assert(desc->fb1.node() == SoftXMT_mynode());
////  assert(psz == sizeof(T));
//  assert(*desc->fb1.pointer() == 0); // should still be empty from am_swap_1
//  
////  *desc->addr1.pointer() = *payload;
//  
//  *desc->fb1.pointer() = 1;
//  
//  VLOG(2) << "about to call am_swap_done";
//  SoftXMT_call_on_x(desc->done.node(), &am_swap_done, &desc->done, sizeof(desc->done), &desc->caller, sizeof(desc->caller));
//}
//
//template< typename T >
//struct swap_struct_2 {
//  swap_desc<T> * desc;
//  T val1;
//  bool wait;
//};
//
//template< typename T >
//static void swap_task_2(swap_struct_2<T> * s) {
//  if (s->wait) {
//    wait_for_full(s->desc->fb2.pointer());
//  }
//  
//  T * val2 = s->desc->addr2.pointer();
//  T tmp = *val2;
//  *val2 = s->val1;
//  
//  VLOG(2) << "about to call am_swap_3";
//  SoftXMT_call_on_x(s->desc->addr1.node(), &am_swap_3<T>, s->desc, sizeof(*s->desc), &tmp, sizeof(tmp));
//}

template< typename T >
static void swap_task_2(swap_desc<T> * desc) {
//  assert(desc->addr2.node() == SoftXMT_mynode());
  CHECK(desc->fb2.node() == SoftXMT_mynode()) << "<" << desc->k << "> fb2.node() = " << desc->fb2.node();
  
  int64_t * fb2 = desc->fb2.pointer();
  VLOG(2) << "<" << desc->k << "> about to wait for addr2: " << fb2;
  wait_for_full(fb2);
  *fb2 = 0;
  
  typename Incoherent<T>::RW cval1(desc->addr1, 1);
  typename Incoherent<T>::RW cval2(desc->addr2, 1);
  
  cval1.block_until_acquired();
  cval2.block_until_acquired();
  
  T tmp = *cval2;
  *cval2 = *cval1;
  *cval1 = tmp;
  
  cval1.block_until_released();
  cval2.block_until_released();
  
  SoftXMT_delegate_write_word(desc->fb1, 1);
  *fb2 = 1;
  
//  VLOG(2) << "about to call am_swap_3";
//  SoftXMT_call_on_x(desc->fb1.node(), &am_swap_3<T>, desc);
  VLOG(2) << "<" << desc->k << "> about to call am_swap_done";
  SoftXMT_call_on_x(desc->done.node(), &am_swap_done, &desc->done, sizeof(desc->done), &desc->caller, sizeof(desc->caller));
}

/// Sketching a wrapper function?
//template< typename Func >
//static void wrapper(void* ptr) {
//  GlobalAddress<Func> addr = reinterpret_cast< GlobalAddress<Func> >(ptr);
//  Func f;
//  typename Incoherent<Func>::RW c(addr, 1, &f);
//  c.block_until_acquired();
//  f();
//  c.block_until_released();
//}
//
//template< typename FnTy, typename Args >
//static void wrapper(void* ptr) {
//  GlobalAddress<Args> addr = reinterpret_cast< GlobalAddress<Func> >(ptr);
//  Func f;
//  typename Incoherent<Func>::RW c(addr, 1, &f);
//  c.block_until_acquired();
//  f();
//  c.block_until_released();
//}

template< typename T >
static void swap_task_1(swap_desc<T>* desc) {
  
//  assert(desc->addr1.node() == SoftXMT_mynode());
  CHECK(desc->fb1.node() == SoftXMT_mynode()) << "<" << desc->k << "> fb1.node() = " << desc->fb1.node();
  
  VLOG(2) << "<" << desc-> k << "> about to wait for addr1: " << desc->fb1.pointer();
  wait_for_full(desc->fb1.pointer());
  *desc->fb1.pointer() = 0; //empty it again
//  T * val = desc->addr1.pointer();
  
  VLOG(2) << "<" << desc->k << "> calling am_swap_2";
  SoftXMT_remote_privateTask(&swap_task_2<T>, desc, desc->fb2.node());
}

template< typename T >
static void swap_globals(GlobalAddress<T> array, GlobalAddress<int64_t> fullbits, int64_t index1, int64_t index2) {
  
  swap_desc<T> desc;
  desc.k = index1;
  
  // ensure total ordering to avoid deadlock
  if (index1 == index2) return;
  if (index1 > index2) {
    int64_t tmp = index1;
    index1 = index2;
    index2 = tmp;
  }
  
  bool done = false;
  desc.addr1 = array+index1;
  desc.addr2 = array+index2;
  desc.fb1 = fullbits+index1;
  desc.fb2 = fullbits+index2;
  desc.caller = make_global(CURRENT_THREAD);
  desc.done = make_global(&done);
  
  VLOG(2) << "<" << desc.k << "> swapping " << index1 << " & " << index2;
  
  GlobalAddress< swap_desc<T> > ptr = make_global(&desc);
  SoftXMT_remote_privateTask(&swap_task_1<T>, reinterpret_cast< swap_desc<T>* >(ptr), desc.fb1.node());
  
  while (!done) SoftXMT_suspend();
  VLOG(2) << "<" << desc.k << "> DONE";
}

LOOP_FUNCTOR_TEMPLATED(T, rand_permute, k,
  (( GlobalAddress<T>, array ))
  (( GlobalAddress<int64_t>, fullbits ))
  (( mrg_state, st ))
  (( int64_t, nelem )) )
{
  mrg_state new_st = st;
  int64_t which = k % SoftXMT_nodes();
  mrg_skip(&new_st, 1, k*(which+1), 0);
  int64_t place = k + (int64_t)floor( mrg_get_double_orig(&new_st) * (nelem - k) );
    
  swap_globals(array, fullbits, k, place);
}

/// An attempt at a parallel version of randpermute that doesn't work yet.
template<typename T>
static void randpermute(GlobalAddress<T> array, int64_t nelem, mrg_state * restrict st) {
  //  typename Incoherent<T>::RW carray(array, nelem);
  GlobalAddress<int64_t> fullbits = SoftXMT_typed_malloc<int64_t>(nelem);
  func_set_const fc(fullbits, 1);
  fork_join(&fc, 0, nelem);

  rand_permute<T> f;
  f.array = array; f.fullbits = fullbits; f.st = *st; f.nelem = nelem;
  fork_join(&f, 0, nelem);
  SoftXMT_free(fullbits);
}
#endif /* /disabled parallel randpermute for faster builds */

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
    
    if (k % 4096 == 0) VLOG(1) << "k: " << k << " -- " << timer();
  }
}

LOOP_FUNCTOR( write_edge_func, index,
             (( GlobalAddress<int64_t>,newlabel ))
             (( GlobalAddress<packed_edge>, ij))
             (( int64_t, nedge )) )
{
  Incoherent<packed_edge>::RW edge(ij+index, 1);
  int64_t v0 = (*edge).v0;
  int64_t v1 = (*edge).v1;
  packed_edge new_edge;
  new_edge.v0 = SoftXMT_delegate_read_word(newlabel+v0);
  new_edge.v1 = SoftXMT_delegate_read_word(newlabel+v1);
  *edge = new_edge;
}

static void permute_vertex_labels (GlobalAddress<packed_edge> ij, int64_t nedge, int64_t max_nvtx, mrg_state * restrict st, GlobalAddress<int64_t> newlabel) {
	
  //  OMP("omp for")
  //	for (k = 0; k < max_nvtx; ++k)
  //		newlabel[k] = k;
  {
    func_set_seq f;
    f.base_addr = newlabel;
    f.value = 0.0;
    fork_join(&f, 0, max_nvtx);
  }
  
  //	randpermute(newlabel, max_nvtx, st);
  //	{
  //    randpermute_func f;
  //      f.array = newlabel;
  //      f.nelem = max_nvtx;
  //      f.st = *st;
  //    fork_join(current_thread, &f, 0, max_nvtx);
  //  }
  double t;
  TIME(t, randpermute(newlabel, max_nvtx, st)); // TODO: doing this serially for now...
  VLOG(1) << "done: randpermute (time = " << t << ")";
  
  //	OMP("omp for")
  //	for (k = 0; k < nedge; ++k)
  //		write_edge(&IJ[k],
  //               newlabel[get_v0_from_edge(&IJ[k])],
  //               newlabel[get_v1_from_edge(&IJ[k])]);
  t = timer();
  {
    write_edge_func f;
    f.newlabel = newlabel;
    f.ij = ij;
    fork_join(&f, 0, nedge);
  }
  t = timer() - t;
  VLOG(1) << "done write_edge_func (time = " << t << ")";
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

LOOP_FUNCTOR( random_edges_functor, index,
             (( GlobalAddress<packed_edge>, ij ))
             (( GlobalAddress<int64_t>, iwork  ))
             (( int64_t, nedge ))
             (( mrg_state*, prng_state ))
             (( int, SCALE )) )
{
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

LOOP_FUNCTOR( random_edges_node_work, mynode,
  ((mrg_state, local_prng_state))
  ((GlobalAddress<packed_edge>, edges))
  ((GlobalAddress<int64_t>, iwork))
  ((int64_t, nedge))
  ((int, SCALE)) )
{
  mrg_state mrg_here = local_prng_state;
  range_t myblock = blockDist(0, nedge, mynode, SoftXMT_nodes());
  random_edges_functor f;
  f.ij = edges;
  f.iwork = iwork;
  f.nedge = nedge;
  f.prng_state = &mrg_here;
  f.SCALE = SCALE;
  fork_join_onenode(&f, myblock.start, myblock.end);
}

void rmat_edgelist(tuple_graph* grin, int64_t SCALE) {
  uint64_t seed1 = 2, seed2 = 3;
  
  /* Spread the two 64-bit numbers into five nonzero values in the correct
   * range. */
  uint_fast32_t seed[5];
  make_mrg_seed(seed1, seed2, seed);
  mrg_seed(&prng_state_store, seed);
  prng_state = &prng_state_store;
  
  int64_t NV = 1L<<SCALE;
  GlobalAddress<int64_t> iwork = SoftXMT_typed_malloc<int64_t>(NV);
  
  double t;
  t = timer();
  {
    random_edges_node_work f;
    f.edges = grin->edges;
    f.iwork = iwork;
    f.nedge = grin->nedge;
    f.local_prng_state = prng_state_store;
    f.SCALE = SCALE;
    fork_join_custom(&f);
  }
  t = timer() - t;
  VLOG(1) << "done: random_edges forkjoin (time = " << t << ")";
  
  mrg_skip(&prng_state_store, 1, NRAND(grin->nedge), 0);
  
  TIME(t, permute_vertex_labels(grin->edges, grin->nedge, NV, &prng_state_store, iwork));
  
  VLOG(1) << "done: permute_vertex_labels (time = " << t << ")";
  
  mrg_skip(&prng_state_store, 1, NV, 0);
  
  t = timer();
  randpermute(grin->edges, grin->nedge, &prng_state_store);
  t = timer() - t;
  VLOG(1) << "done: randpermute (time = " << t << ")";
}
