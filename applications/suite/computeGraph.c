// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdlib.h>
#include <stdio.h>
#include "defs.h"
#include "stinger-atomics.h"


/* A bucket sort */
void SortStart(graphint NV, graphint NE, graphint * sv1, graphint * ev1, graphint * w1, graphint * sv2, graphint * ev2, graphint * w2, graphint * start) {
	graphint i;
	
	for (i = 0; i < NV + 2; i++) start[i] = 0;
	
	start += 2;
	
	/* Histogram key values */
	MTA("mta assert no alias *start *sv1")
	//	for (i = 0; i < NE; i++) start[sv1[i]] ++;
	for (i = 0; i < NE; i++) stinger_int_fetch_add(&start[sv1[i]], 1);
	
	/* Compute start index of each bucket */
	for (i = 1; i < NV; i++) start[i] += start[i-1];
	
	start--;
	
	/* Move edges into its bucket's segment */
	MTA("mta assert no dependence")
	for (i = 0; i < NE; i++) {
		//		int64_t index = start[sv1[i]] ++;
		int64_t index = stinger_int_fetch_add(&start[sv1[i]], 1);
		sv2[index] = sv1[i];
		ev2[index] = ev1[i];
		w2[index] = w1[i];
	}
}


void computeGraph(graph* g, edgelist* elist) {
	graphint i, NE, NV;
	graphint *sv1, *ev1;
	
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
}

