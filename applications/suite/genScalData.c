// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#ifdef __MTA__
#	include <mta_rng.h>
#	include <machine/mtaops.h>
#	define CHAR_BIT 8
#else
#	include "compat/xmt-ops.h"
#endif

#include <stdlib.h>
#include <sys/mman.h>

#include "defs.h"

static graphint Remove(graphint NV, graphint NE, graphint *sV, graphint *eV);
static void RMAT(graphint i, double *rn, graphint *start, graphint *end);

void genScalData(graphedges* SDGdataPtr, double a, double b, double c, double d) {
	graphint i, j, n, skip, NE, NV;
	graphint *sV, *eV, *weight, *permV;
	void *mem;
	size_t memsz;
	double *rn;
	
	memsz = (2 * numVertices) * sizeof(double) + numVertices * sizeof(graphint);
	mem = xmmap(NULL, memsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
				 0, 0);
	
	rn = (double *) mem;
	permV = (graphint *) &rn[2 * numVertices];
	
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
			graphint k, m, t, x, y;
			k = rn[2 * i]     * NV;
			m = rn[2 * i + 1] * NV;
			
			if (k == m) continue;
			if (k >  m) {t = k; k = m; m = t;}
			
			x = readfe(permV + k);
			y = readfe(permV + m);
			
			writeef(permV + k, y);
			writeef(permV + m, x);
		}
	}
	
	/*-------------------------------------------------------------------------*/
	/* STEP 1: Create the edges using the RMAT algorithm and permute labels.   */
	/*-------------------------------------------------------------------------*/
	
	alloc_edgelist(SDGdataPtr, numEdges);
	sV     = SDGdataPtr->startVertex;
	eV     = SDGdataPtr->endVertex;
	weight = SDGdataPtr->intWeight;
	
	/* Create skip edges at a time to save space */
	/* Log of the number of edges that can be computed with 2 * NV rn */
	skip = (graphint)(CHAR_BIT * sizeof(graphint) - MTA_BIT_LEFT_ZEROS((2 * NV) / (5 * SCALE)));
	skip = 1 << skip;
	
	for (j = 0; j < NE; j += skip) {
		
		n = 2 * NV;
		prand(n, rn);
		
		MTA("mta assert no dependence")
		for (i = 0; i < skip; i++) {
			graphint sv, ev;
			RMAT(i, rn, &sv, &ev);
			sV[i + j] = permV[sv];
			eV[i + j] = permV[ev];
		}
	}	
	
	/*-------------------------------------------------------------------------*/
	/* STEP 2: Assign weights                                                  */
	/*-------------------------------------------------------------------------*/
	
	for (j = 0; j < NE; j += 2 * NV) {
		
		n = 2 * NV;
		prand(n, rn);
		
		MTA("mta assert no dependence")
		for (i = 0; i < 2 * NV; i++)
			if (i + j < NE)
				weight[i + j] = (int) (rn[i] * NV);
	}
	
	/*-------------------------------------------------------------------------*/
	/* STEP 3: Remove self- and duplicate edges                                */
	/*-------------------------------------------------------------------------*/
	NE = Remove(NV, NE, sV, eV);
	printf("\nNumber of edges created - %9lld\n", NE);
	
	/* 
	 int* degree = (int *) xcalloc(NV , sizeof(int));
	 int* degree_hist = (int *) xcalloc(NV, sizeof(int));
	 
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
	
	munmap(mem, memsz);
}

/* Recursively divide a grid of N x N by four to a single point, (i, j).
 Choose between the four quadrants with probability a, b, c, and d.
 Create an edge between node i and j.
 */
MTA("mta inline")
void RMAT(graphint i, double *rn, graphint *start, graphint *end) {
	double a, b, c, d, norm;
	
	graphint bit = 1 << (SCALE - 1);         /* size of original quandrant     */
	
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
	}
}

/* Remove self- and duplicate edges. We use a hash function and linked
 list to store non-duplicate edges.
 */
graphint Remove(graphint NV, graphint NE, graphint *sV, graphint *eV) {
	graphint i, NGE = 0;
	graphint *head = (graphint *) xmalloc(NV * sizeof(graphint));
	graphint *next = (graphint *) xmalloc(NE * sizeof(graphint));
	
	/* Initialize linked lists */
	for (i = 0; i < NV; i++) head[i] = -1;
	for (i = 0; i < NE; i++) next[i] = -2;
	
//	MTA("mta assert no dependence")
	for (i = 0; i < NE; i++) {
		graphint k, *ptr;
		graphint sv  = sV[i];
		graphint ev  = eV[i];
		graphint key = sv ^ ev;               /* hash function */
		if (key == 0) continue;          /* self-edge */
		
		ptr = head + key;
		k   = readfe(ptr);
		
		/* Search this key's linked list for this edge */
		while (k != -1) {
			if ((sV[k] == sv) & (eV[k] == ev)) break;   /* duplicate edge */
			
			writeef(ptr, k);
			ptr = next + k;
			k   = readfe(ptr);
		}
		
		/* Add a new edge to end of this list */
		if (k == -1) {k = i; next[i] = -1; NGE++;}
		writeef(ptr, k);
	}
	
	/* Move good edges to front of sV and eV arrays */
//	MTA("mta assert no dependence")
	for (i = 0; i < NGE; i++) {
		while (next[i] == -2) {
			graphint k = int_fetch_add(&NE, -1) - 1;
			sV[i] = sV[k]; eV[i] = eV[k]; next[i] = next[k];
		}
	}
	
	free(head); free(next);
	
	return NGE;
}
