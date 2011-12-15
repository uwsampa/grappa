#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <malloc.h>
#include <sys/mman.h>
#ifdef __MTA__
#include <mta_rng.h>
#include <machine/mtaops.h>
#else
#include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include "globals.h"

/* Recursively divide a grid of N x N by four to a single point, (i, j).
   Choose between the four quadrants with probability a, b, c, and d.
   Create an edge between node i and j.
*/
MTA("mta inline")
void RMAT(int64_t i, double *rn, int64_t *start, int64_t *end) {
   double a, b, c, d, norm;

   int64_t bit = 1 << (SCALE - 1);         /* size of original quandrant     */

   a = A;                              /* RMAT parameters */
   b = B; c = C; d = D;                /* initial Q2, Q3, Q4 probability */
   // b = 0.1; c = 0.1; d = 0.25;

   if      (rn[i+4] <= a)         { *start = 0;   *end = 0;   }   /* Q1 */
   else if (rn[i+4] <= a + b)     { *start = 0;   *end = bit; }   /* Q2 */
   else if (rn[i+4] <= a + b + c) { *start = bit; *end = 0;   }   /* Q3 */
   else                           { *start = bit; *end = bit; }   /* Q4 */

/* Divide grid by 4 by moving bit 1 place to the right */
   for (bit >>= 1; bit > 0; bit >>= 1) {
       i += 5;

/* New probability = old probability +/- at most 10% */
       a *= 0.95 + 0.1 * rn[i  ];
       b *= 0.95 + 0.1 * rn[i+1];
       c *= 0.95 + 0.1 * rn[i+2];
       d *= 0.95 + 0.1 * rn[i+3];

       norm = 1.0 / (a + b + c + d);
       a *= norm; b *= norm; c *= norm; d *= norm;

       if      (rn[i+4] <= a)         {                           }  /* Q1 */
       else if (rn[i+4] <= a + b)     {*end   += bit;             }  /* Q2 */
       else if (rn[i+4] <= a + b + c) {*start += bit;             }  /* Q3 */
       else                           {*start += bit; *end += bit;}  /* Q4 */
}  }
 
/* Remove self- and duplicate edges. We use a hash function and linked
   list to store non-duplicate edges.
*/
int64_t Remove(int64_t NV, int64_t NE, int64_t *sV, int64_t *eV) {
  int64_t i, NGE = 0;
  int64_t *head = (int64_t *) xmalloc(NV * sizeof(int64_t));
  int64_t *next = (int64_t *) xmalloc(NE * sizeof(int64_t));

/* Initialize linked lists */
  for (i = 0; i < NV; i++) head[i] = -1;
  for (i = 0; i < NE; i++) next[i] = -2;

MTA("mta assert no dependence")
  for (i = 0; i < NE; i++) {
      int64_t k, *ptr;
      int64_t sv  = sV[i];
      int64_t ev  = eV[i];
      int64_t key = sv ^ ev;               /* hash function */
      if (key == 0) continue;          /* self-edge */

      ptr = head + key;
      k   = readfe(ptr);

/* Search this key's linked list for this edge */
      while (k != -1) {
         if ((sV[k] == sv) && (eV[k] == ev)) break;   /* duplicate edge */
         
         writeef(ptr, k);
         ptr = next + k;
         k   = readfe(ptr);
      }

/* Add a new edge to end of this list */
      if (k == -1) {k = i; next[i] = -1; NGE++;}
      writeef(ptr, k);
  }

/* Move good edges to front of sV and eV arrays */
MTA("mta assert no dependence")
  for (i = 0; i < NGE; i++) {
      while (next[i] == -2) {
         int64_t k = int_fetch_add(&NE, -1) - 1;
         sV[i] = sV[k]; eV[i] = eV[k]; next[i] = next[k];
  }   }

  free(head); free(next);

  return NGE;
}


void genScalData(graphSDG* SDGdataPtr, double a, double b, double c, double d)
{ int64_t i, j, n, skip, NE, NV;
  int64_t *sV, *eV, *weight, *permV;
  void *mem;
  size_t memsz;
  double *rn;

  memsz = (2 * numVertices) * sizeof (double) + numVertices * sizeof (int64_t);
  mem = xmmap (NULL, memsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
	       0, 0);

  rn = (double *) mem;
  permV = (int64_t *) &rn[2 * numVertices];

  A = a;
  B = b;
  C = c;
  D = d;

/*-------------------------------------------------------------------------*/
/* STEP 0: Create the permutations required to randomize the vertices      */
/*-------------------------------------------------------------------------*/
       
  NE    = numEdges;
  NV    = numVertices;
  for (i = 0; i < NV; i++) permV[i] = i;

/* Permute indices SCALE * NV times                 */
/* Perform in sets of NV permutations to save space */
  for (j = 0; j < SCALE; j++) {

      n = 2 * NV;
      prand(n, rn);
 
MTA("mta assert parallel")
      for (i = 0; i < NV; i++) {
          int64_t k, m, t, x, y;
          k = rn[2 * i]     * NV;
          m = rn[2 * i + 1] * NV;
 
          if (k == m) continue;
          if (k >  m) {t = k; k = m; m = t;}
 
          x = readfe(permV + k);
          y = readfe(permV + m);

          writeef(permV + k, y);
          writeef(permV + m, x);
  }   }

/*-------------------------------------------------------------------------*/
/* STEP 1: Create the edges using the RMAT algorithm and permute labels.   */
/*-------------------------------------------------------------------------*/

  alloc_edgelist (SDGdataPtr, numEdges);
  sV     = SDGdataPtr->startVertex;
  eV     = SDGdataPtr->endVertex;
  weight = SDGdataPtr->intWeight;

/* Create skip edges at a time to save space */
/* Log of the number of edges that can be computed with 2 * NV rn */
  skip = CHAR_BIT * sizeof (int64_t) - MTA_BIT_LEFT_ZEROS((2 * NV) / (5 * SCALE));
  skip = 1 << skip;

  for (j = 0; j < NE; j += skip) {

      n = 2 * NV;
      prand(n, rn);

OMP("omp parallel for")
MTA("mta assert no dependence")
      for (i = 0; i < skip; i++) {
          int64_t sv, ev;
          RMAT(i, rn, &sv, &ev);
          sV[i + j] = permV[sv];
          eV[i + j] = permV[ev];
  }   }

/*-------------------------------------------------------------------------*/
/* STEP 2: Assign weights                                                  */
/*-------------------------------------------------------------------------*/

  for (j = 0; j < NE; j += 2 * NV) {

      n = 2 * NV;
      prand(n, rn);

MTA("mta assert no dependence")
      for (i = 0; i < 2 * NV; i++)
	if (i + j < NE)
          weight[i + j] = (int64_t) (rn[i] * NV);
  }

/*-------------------------------------------------------------------------*/
/* STEP 3: Remove self- and duplicate edges                                */
/*-------------------------------------------------------------------------*/
  NE = Remove(NV, NE, sV, eV);
  printf("\nNumber of edges created - %9ld\n", NE);

  /* 
  int64_t* degree = (int64_t *) xcalloc(NV , sizeof(int64_t));
  int64_t* degree_hist = (int64_t *) xcalloc(NV, sizeof(int64_t));
  
  for (i=0; i<NE; i++) {
      degree[sV[i]]++;
  }
   
  for (i=0; i<NV; i++) {
      degree_hist[degree[i]]++;
  }
 
  for (i=0; i<NV; i++) {
      if (degree_hist[i] != 0) 
         printf("[%d %d] ", i, degree_hist[i]); 
  }
  free(degree); free(degree_hist);
  */

  munmap (mem, memsz);
}


void gen1DTorus(graphSDG* SDGdataPtr)
{ int64_t i, NE, NV;
  int64_t *sV, *eV, *weight;

  NE     = numVertices;
  NV     = numVertices;

  alloc_edgelist (SDGdataPtr, NE);
  sV     = SDGdataPtr->startVertex;
  eV     = SDGdataPtr->endVertex;
  weight = SDGdataPtr->intWeight;

MTA("mta assert no dependence")
  for (i = 0; i < NE - 1; i ++) {sV[i] = i; eV[i] = i + 1;}

  sV[NE - 1] = NV - 1;
  eV[NE - 1] = 0;

  for (i = 0; i < NE; i ++) weight[i] = i % 8;

  printf("Number of edges created - %9ld\n", NE);
}
