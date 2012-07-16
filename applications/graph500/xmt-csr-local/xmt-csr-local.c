/* -*- mode: C; mode: folding; fill-column: 70; -*- */
/* Copyright 2010,  Georgia Institute of Technology, USA. */
/* See COPYING for license. */
#define _FILE_OFFSET_BITS 64
#define _THREAD_SAFE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <assert.h>
#include <stdbool.h>


#include "../compat.h"
#include "../compatio.h"

#include "../graph500.h"
#include "../xalloc.h"
#include "../generator/graph_generator.h"
#include "../options.h"
#include "../timer.h"
#include "../prng.h"

#define MINVECT_SIZE 2
#define XOFF(k) (xoff[2*(k)])
#define XENDOFF(k) (xoff[1+2*(k)])

static int64_t maxvtx, nv, sz, nadj;
static int64_t * restrict xoff; /* Length 2*nv+2 */
static int64_t * restrict xadjstore; /* Length MINVECT_SIZE + (xoff[nv] == nedge) */
static int64_t * restrict xadj;

bool checkpoint_in(int SCALE, int edgefactor, struct packed_edge *restrict * IJ, int64_t * nedge, int64_t * bfs_roots, int64_t * nbfs) {
#ifdef __MTA__
  snap_init();
#endif
  tic();

  char fname[256];
  /*sprintf(fname, "/scratch/tmp/holt225/graph500.%ld.%ld.ckpt", SCALE, edgefactor);*/
  sprintf(fname, "ckpts/graph500.%ld.%ld.xmt.w.ckpt", SCALE, edgefactor);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    fprintf(stderr, "Unable to open file - %s, will generate graph and write checkpoint.\n", fname);
    return false;
  }

  int64_t nadj;

  fread(nedge, sizeof(*nedge), 1, fin);
  fread(&nv, sizeof(nv), 1, fin);
  fread(&nadj, sizeof(nadj), 1, fin);
  fread(nbfs, sizeof(*nbfs), 1, fin);

  (*IJ) = xmalloc(*nedge * sizeof(**IJ));
  xoff = xmalloc((2*nv+2) * sizeof(*xoff));
  xadjstore = xmalloc(nadj * sizeof(*xadjstore));
  xadj = xadjstore + 2;
  
  maxvtx = nv-1;

  fread_plus(*IJ, sizeof(packed_edge), *nedge, fin, "edges", SCALE, edgefactor);
  fread_plus(xoff, sizeof(*xoff), 2*nv+2, fin, "xoff", SCALE, edgefactor);
  fread_plus(xadjstore, sizeof(*xadjstore), nadj, fin, "xadj", SCALE, edgefactor);

  double t = toc();
  printf("checkpoint_read_time: %g\n", t);

  return true;
}

/*#define BUFSIZE (1L<<10)*/
/*static char buf[BUFSIZE];*/
/*size_t kbuf;*/

/*inline void buf_init() { kbuf = 0; }*/
/*void buf_fwrite(void * data, size_t size, size_t nmemb, FILE * f) {*/
  /*size_t p = 0;*/
  /*while (p < size*nmemb) {*/
    /*size_t n = min(size*nmemb, BUFSIZE-kbuf)*/
    
  /*}*/
/*}*/

void checkpoint_out(int64_t SCALE, int64_t edgefactor, const struct packed_edge * restrict edges, const int64_t nedge, const int64_t * restrict bfs_roots, const int64_t nbfs) {
  fprintf(stderr, "starting checkpoint...\n");
  tic();
  char fname[256];
  sprintf(fname, "ckpts/graph500.%lld.%lld.xmt.w.ckpt", SCALE, edgefactor);
  FILE * fout = fopen(fname, "w");
  if (!fout) {
    fprintf(stderr,"Unable to open file for writing: %s\n", fname);
    exit(1);
  }
  
  /*fprintf(stderr, "nedge=%lld, nv=%lld, nadj=%lld, nbfs=%lld\n", nedge, nv, nadj, nbfs);*/
  
  fwrite(&nedge, sizeof(nedge), 1, fout);
  fwrite(&nv, sizeof(nv), 1, fout);
  fwrite(&nadj, sizeof(nadj), 1, fout);
  fwrite(&nbfs, sizeof(nbfs), 1, fout);
  
  // write out edge tuples
  fwrite(edges, sizeof(*edges), nedge, fout);

  // xoff
  fwrite(xoff, sizeof(*xoff), 2*nv+2, fout);
  
  // xadj
  fwrite(xadjstore, sizeof(*xadjstore), nadj, fout);
  
  // bfs_roots
  fwrite(bfs_roots, sizeof(*bfs_roots), nbfs, fout);
  
  // int weights (for suite)
  int64_t actual_nadj = 0;
  for (int64_t j=0; j<nv; j++) { actual_nadj += XENDOFF(j)-XOFF(j); }
  fwrite(&actual_nadj, sizeof(actual_nadj), 1, fout);

  int64_t bufsize = (1L<<12);
  double * rn = xmalloc(bufsize*sizeof(double));
  int64_t * rni = xmalloc(bufsize*sizeof(int64_t));
  for (int64_t j=0; j<actual_nadj; j+=bufsize) {
    prand(bufsize, rn);
    for (int64_t i=0; i<bufsize; i++) {
      rni[i] = (int64_t)(rn[i]*nv);
    }
    int64_t n = min(bufsize, actual_nadj-j);
    fwrite(rni, sizeof(int64_t), n, fout);
  }
  free(rn); free(rni);

  fclose(fout);
  double t = toc();
  fprintf(stderr, "checkpoint_write_time: %g\n", t);
}

static void
find_nv (const struct packed_edge * restrict IJ, const int64_t nedge)
{
	int64_t k, tmaxvtx = -1;
	
	maxvtx = -1;
	for (k = 0; k < nedge; ++k) {
		if (get_v0_from_edge(&IJ[k]) > tmaxvtx)
			tmaxvtx = get_v0_from_edge(&IJ[k]);
		if (get_v1_from_edge(&IJ[k]) > tmaxvtx)
			tmaxvtx = get_v1_from_edge(&IJ[k]);
	}
	k = readfe (&maxvtx);
	if (tmaxvtx > k)
		k = tmaxvtx;
	writeef (&maxvtx, k);
	nv = 1+maxvtx;
}

static int
alloc_graph (int64_t nedge)
{
	sz = (2*nv+2) * sizeof (*xoff);
	xoff = xmalloc_large (sz);
	if (!xoff) return -1;
	return 0;
}

static void
free_graph (void)
{
	xfree_large (xadjstore);
	xfree_large (xoff);
}

static int
setup_deg_off (const struct packed_edge * restrict IJ, int64_t nedge)
{
	int64_t k, accum;
	for (k = 0; k < 2*nv+2; ++k)
		xoff[k] = 0;
	MTA("mta assert nodep") MTA("mta use 100 streams")
	for (k = 0; k < nedge; ++k) {
		int64_t i = get_v0_from_edge(&IJ[k]);
		int64_t j = get_v1_from_edge(&IJ[k]);
		if (i != j) { /* Skip self-edges. */
			int_fetch_add (&XOFF(i), 1);
			int_fetch_add (&XOFF(j), 1);
		}
	}
	accum = 0;
	MTA("mta use 100 streams")
	for (k = 0; k < nv; ++k) {
		int64_t tmp = XOFF(k);
		if (tmp < MINVECT_SIZE) tmp = MINVECT_SIZE;
		XOFF(k) = accum;
		accum += tmp;
	}
	XOFF(nv) = accum;
  nadj = accum+MINVECT_SIZE;
	MTA("mta use 100 streams")
	for (k = 0; k < nv; ++k)
		XENDOFF(k) = XOFF(k);
	if (!(xadjstore = malloc ((accum + MINVECT_SIZE) * sizeof (*xadjstore))))
		return -1;
	xadj = &xadjstore[MINVECT_SIZE]; /* Cheat and permit xadj[-1] to work. */
	for (k = 0; k < accum + MINVECT_SIZE; ++k)
		xadjstore[k] = -1;
	return 0;
}

static void
scatter_edge (const int64_t i, const int64_t j)
{
	int64_t where;
	where = int_fetch_add (&XENDOFF(i), 1);
	xadj[where] = j;
}

static int
i64cmp (const void *a, const void *b)
{
	const int64_t ia = *(const int64_t*)a;
	const int64_t ib = *(const int64_t*)b;
	if (ia < ib) return -1;
	if (ia > ib) return 1;
	return 0;
}

MTA("mta no inline")
static void
pack_edges (void)
{
	int64_t v;
	
	MTA("mta assert parallel") MTA("mta use 100 streams")
  for (v = 0; v < nv; ++v) {
		int64_t kcur, k;
		if (XOFF(v)+1 < XENDOFF(v)) {
			qsort (&xadj[XOFF(v)], XENDOFF(v)-XOFF(v), sizeof(*xadj), i64cmp);
			kcur = XOFF(v);
			MTA("mta loop serial")
			for (k = XOFF(v)+1; k < XENDOFF(v); ++k)
				if (xadj[k] != xadj[kcur])
					xadj[1 + int_fetch_add (&kcur, 1)] = xadj[k];
			++kcur;
			for (k = kcur; k < XENDOFF(v); ++k)
				xadj[k] = -1;
			XENDOFF(v) = kcur;
		}
  }
}

static void
gather_edges (const struct packed_edge * restrict IJ, int64_t nedge)
{
	int64_t k;
	
	MTA("mta assert nodep")
	MTA("mta use 100 streams")
	for (k = 0; k < nedge; ++k) {
		int64_t i = get_v0_from_edge(&IJ[k]);
		int64_t j = get_v1_from_edge(&IJ[k]);
		if (i != j) {
			scatter_edge (i, j);
			scatter_edge (j, i);
		}
	}
	
	pack_edges ();
}

int 
create_graph_from_edgelist (struct packed_edge *IJ, int64_t nedge)
{
	find_nv (IJ, nedge);
	if (alloc_graph (nedge)) return -1;
	if (setup_deg_off (IJ, nedge)) {
		xfree_large (xoff);
		return -1;
	}
	gather_edges (IJ, nedge);
	return 0;
}

int
make_bfs_tree (int64_t *bfs_tree_out, int64_t *max_vtx_out,
			   int64_t srcvtx)
{
	int64_t * restrict bfs_tree = bfs_tree_out;
	int err = 0;
	
	int64_t * restrict vlist = NULL;
	int64_t k1, k2;
	
	*max_vtx_out = maxvtx;
	
	vlist = xmalloc_large (nv * sizeof (*vlist));
	if (!vlist) return -1;
	
	for (k1 = 0; k1 < nv; ++k1)
		bfs_tree[k1] = -1;
	
	vlist[0] = srcvtx;
	bfs_tree[srcvtx] = srcvtx;
	k1 = 0; k2 = 1;
	while (k1 != k2) {
		const int64_t oldk2 = k2;
		int64_t k;
		MTA("mta assert nodep") MTA("mta use 100 streams") MTA("mta interleave schedule")
		for (k = k1; k < oldk2; ++k) {
			const int64_t v = vlist[k];
			const int64_t veo = XENDOFF(v);
#define NPACK 64
			int kpack = 0;
			int64_t vpack[NPACK];
			int64_t vo;
			MTA("mta loop serial")
			for (vo = XOFF(v); vo < veo; ++vo) {
				const int64_t j = xadj[vo];
				if (bfs_tree[j] == -1) {
					int64_t t = readfe (&bfs_tree[j]);
					if (t == -1) {
						t = v;
						vpack[kpack++] = j;
						if (kpack == NPACK) {
							int64_t dstk = int_fetch_add (&k2, NPACK), k;
							for (k = 0; k < NPACK; ++k)
								vlist[dstk + k] = vpack[k];
							kpack = 0;
						}
					}
					writeef (&bfs_tree[j], t);
				}
			}
			if (kpack) {
				int64_t dstk = int_fetch_add (&k2, kpack);
				int k;
				for (k = 0; k < kpack; ++k)
					vlist[dstk + k] = vpack[k];
			}
		}
		k1 = oldk2;
	}
	
	xfree_large (vlist);
	
	return err;
}

void
destroy_graph (void)
{
	free_graph ();
}

