********************************************************************************
*   Cray  Compilation Report
*   Source File:     xmt-csr/xmt-csr.c
*   Program Library: xmt-csr/xmt-csr.pl
*   Module:          xmt-csr.o
********************************************************************************
          | /* -*- mode: C; mode: folding; fill-column: 70; -*- */
          | /* Copyright 2010,  Georgia Institute of Technology, USA. */
          | /* See COPYING for license. */
          | #define _FILE_OFFSET_BITS 64
          | #define _THREAD_SAFE
          | #include <stdlib.h>
          | #include <stdio.h>
          | #include <string.h>
          | #include <math.h>
          | 
          | #include <assert.h>
          | #include <stdbool.h>
          | 
          | 
          | #include "../compat.h"
          | #include "../compatio.h"
          | 
          | #include "../graph500.h"
          | #include "../xalloc.h"
          | #include "../generator/graph_generator.h"
          | #include "../options.h"
          | #include "../timer.h"
          | #include "../prng.h"
          | 
          | 
          | #define MINVECT_SIZE 2
          | #define XOFF(k) (xoff[2*(k)])
          | #define XENDOFF(k) (xoff[1+2*(k)])
          | 
          | static int64_t maxvtx, nv, sz, nadj;
          | static int64_t * restrict xoff; /* Length 2*nv+2 */
          | static int64_t * restrict xadjstore; /* Length MINVECT_SIZE + (xoff[nv] == nedge) */
          | static int64_t * restrict xadj;
          | 
          | // TODO: fix me so I'm not assigning to copies of pointers...
          | bool checkpoint_in(int SCALE, int edgefactor, struct packed_edge *restrict * IJ, int64_t * nedge, int64_t * bfs_roots, int64_t * nbfs) {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |   tic();
++ function tic inlined
          | 
          |   char fname[256];
          |   /*sprintf(fname, "/scratch/tmp/holt225/graph500.%ld.%ld.ckpt", SCALE, edgefactor);*/
          |   sprintf(fname, "ckpts/graph500.%ld.%ld.xmt.w.ckpt", SCALE, edgefactor);
          |   FILE * fin = fopen(fname, "r");
          |   if (!fin) {
          |     fprintf(stderr, "Unable to open file - %s, will generate graph and write checkpoint.\n", fname);
          |     return false;
          |   }
          | 
          |   int64_t nadj;
          | 
    1 X   |   fread(nedge, sizeof(*nedge), 1, fin);
++ function xmt_fread inlined
    1 X   |   fread(&nv, sizeof(nv), 1, fin);
++ function xmt_fread inlined
    1 X   |   fread(&nadj, sizeof(nadj), 1, fin);
++ function xmt_fread inlined
    1 X   |   fread(nbfs, sizeof(*nbfs), 1, fin);
++ function xmt_fread inlined
          | 
          |   (*IJ) = xmalloc(*nedge * sizeof(**IJ));
++ function xmalloc inlined
          |   xoff = xmalloc((2*nv+2) * sizeof(*xoff));
++ function xmalloc inlined
          |   xadjstore = xmalloc(nadj * sizeof(*xadjstore));
++ function xmalloc inlined
          |   xadj = xadjstore + 2;
          | 
   13 X   |   fread(*IJ, sizeof(packed_edge), *nedge, fin);
++ function xmt_fread inlined
   17 X   |   fread(xoff, sizeof(*xoff), 2*nv+2, fin);
++ function xmt_fread inlined
   21 X   |   fread(xadjstore, sizeof(*xadjstore), nadj, fin);
++ function xmt_fread inlined
          | 
          |   double t = toc();
++ function toc inlined
          |   fprintf(stderr, "checkpoint_read_time: %g\n", t);
          | 
          |   return true;
          | }
          | 
          | /*#define BUFSIZE (1L<<10)*/
          | /*static char buf[BUFSIZE];*/
          | /*size_t kbuf;*/
          | 
          | /*inline void buf_init() { kbuf = 0; }*/
          | /*void buf_fwrite(void * data, size_t size, size_t nmemb, FILE * f) {*/
          |   /*size_t p = 0;*/
          |   /*while (p < size*nmemb) {*/
          |     /*size_t n = min(size*nmemb, BUFSIZE-kbuf)*/
          |     
          |   /*}*/
          | /*}*/
          | 
          | void checkpoint_out(int64_t SCALE, int64_t edgefactor, const struct packed_edge * restrict edges, const int64_t nedge, const int64_t * restrict bfs_roots, const int64_t nbfs) {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |   fprintf(stderr, "starting checkpoint...\n");
          |   tic();
++ function tic inlined
          |   char fname[256];
          |   sprintf(fname, "ckpts/graph500.%lld.%lld.xmt.w.ckpt", SCALE, edgefactor);
          |   FILE * fout = fopen(fname, "w");
          |   if (!fout) {
          |     fprintf(stderr,"Unable to open file for writing: %s\n", fname);
          |     exit(1);
          |   }
          |   
          |   /*fprintf(stderr, "nedge=%lld, nv=%lld, nadj=%lld, nbfs=%lld\n", nedge, nv, nadj, nbfs);*/
          |   
   25 XP  |   fwrite(&nedge, sizeof(nedge), 1, fout);
++ function xmt_fwrite inlined
   29 XP  |   fwrite(&nv, sizeof(nv), 1, fout);
++ function xmt_fwrite inlined
   33 XP  |   fwrite(&nadj, sizeof(nadj), 1, fout);
++ function xmt_fwrite inlined
   37 XP  |   fwrite(&nbfs, sizeof(nbfs), 1, fout);
++ function xmt_fwrite inlined
          |   
          |   // write out edge tuples
   41 XP  |   fwrite(edges, sizeof(*edges), nedge, fout);
++ function xmt_fwrite inlined
          | 
          |   // xoff
   45 XP  |   fwrite(xoff, sizeof(*xoff), 2*nv+2, fout);
++ function xmt_fwrite inlined
          |   
          |   // xadj
   49 XP  |   fwrite(xadjstore, sizeof(*xadjstore), nadj, fout);
++ function xmt_fwrite inlined
          |   
          |   // bfs_roots
   53 XP  |   fwrite(bfs_roots, sizeof(*bfs_roots), nbfs, fout);
++ function xmt_fwrite inlined
          |   
          |   // int weights (for suite)
          |   int64_t actual_nadj = 0;
   56 P:$ |   for (int64_t j=0; j<nv; j++) { actual_nadj += XENDOFF(j)-XOFF(j); }
** reduction moved out of 1 loop
   60 XP  |   fwrite(&actual_nadj, sizeof(actual_nadj), 1, fout);
++ function xmt_fwrite inlined
          | 
          |   int64_t bufsize = (1L<<12);
          |   double * rn = xmalloc(bufsize*sizeof(double));
++ function xmalloc inlined
          |   int64_t * rni = xmalloc(bufsize*sizeof(int64_t));
++ function xmalloc inlined
          |   for (int64_t j=0; j<actual_nadj; j+=bufsize) {
   61 S   |     prand(bufsize, rn);
** function with unknown side effects: prand
          |     for (int64_t i=0; i<bufsize; i++) {
   64 SP  |       rni[i] = (int64_t)(rn[i]*nv);
          |     }
   61 -   |     int64_t n = min(bufsize, actual_nadj-j);
   65 SXP |     fwrite(rni, sizeof(int64_t), n, fout);
++ function xmt_fwrite inlined
          |   }
          |   free(rn); free(rni);
          | 
          |   fclose(fout);
          |   double t = toc();
++ function toc inlined
          |   fprintf(stderr, "checkpoint_write_time: %g\n", t);
          | }
          | 
          | static void
          | find_nv (const struct packed_edge * restrict IJ, const int64_t nedge)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         int64_t k, tmaxvtx = -1;
          |         
          |         maxvtx = -1;
          |         for (k = 0; k < nedge; ++k) {
   69 -   |                 if (get_v0_from_edge(&IJ[k]) > tmaxvtx)
++ function get_v0_from_edge inlined
   69 S   |                         tmaxvtx = get_v0_from_edge(&IJ[k]);
++ function get_v0_from_edge inlined
   69 -   |                 if (get_v1_from_edge(&IJ[k]) > tmaxvtx)
++ function get_v1_from_edge inlined
   69 S   |                         tmaxvtx = get_v1_from_edge(&IJ[k]);
++ function get_v1_from_edge inlined
          |         }
          |         k = readfe (&maxvtx);
          |         if (tmaxvtx > k)
          |                 k = tmaxvtx;
          |         writeef (&maxvtx, k);
          |         nv = 1+maxvtx;
          | }
          | 
          | static int
          | alloc_graph (int64_t nedge)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         sz = (2*nv+2) * sizeof (*xoff);
          |         xoff = xmalloc_large (sz);
          |         if (!xoff) return -1;
          |         return 0;
          | }
          | 
          | static void
          | free_graph (void)
          | {
          |         xfree_large (xadjstore);
          |         xfree_large (xoff);
          | }
          | 
          | static int
          | setup_deg_off (const struct packed_edge * restrict IJ, int64_t nedge)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         int64_t k, accum;
          |         for (k = 0; k < 2*nv+2; ++k)
   72 P   |                 xoff[k] = 0;
          |         MTA("mta assert nodep") MTA("mta use 100 streams")
          |         for (k = 0; k < nedge; ++k) {
   74 P   |                 int64_t i = get_v0_from_edge(&IJ[k]);
++ function get_v0_from_edge inlined
   74 P   |                 int64_t j = get_v1_from_edge(&IJ[k]);
++ function get_v1_from_edge inlined
   74 P   |                 if (i != j) { /* Skip self-edges. */
   74 P   |                         int_fetch_add (&XOFF(i), 1);
   74 P   |                         int_fetch_add (&XOFF(j), 1);
          |                 }
          |         }
          |         accum = 0;
          |         MTA("mta use 100 streams")
          |         for (k = 0; k < nv; ++k) {
   76 P   |                 int64_t tmp = XOFF(k);
          |                 if (tmp < MINVECT_SIZE) tmp = MINVECT_SIZE;
   78 P   |                 XOFF(k) = accum;
   78 L   |                 accum += tmp;
          |         }
          |         XOFF(nv) = accum;
          |   nadj = accum+MINVECT_SIZE;
          |         MTA("mta use 100 streams")
          |         for (k = 0; k < nv; ++k)
   80 P   |                 XENDOFF(k) = XOFF(k);
          |         if (!(xadjstore = malloc ((accum + MINVECT_SIZE) * sizeof (*xadjstore))))
          |                 return -1;
          |         xadj = &xadjstore[MINVECT_SIZE]; /* Cheat and permit xadj[-1] to work. */
          |         for (k = 0; k < accum + MINVECT_SIZE; ++k)
   82 P   |                 xadjstore[k] = -1;
          |         return 0;
          | }
          | 
          | static void
          | scatter_edge (const int64_t i, const int64_t j)
          | {
          |         int64_t where;
          |         where = int_fetch_add (&XENDOFF(i), 1);
          |         xadj[where] = j;
          | }
          | 
          | static int
          | i64cmp (const void *a, const void *b)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         const int64_t ia = *(const int64_t*)a;
          |         const int64_t ib = *(const int64_t*)b;
          |         if (ia < ib) return -1;
          |         if (ia > ib) return 1;
          |         return 0;
          | }
          | 
          | MTA("mta no inline")
          | static void
          | pack_edges (void)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         int64_t v;
          |         
          |         MTA("mta assert parallel") MTA("mta use 100 streams")
          |   for (v = 0; v < nv; ++v) {
          |                 int64_t kcur, k;
   84 p   |                 if (XOFF(v)+1 < XENDOFF(v)) {
   84 D   |                         qsort (&xadj[XOFF(v)], XENDOFF(v)-XOFF(v), sizeof(*xadj), i64cmp);
   84 p   |                         kcur = XOFF(v);
          |                         MTA("mta loop serial")
   84 p   |                         for (k = XOFF(v)+1; k < XENDOFF(v); ++k)
   85 p-  |                                 if (xadj[k] != xadj[kcur])
   85 DS  |                                         xadj[1 + int_fetch_add (&kcur, 1)] = xadj[k];
   84 p   |                         ++kcur;
          |                         for (k = kcur; k < XENDOFF(v); ++k)
   86 D-  |                                 xadj[k] = -1;
   84 p   |                         XENDOFF(v) = kcur;
          |                 }
          |   }
          | }
          | 
          | static void
          | gather_edges (const struct packed_edge * restrict IJ, int64_t nedge)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         int64_t k;
          |         
          |         MTA("mta assert nodep")
          |         MTA("mta use 100 streams")
          |         for (k = 0; k < nedge; ++k) {
   89 P   |                 int64_t i = get_v0_from_edge(&IJ[k]);
++ function get_v0_from_edge inlined
   89 P   |                 int64_t j = get_v1_from_edge(&IJ[k]);
++ function get_v1_from_edge inlined
   89 P   |                 if (i != j) {
   89 P   |                         scatter_edge (i, j);
++ function scatter_edge inlined
   89 P   |                         scatter_edge (j, i);
++ function scatter_edge inlined
          |                 }
          |         }
          |         
          |         pack_edges ();
          | }
          | 
          | int 
          | create_graph_from_edgelist (struct packed_edge *IJ, int64_t nedge)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         find_nv (IJ, nedge);
          |         if (alloc_graph (nedge)) return -1;
          |         if (setup_deg_off (IJ, nedge)) {
          |                 xfree_large (xoff);
          |                 return -1;
          |         }
          |         gather_edges (IJ, nedge);
          |         return 0;
          | }
          | 
          | int
          | make_bfs_tree (int64_t *bfs_tree_out, int64_t *max_vtx_out,
          |                            int64_t srcvtx)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         int64_t * restrict bfs_tree = bfs_tree_out;
          |         int err = 0;
          |         
          |         int64_t * restrict vlist = NULL;
          |         int64_t k1, k2;
          |         
          |         *max_vtx_out = maxvtx;
          |         
          |         vlist = xmalloc_large (nv * sizeof (*vlist));
          |         if (!vlist) return -1;
          |         
          |         for (k1 = 0; k1 < nv; ++k1)
   92 P   |                 bfs_tree[k1] = -1;
          |         
          |         vlist[0] = srcvtx;
          |         bfs_tree[srcvtx] = srcvtx;
          |         k1 = 0; k2 = 1;
   94 X   |         while (k1 != k2) {
   94 X   |                 const int64_t oldk2 = k2;
          |                 int64_t k;
          |                 MTA("mta assert nodep") MTA("mta use 100 streams") MTA("mta interleave schedule")
   94 X   |                 for (k = k1; k < oldk2; ++k) {
   95 XP  |                         const int64_t v = vlist[k];
   95 XP  |                         const int64_t veo = XENDOFF(v);
          |                         int64_t vo;
          |                         MTA("mta loop serial")
   95 XP  |                         for (vo = XOFF(v); vo < veo; ++vo) {
   96 XP- |                                 const int64_t j = xadj[vo];
   96 XPS |                                 if (bfs_tree[j] == -1) {
   96 XPS |                                         int64_t t = readfe (&bfs_tree[j]);
   96 XPS |                                         if (t == -1) {
   96 XP- |                                                 t = v;
   96 XPS |                                                 vlist[int_fetch_add (&k2, 1)] = j;
          |                                         }
   96 XPS |                                         writeef (&bfs_tree[j], t);
          |                                 }
          |                         }
          |                 }
   94 X   |                 k1 = oldk2;
          |         }
          |         
          |         xfree_large (vlist);
          |         
          |         return err;
          | }
          | 
          | void
          | destroy_graph (void)
          | {
** multiprocessor parallelization enabled (-par)
** expected to be called in a serial context
** fused mul-add allowed
** debug level: off
          |         free_graph ();
++ function free_graph inlined
          | }
          | 


********************************************************************************
*   Additional Loop Details
********************************************************************************

Loop   1 in checkpoint_in at line 49

Loop   2 in checkpoint_in
       Completely unrolled
       Single iteration loop removed
       Compiler generated

Loop   3 in checkpoint_in
       Completely unrolled
       Single iteration loop removed
       Compiler generated

Loop   4 in checkpoint_in
       Completely unrolled
       Single iteration loop removed
       Compiler generated

Loop   5 in checkpoint_in
       Completely unrolled
       Single iteration loop removed
       Compiler generated

Loop   6 in checkpoint_in at line 49
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Loop   7 in checkpoint_in at line 50
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Loop   8 in checkpoint_in at line 51
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Loop   9 in checkpoint_in at line 52
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Loop  10 in checkpoint_in at line 59
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Parallel region  11 in checkpoint_in
       Multiple processor implementation
       Requesting at least 45 streams

Loop  12 in checkpoint_in in region 11
       In parallel phase 1
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  13 in checkpoint_in at line 59 in loop 12
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  14 in checkpoint_in at line 60
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Parallel region  15 in checkpoint_in
       Multiple processor implementation
       Requesting at least 45 streams

Loop  16 in checkpoint_in in region 15
       In parallel phase 2
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  17 in checkpoint_in at line 60 in loop 16
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  18 in checkpoint_in at line 61
       Loop summary: 0 loads, 0 stores, 0 floating point operations
		1 instructions, needs 30 streams for full utilization
		pipelined

Parallel region  19 in checkpoint_in
       Multiple processor implementation
       Requesting at least 45 streams

Loop  20 in checkpoint_in in region 19
       In parallel phase 3
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  21 in checkpoint_in at line 61 in loop 20
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  22 in checkpoint_out at line 95

Parallel region  23 in checkpoint_out in loop 22
       Multiple processor implementation
       Requesting at least 45 streams

Loop  24 in checkpoint_out in region 23
       In parallel phase 1
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  25 in checkpoint_out at line 95 in loop 24
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  26 in checkpoint_out at line 96

Parallel region  27 in checkpoint_out in loop 26
       Multiple processor implementation
       Requesting at least 45 streams

Loop  28 in checkpoint_out in region 27
       In parallel phase 2
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  29 in checkpoint_out at line 96 in loop 28
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  30 in checkpoint_out at line 97

Parallel region  31 in checkpoint_out in loop 30
       Multiple processor implementation
       Requesting at least 45 streams

Loop  32 in checkpoint_out in region 31
       In parallel phase 3
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  33 in checkpoint_out at line 97 in loop 32
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  34 in checkpoint_out at line 98

Parallel region  35 in checkpoint_out in loop 34
       Multiple processor implementation
       Requesting at least 45 streams

Loop  36 in checkpoint_out in region 35
       In parallel phase 4
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  37 in checkpoint_out at line 98 in loop 36
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  38 in checkpoint_out at line 101

Parallel region  39 in checkpoint_out in loop 38
       Multiple processor implementation
       Requesting at least 45 streams

Loop  40 in checkpoint_out in region 39
       In parallel phase 5
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  41 in checkpoint_out at line 101 in loop 40
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  42 in checkpoint_out at line 104

Parallel region  43 in checkpoint_out in loop 42
       Multiple processor implementation
       Requesting at least 45 streams

Loop  44 in checkpoint_out in region 43
       In parallel phase 6
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  45 in checkpoint_out at line 104 in loop 44
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  46 in checkpoint_out at line 107

Parallel region  47 in checkpoint_out in loop 46
       Multiple processor implementation
       Requesting at least 45 streams

Loop  48 in checkpoint_out in region 47
       In parallel phase 7
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  49 in checkpoint_out at line 107 in loop 48
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  50 in checkpoint_out at line 110

Parallel region  51 in checkpoint_out in loop 50
       Multiple processor implementation
       Requesting at least 45 streams

Loop  52 in checkpoint_out in region 51
       In parallel phase 8
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  53 in checkpoint_out at line 110 in loop 52
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Parallel region  54 in checkpoint_out
       Multiple processor implementation
       Requesting at least 45 streams

Loop  55 in checkpoint_out in region 54
       In parallel phase 9
       Dynamically scheduled, variable chunks, min size = 13
       Compiler generated

Loop  56 in checkpoint_out at line 114 in loop 55
       Loop summary: 2 loads, 0 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  57 in checkpoint_out at line 115

Parallel region  58 in checkpoint_out in loop 57
       Multiple processor implementation
       Requesting at least 45 streams

Loop  59 in checkpoint_out in region 58
       In parallel phase 10
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  60 in checkpoint_out at line 115 in loop 59
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  61 in checkpoint_out at line 121

Parallel region  62 in checkpoint_out in loop 61
       Multiple processor implementation
       Requesting at least 45 streams

Loop  63 in checkpoint_out in region 62
       In parallel phase 11
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  64 in checkpoint_out at line 123 in loop 63
       Expecting 4096 iterations
       Loop summary: 1 loads, 1 stores, 1 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  65 in checkpoint_out at line 126 in loop 61

Parallel region  66 in checkpoint_out in loop 65
       Multiple processor implementation
       Requesting at least 45 streams

Loop  67 in checkpoint_out in region 66
       In parallel phase 12
       Dynamically scheduled, variable chunks, min size = 11
       Compiler generated

Loop  68 in checkpoint_out at line 126 in loop 67
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  69 in find_nv at line 142
       Loop not pipelined: not structurally ok
       Scheduled to minimize serial time
       Loop summary: 8 instructions, 0 floating point operations
		2 loads, 0 stores, 0 reloads, 0 spills, 2 branches, 0 calls

Parallel region  70 in setup_deg_off
       Multiple processor implementation
       Requesting at least 100 streams

Loop  71 in setup_deg_off in region 70
       In parallel phase 1
       Dynamically scheduled, variable chunks, min size = 64
       Compiler generated

Loop  72 in setup_deg_off at line 175 in loop 71
       Loop summary: 0 loads, 1 stores, 0 floating point operations
		1 instructions, needs 50 streams for full utilization
		pipelined

Loop  73 in setup_deg_off in region 70
       In parallel phase 2
       Dynamically scheduled, variable chunks, min size = 20
       Compiler generated

Loop  74 in setup_deg_off at line 178 in loop 73
       Loop not pipelined: not structurally ok
       Loop summary: 8 instructions, 0 floating point operations
		2 loads, 2 stores, 0 reloads, 0 spills, 2 branches, 0 calls

Loop  75 in setup_deg_off in region 70
       In parallel phase 3
       Recurrence control loop, chunk size = 1024
       Compiler generated

Loop  76 in setup_deg_off at line 188 in loop 75
       Stage 1 of recurrence
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  77 in setup_deg_off in loop 75
       Stage 1 recurrence communication
       Loop not pipelined: not structurally ok
       Compiler generated
       Loop summary: 13 instructions, 0 floating point operations
		1 loads, 1 stores, 1 reloads, 0 spills, 3 branches, 0 calls

Loop  78 in setup_deg_off at line 190 in loop 75
       Stage 2 of recurrence
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  79 in setup_deg_off in region 70
       In parallel phase 6
       Dynamically scheduled, variable chunks, min size = 13
       Compiler generated

Loop  80 in setup_deg_off at line 197 in loop 79
       Loop summary: 1 loads, 1 stores, 0 floating point operations
		2 instructions, needs 45 streams for full utilization
		pipelined

Loop  81 in setup_deg_off in region 70
       In parallel phase 6
       Dynamically scheduled, variable chunks, min size = 64
       Compiler generated

Loop  82 in setup_deg_off at line 202 in loop 81
       Loop summary: 0 loads, 1 stores, 0 floating point operations
		1 instructions, needs 50 streams for full utilization
		pipelined

Parallel region  83 in pack_edges
       Multiple processor implementation
       Requesting at least 100 streams

Loop  84 in pack_edges at line 233 in region 83
       In parallel phase 1
       Dynamically scheduled

Loop  85 in pack_edges at line 238 in loop 84
       Loop not pipelined: not structurally ok
       Loop summary: 9 instructions, 0 floating point operations
		3 loads, 2 stores, 0 reloads, 0 spills, 2 branches, 0 calls

Loop  86 in pack_edges at line 242 in loop 84
       Loop summary: 0 loads, 1 stores, 0 floating point operations
		1 instructions, needs 50 streams for full utilization
		pipelined

Parallel region  87 in gather_edges
       Multiple processor implementation
       Requesting at least 100 streams

Loop  88 in gather_edges in region 87
       In parallel phase 1
       Dynamically scheduled, variable chunks, min size = 15
       Compiler generated

Loop  89 in gather_edges at line 256 in loop 88
       Loop not pipelined: not structurally ok
       Loop summary: 10 instructions, 0 floating point operations
		2 loads, 4 stores, 0 reloads, 0 spills, 2 branches, 0 calls

Parallel region  90 in make_bfs_tree
       Multiple processor implementation
       Requesting at least 50 streams

Loop  91 in make_bfs_tree in region 90
       In parallel phase 1
       Dynamically scheduled, variable chunks, min size = 64
       Compiler generated

Loop  92 in make_bfs_tree at line 296 in loop 91
       Loop summary: 0 loads, 1 stores, 0 floating point operations
		1 instructions, needs 50 streams for full utilization
		pipelined

Parallel region  93 in make_bfs_tree
       Multiple processor implementation
       Requesting at least 100 streams

Loop  94 in make_bfs_tree at line 302 in region 93

Loop  95 in make_bfs_tree at line 306 in loop 94
       In parallel phase 4
       Interleave scheduled

Loop  96 in make_bfs_tree at line 311 in loop 95
       Loop not pipelined: not structurally ok
       Loop summary: 14 instructions, 0 floating point operations
		4 loads, 3 stores, 0 reloads, 0 spills, 3 branches, 0 calls
