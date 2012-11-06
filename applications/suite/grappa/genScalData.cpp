#include "defs.hpp"
#include "Grappa.hpp"
#include "GlobalAllocator.hpp"
#include "ForkJoin.hpp"

LOOP_FUNCTOR( func_set_seq, k, (( GlobalAddress<graphint>, base_addr)) ) {
  Incoherent<graphint>::WO c(base_addr+k, 1);
  c[0] = k;
}

static void xmtcompat_initialize_rand(void) {
  static bool xmtcompat_rand_initialized = false;
  if (!xmtcompat_rand_initialized) {
    srand48(894893uL); /* constant picked by hitting the keyboard */
    xmtcompat_rand_initialized = true;
  }
}

static void prand(int64_t n, double * v) {
  int64_t i;
  xmtcompat_initialize_rand();
  for (i = 0; i < n; ++i) {
		v[i] = drand48();
	}
}

double genScalData(graphedges * ge, double a, double b, double c, double d) {
  GlobalAddress<double> rn = Grappa_typed_malloc<double>(2*numVertices);
  GlobalAddress<graphint> permV = Grappa_typed_malloc<graphint>(numVertices);
  
  double t = timer();
  
  /*-------------------------------------------------------------------------*/
	/* STEP 0: Create the permutations required to randomize the vertices      */
	/*-------------------------------------------------------------------------*/
  func_set_seq fseq(permV);
  fork_join(&fseq, 0, numVertices);
  
	/* Permute indices SCALE * NV times                 */
	/* Perform in sets of NV permutations to save space */
  for (int j = 0; j < SCALE; j++) {
    graphint n = 2 * numVertices;
    //prand(n, rn);
  }
  
  t = timer() - t;
  
  LOG(INFO) << "done";
  
  return t;
}
