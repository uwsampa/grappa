// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/mman.h>

#include "defs.h"
#include "stinger-atomics.h"


/* A bucket sort */
static void SortStart(graphint NV, graphint NE, graphint * sv1, graphint * ev1, graphint * w1, graphint * sv2, graphint * ev2, graphint * w2, graphint * start) {
	graphint i;
	
	for (i = 0; i < NV + 2; i++) start[i] = 0;
	
	start += 2;
	
	/* Histogram key values */
	MTA("mta assert no alias *start *sv1")
	//	for (i = 0; i < NE; i++) start[sv1[i]] ++;
	for (i = 0; i < NE; i++) stinger_int64_fetch_add(&start[sv1[i]], 1);
	
	/* Compute start index of each bucket */
	for (i = 1; i < NV; i++) start[i] += start[i-1];
	
	start--;
	
	/* Move edges into its bucket's segment */
	MTA("mta assert no dependence")
	for (i = 0; i < NE; i++) {
		//		int64_t index = start[sv1[i]] ++;
		graphint index = stinger_int64_fetch_add(&start[sv1[i]], 1);
		sv2[index] = sv1[i];
		ev2[index] = ev1[i];
		w2[index] = w1[i];
	}
}


graph* computeGraph(graphedges* elist) {
	graphint i, NE, NV;
	graphint *sv1, *ev1;
	
	graph* g = xmalloc(sizeof(graph));
	
	NE  = elist->numEdges;
	sv1 = elist->startVertex;
	ev1 = elist->endVertex;
	
	NV = 0;
	for (i = 0; i < NE; i++) NV = (NV > sv1[i]) ? NV : sv1[i];
	for (i = 0; i < NE; i++) NV = (NV > ev1[i]) ? NV : ev1[i];
	NV ++;
	
	alloc_graph(g, NV, NE);
	
	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (i = 0; i < NV; i++) g->marks[i] = 0;
	
	/*-------------------------------------------------------------------------*/
	/* STEP 0: Sort edges                                                      */
	/*-------------------------------------------------------------------------*/
	
	SortStart(NV, NE, sv1, ev1, elist->intWeight,
			  g->startVertex, g->endVertex, g->intWeight, g->edgeStart);
	
	return g;
}

static int intcmp(const void *pa, const void *pb) {
	const graphint a = *(const graphint *) pa;
	const graphint b = *(const graphint *) pb;
	return (int)(a - b);
}

static void inline swap(graphint *a, graphint *b) {
	graphint tmp = *a;
	*a = *b;
	*b = tmp;
}

typedef struct SWAPTARGET {
	void* target; //graph
	void* sort_base; //&endVertex[0]
	void (*swapfunc)(struct SWAPTARGET* target, void* a, void* b);
} swap_target;

static void siftDown(graphint numbers[], graphint root, graphint bottom, swap_target* swapt) {
	graphint maxChild = root * 2 + 1;
	
	// Find the biggest child
	if(maxChild < bottom) {
		graphint otherChild = maxChild + 1;
		// Reversed for stability
		maxChild = (numbers[otherChild] > numbers[maxChild])?otherChild:maxChild;
	} else {
		// Don't overflow
		if(maxChild > bottom) return;
	}
	
	// If we have the correct ordering, we are done.
	if(numbers[root] >= numbers[maxChild]) return;
	
	// Swap
	swapt->swapfunc(swapt, &numbers[root], &numbers[maxChild]);
	
	// Tail queue recursion. Will be compiled as a loop with correct compiler switches.
	siftDown(numbers, maxChild, bottom, swapt);
}

static void heapSort(graphint *numbers, graphint array_size, swap_target* swapt) {
	graphint i;
	
	for (i = (array_size / 2); i >= 0; i--) {
		siftDown(numbers, i, array_size - 1, swapt);
	}
	
	for (i = array_size-1; i >= 1; i--) {
		// Swap
		swapt->swapfunc(swapt, &numbers[0], &numbers[i]);
		
		siftDown(numbers, 0, i-1, swapt);
	}
}

static void swap_edges(swap_target* t, void* a, void* b) {
	graph* g = (graph*)t->target;
	
	graphint offa = (graphint)(((graphint*)a) - ((graphint*)t->sort_base));
	graphint offb = (graphint)(((graphint*)b) - ((graphint*)t->sort_base));
	
	swap(&g->endVertex[offa], &g->endVertex[offb]);	
	swap(&g->startVertex[offa], &g->startVertex[offb]);
	swap(&g->intWeight[offa], &g->intWeight[offb]);
}

static void sort_edges(graph *g) {
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
	graphint * restrict eV = g->endVertex; /* Edge domain */
	
	swap_target swapt;
	swapt.target = g;
	swapt.sort_base = eV;
	swapt.swapfunc = swap_edges;
	
	for (graphint i=0; i<NV; i++) {
		graphint start = edge[i];
		graphint degree = edge[i+1] - start;
//		qsort(eV+start, nel, sizeof(graphint), intcmp);
		heapSort(&eV[start], degree, &swapt);
	}
	// TODO: need to keep "weights" and other edge information consistent with moving edges...
}

//
//graph* makeDedup(graph *g) {
//	const graphint	NV = g->numVertices,
//					NE = g->numEdges;
//	
//	graph * dg = xmalloc(sizeof(graph));
//	dg->numVertices = NV;
//	
//	graphint * restrict degree = xmalloc(NV*sizeof(graphint));
//	
//	for (graphint i=0; i<NV; i++) {
//		
//	}
//}

/*
 Form transpose.
 Sort transpose.
 Count output degrees.
 Allocate output.
 Merge.
 */
graph* makeUndirected(graph *g) {
	graphint NV, NE;
	graphint * restrict xoff, * restrict xadj, * restrict newoff;
	graphint sum;
	
	if (!g) return NULL;
	
	graph* undirG = xmalloc(sizeof(graph));
	NV = g->numVertices;
	NE = g->numEdges;
	
	xoff = (graphint*)xmmap(NULL, (2*(NV+1) + 2*NE) * sizeof(graphint), PROT_READ | PROT_WRITE,
							 MAP_PRIVATE | MAP_ANON, 0,0);
	xadj = &xoff[NV+1];
	newoff = &xadj[2*NE];
	
	memset(xoff, 0, (2*(NV+1) + 2*NE) * sizeof(graphint));
	
	MTA("mta assert nodep")
	for (graphint k = 0; k < NE; ++k)
		stinger_int64_fetch_add(&xoff[1+g->endVertex[k]], 1);
	
	sum = 0;
	for (graphint k = 1; k <= NV; ++k) {
		graphint tmp = xoff[k];
		xoff[k] = sum;
		sum += tmp;
	}
	
	MTA("mta assert nodep")
	for (graphint k = 0; k < NE; ++k) {
		const graphint i = g->endVertex[k];
		const graphint loc = stinger_int64_fetch_add(&xoff[i+1], 1);
		xadj[2*loc] = g->startVertex[k];
		xadj[1+2*loc] = g->intWeight[k];
	}
	
	assert(xoff[NV] == NE);
	
	MTA("mta assert parallel")
	for (graphint k = 0; k < NV; ++k)
		if (xoff[k] < xoff[k+1])
			qsort(&xadj[2*xoff[k]], xoff[k+1] - xoff[k], 2*sizeof(graphint), intcmp);
	
	MTA("mta assert nodep")
	for (graphint i = 0; i < NV; ++i) {
		graphint deg = xoff[i+1] - xoff[i];
		const graphint adjend = g->edgeStart[i+1];
		
		for (graphint k = g->edgeStart[i]; k != adjend; ++k) {
			const graphint j = g->endVertex[k];
			void * foo = NULL;
			
			foo = bsearch(&j, &xadj[2*xoff[i]], xoff[i+1] - xoff[i], 2*sizeof(graphint), intcmp);
			
			if (!foo) ++deg;
		}
		newoff[i] = deg;
	}
	
	sum = 0;
	for (graphint i = 0; i < NV; ++i) {
		graphint tmp = newoff[i];
		newoff[i] = sum;
		sum += tmp;
	}
	newoff[NV] = sum;
	
	assert(sum <= 2*NE);
	
	alloc_graph(undirG, NV, sum);
	
	MTA("mta assert nodep")
	for (graphint i = 0; i < NV; ++i) {
		graphint kout = newoff[i];
		graphint deg = xoff[i+1] - xoff[i];
		const graphint adjend = g->edgeStart[i+1];
		
		for (graphint k = 0; k < deg; ++k) {
			undirG->startVertex[kout] = i;
			undirG->endVertex[kout] = xadj[2*(k + xoff[i])];
			undirG->intWeight[kout] = xadj[1+2*(k + xoff[i])];
			++kout;
		}
		
		for (graphint k = g->edgeStart[i]; k != adjend; ++k) {
			const graphint j = g->endVertex[k];
			graphint * ploc = NULL;
			
			MTA("mta assert nodep")
			for (graphint kk = newoff[i]; kk < newoff[i]+deg; ++kk) {
				if (j == undirG->endVertex[kk]) ploc = &undirG->endVertex[kk];
			}
			
			if (!ploc) {
				undirG->startVertex[kout] = i;
				undirG->endVertex[kout] = j;
				undirG->intWeight[kout] = g->intWeight[k];
				++kout;
			} else {
				graphint loc = (graphint)(ploc - undirG->endVertex) / sizeof(graphint);
				undirG->intWeight[loc] += g->intWeight[k];
			}
		}
		assert(kout == newoff[i+1]);
	}
	
	assert(sum == newoff[NV]);
	
	MTA("mta assert nodep")
	for (graphint i = 0; i <= NV; ++i) undirG->edgeStart[i] = newoff[i];
	
	MTA("mta assert nodep")
	for (graphint i = 0; i < NV; ++i) undirG->marks[i] = 0;
	
	munmap((caddr_t)xoff, (NV+1 + 2*NE) * sizeof(graphint));
	
	sort_edges(undirG);
	
	return undirG;
}




