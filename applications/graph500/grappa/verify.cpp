#include "Grappa.hpp"
#include "Addressing.hpp"
#include "Cache.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"
#include "Collective.hpp"
#include "GlobalTaskJoiner.hpp"
#include "PerformanceTools.hpp"
#include <ParallelLoop.hpp>
#include <Array.hpp>

#include "verify.hpp"

#include "oned_csr.h"
#include "timer.h"
#include "options.h"

using namespace Grappa;

static int64_t nedge_traversed;

void compute_levels(GlobalAddress<int64_t> level, int64_t nv, GlobalAddress<int64_t> bfs_tree, int64_t root) {
  
  Grappa::memset(level, (int64_t)-1, nv);
  delegate::write(level+root, 0);
  
  forall_localized(level, nv, [bfs_tree, level, nv, root] (int64_t k, int64_t& level_k) {
    if (level_k >= 0) return;
  
    int64_t tree_k = delegate::read(bfs_tree+k);
    if (tree_k >= 0 && k != root) {
      int64_t parent = k;
      int64_t nhop = 0;
      int64_t next_parent;
    
      /* Run up the three until we encounter an already-leveled vertex. */
      while (parent >= 0 && delegate::read(level+parent) < 0 && nhop < nv) {
        next_parent = delegate::read(bfs_tree+parent);
        assert(parent != next_parent);
        parent = next_parent;
        ++nhop;
      }
      CHECK(nhop < nv) << "Error: root " << k << " had a cycle.";
      CHECK(parent >= 0) << "Ran off the end for root " << k << ".";
    
      // Now assign levels until we meet an already-leveled vertex
      // NOTE: This permits benign races if parallelized.
      nhop += delegate::read(level+parent);
      parent = k;
      while (delegate::read(level+parent) < 0) {
        CHECK_GT(nhop, 0);
        delegate::write(level+parent, nhop);
        nhop--;
        parent = delegate::read(bfs_tree+parent);
      }
      CHECK_EQ(nhop, delegate::read(level+parent));
    
      // Internal check to catch mistakes in races...
#if defined(DEBUG)
      nhop = 0;
      parent = k;
      int64_t lastlvl = delegate::read(level+k) + 1;
      while ((next_parent = delegate::read(level+parent)) > 0) {
        CHECK_EQ(lastlvl, (1+next_parent));
        lastlvl = next_parent;
        parent = delegate::read(bfs_tree+parent);
        nhop++;
      }
#endif
    }
  });  
}

static int64_t load_nedge(int64_t root, GlobalAddress<int64_t> bfs_tree) {
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld%s.nedge", SCALE, edgefactor, root, (use_RMAT)?".rmat":"");
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    LOG(ERROR) << "Unable to open file: " << fname << ", will do verify manually and save checkpoint.";
    return -1;
  }
  
  int64_t nedge_traversed;
  fread(&nedge_traversed, sizeof(nedge_traversed), 1, fin);
  return nedge_traversed;
}

static void save_nedge(int64_t root, int64_t nedge_traversed, GlobalAddress<int64_t> bfs_tree) {
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.%lld%s.nedge", SCALE, edgefactor, root, (use_RMAT)?".rmat":"");
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    LOG(ERROR) << "Unable to open file for writing: " << fname;
    return;
  }
  
  fwrite(&nedge_traversed, sizeof(nedge_traversed), 1, fout);
  
  fclose(fout);
}

int64_t verify_bfs_tree(GlobalAddress<int64_t> bfs_tree, int64_t max_bfsvtx, int64_t root, tuple_graph * tg) {
  
  static int nverified = 0;
  if (nverified > 0) {
    LOG(INFO) << "warning: skipping verification!!";    
    return nedge_traversed;
  }
  nverified++;
  
  //TAU_PHASE("verify_bfs_tree", "int64_t (GlobalAddress<int64_t>,int64_t,int64_t,tuple_graph*)", TAU_USER);
  
  CHECK_EQ(delegate::read(bfs_tree+root), root);
  
  int64_t nv = max_bfsvtx+1;
  
  if (!verify) {
    LOG(INFO) << "warning: skipping verification!!";
    nedge_traversed = load_nedge(root, bfs_tree);

    if (nedge_traversed != -1) {
      return nedge_traversed;
    }
  }
  
  int64_t err = 0;
  
  GlobalAddress<int64_t> seen_edge = Grappa_typed_malloc<int64_t>(nv);
  GlobalAddress<int64_t> level = Grappa_typed_malloc<int64_t>(nv);
  
  double t;
  TIME(t, compute_levels(level, nv, bfs_tree, root));
  VLOG(1) << "compute_levels time: " << t;
  
  t = timer();
  Grappa::memset(seen_edge, 0, nv);
  t = timer() - t;
  VLOG(1) << "set_const time: " << t;
  
  t = timer();
  
  call_on_all_cores([]{ nedge_traversed = 0; });
    
  forall_localized(tg->edges, tg->nedge, [seen_edge, bfs_tree, level, max_bfsvtx]
      (int64_t e, packed_edge& cedge) {  
    const int64_t i = cedge.v0;
    const int64_t j = cedge.v1;
    int64_t lvldiff;

    if (i < 0 || j < 0) return;
    CHECK(!(i > max_bfsvtx && j <= max_bfsvtx)) << "Error!";
    CHECK(!(j > max_bfsvtx && i <= max_bfsvtx)) << "Error!";
    if (i > max_bfsvtx) // both i & j are on the same side of max_bfsvtx
      return;

    // All neighbors must be in the tree.
    int64_t ti = delegate::read(bfs_tree+i);
    int64_t tj = delegate::read(bfs_tree+j);

    CHECK(!(ti >= 0 && tj < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    CHECK(!(tj >= 0 && ti < 0)) << "Error! ti=" << ti << ", tj=" << tj;
    if (ti < 0) // both i & j have the same sign
      return;

    /* Both i and j are in the tree, count as a traversed edge.
 
     NOTE: This counts self-edges and repeated edges.  They're
     part of the input data.
     */
    nedge_traversed++;

    // Mark seen tree edges.
    if (i != j) {
      if (ti == j)
        delegate::write(seen_edge+i, 1);
      if (tj == i)
        delegate::write(seen_edge+j, 1);
    }
    lvldiff = delegate::read(level+i) - delegate::read(level+j);
    /* Check that the levels differ by no more than one. */
    CHECK(!(lvldiff > 1 || lvldiff < -1)) << "Error, levels differ by more than one! "
      << "(k = " << e << ", lvl[" << i << "]=" << delegate::read(level+i) << ", lvl[" << j << "]=" << delegate::read(level+j) << ")";
  });
  nedge_traversed = Grappa::reduce<int64_t,collective_add>(&nedge_traversed);
  
  t = timer() - t;
  VLOG(1) << "verify_func time: " << t;

  /* Check that every BFS edge was seen and that there's only one root. */
  forall_localized(bfs_tree, nv, [root, seen_edge](int64_t k, int64_t& tk){
    if (k != root) {
      CHECK(!(tk >= 0 && !delegate::read(seen_edge+k))) << "Error!";
      CHECK_NE(tk, k) << "Error!";
    }
  });  
  VLOG(1) << "final_verify_func time: " << t;  
  
  Grappa_free(seen_edge);
  Grappa_free(level);
    
  // everything checked out...
  if (!verify) save_nedge(root, nedge_traversed, bfs_tree);
    
  return nedge_traversed;
}

////////////////////////////////////////////////////////////////
// Graph500 stats output
#define PRId64 "ld"
#define NSTAT 9
#define PRINT_STATS(lbl, israte)					\
  do {									\
    printf ("min_%s: %20.17e\n", lbl, stats[0]);			\
    printf ("firstquartile_%s: %20.17e\n", lbl, stats[1]);		\
    printf ("median_%s: %20.17e\n", lbl, stats[2]);			\
    printf ("thirdquartile_%s: %20.17e\n", lbl, stats[3]);		\
    printf ("max_%s: %20.17e\n", lbl, stats[4]);			\
    if (!israte) {							\
      printf ("mean_%s: %20.17e\n", lbl, stats[5]);			\
      printf ("stddev_%s: %20.17e\n", lbl, stats[6]);			\
    } else {								\
      printf ("harmonic_mean_%s: %20.17e\n", lbl, stats[7]);		\
      printf ("harmonic_stddev_%s: %20.17e\n", lbl, stats[8]);	\
    }									\
  } while (0)

static int
dcmp (const void *a, const void *b)
{
	const double da = *(const double*)a;
	const double db = *(const double*)b;
	if (da > db) return 1;
	if (db > da) return -1;
	if (da == db) return 0;
	fprintf (stderr, "No NaNs permitted in output.\n");
	abort ();
	return 0;
}

static void get_statistics(const double x[], int n, double r[s_LAST]) {
  double temp;
  int i;
  /* Compute mean. */
  temp = 0;
  for (i = 0; i < n; ++i) temp += x[i];
  temp /= n;
  r[s_mean] = temp;
  /* Compute std. dev. */
  temp = 0;
  for (i = 0; i < n; ++i) temp += (x[i] - r[s_mean]) * (x[i] - r[s_mean]);
  temp /= n - 1;
  r[s_std] = sqrt(temp);
  /* Sort x. */
  double* xx = (double*)xmalloc(n * sizeof(double));
  memcpy(xx, x, n * sizeof(double));
  qsort(xx, n, sizeof(double), compare_doubles);
  /* Get order statistics. */
  r[s_minimum] = xx[0];
  r[s_firstquartile] = (xx[(n - 1) / 4] + xx[n / 4]) * .5;
  r[s_median] = (xx[(n - 1) / 2] + xx[n / 2]) * .5;
  r[s_thirdquartile] = (xx[n - 1 - (n - 1) / 4] + xx[n - 1 - n / 4]) * .5;
  r[s_maximum] = xx[n - 1];
  /* Clean up. */
  free(xx);
}

static void statistics(double *out, double *data, int64_t n) {
	long double s, mean;
	double t;
	int64_t k;
	
	/* Quartiles */
	qsort (data, n, sizeof (*data), dcmp);
	out[0] = data[0];
	t = (n+1) / 4.0;
	k = (int) t;
	if (t == k)
		out[1] = data[k];
	else
		out[1] = 3*(data[k]/4.0) + data[k+1]/4.0;
	t = (n+1) / 2.0;
	k = (int) t;
	if (t == k)
		out[2] = data[k];
	else
		out[2] = data[k]/2.0 + data[k+1]/2.0;
	t = 3*((n+1) / 4.0);
	k = (int) t;
	if (t == k)
		out[3] = data[k];
	else
		out[3] = data[k]/4.0 + 3*(data[k+1]/4.0);
	out[4] = data[n-1];
	
	s = data[n-1];
	for (k = n-1; k > 0; --k)
		s += data[k-1];
	mean = s/n;
	out[5] = mean;
	s = data[n-1] - mean;
	s *= s;
	for (k = n-1; k > 0; --k) {
		long double tmp = data[k-1] - mean;
		s += tmp * tmp;
	}
	out[6] = sqrt (s/(n-1));
	
	s = (data[0]? 1.0L/data[0] : 0);
	for (k = 1; k < n; ++k)
		s += (data[k]? 1.0L/data[k] : 0);
	out[7] = n/s;
	mean = s/n;
	
	/*
	 Nilan Norris, The Standard Errors of the Geometric and Harmonic
	 Means and Their Application to Index Numbers, 1940.
	 http://www.jstor.org/stable/2235723
	 */
	s = (data[0]? 1.0L/data[0] : 0) - mean;
	s *= s;
	for (k = 1; k < n; ++k) {
		long double tmp = (data[k]? 1.0L/data[k] : 0) - mean;
		s += tmp * tmp;
	}
	s = (sqrt (s)/(n-1)) * out[7] * out[7];
	out[8] = s;
}

void output_results(const int64_t SCALE, int64_t nvtx_scale, int64_t edgefactor,
                    const double A, const double B, const double C, const double D,
                    const double generation_time,
                    const double construction_time,
                    const int NBFS, const double *bfs_time, const int64_t *bfs_nedge) {
	int k;
	int64_t sz;
	double tm[NBFS];
	double stats[NSTAT];
	
	if (!tm || !stats) {
		perror ("Error allocating within final statistics calculation.");
		abort ();
	}
	
	sz = (1L << SCALE) * edgefactor * 2 * sizeof (int64_t);
	printf ("SCALE: %" PRId64 "\nnvtx: %" PRId64 "\nedgefactor: %" PRId64 "\n"
          "terasize: %20.17e\n",
          SCALE, nvtx_scale, edgefactor, sz/1.0e12);
	printf ("A: %20.17e\nB: %20.17e\nC: %20.17e\nD: %20.17e\n", A, B, C, D);
	printf ("generation_time: %20.17e\n", generation_time);
	printf ("construction_time: %20.17e\n", construction_time);
	printf ("nbfs: %d\n", NBFS);
	
	memcpy(tm, bfs_time, NBFS*sizeof(tm[0]));
	statistics(stats, tm, NBFS);
	PRINT_STATS("time", 0);
	
	for (k = 0; k < NBFS; ++k)
		tm[k] = bfs_nedge[k];
	statistics(stats, tm, NBFS);
	PRINT_STATS("nedge", 0);
	
	for (k = 0; k < NBFS; ++k)
		tm[k] = bfs_nedge[k] / bfs_time[k];
	statistics(stats, tm, NBFS);
	PRINT_STATS("TEPS", 1);
}

