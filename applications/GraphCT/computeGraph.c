#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"
#include "stinger-atomics.h"


/* A bucket sort */
void SortStart(int64_t NV, int64_t NE, int64_t * sv1, int64_t * ev1, int64_t * w1, int64_t * sv2, int64_t * ev2, int64_t * w2, int64_t * start)
{
	int64_t i;

	OMP("omp parallel for")
	for (i = 0; i < NV + 2; i++) start[i] = 0;

	start += 2;

	/* Histogram key values */
	OMP("omp parallel for")
	MTA("mta assert no alias *start *sv1")
//	for (i = 0; i < NE; i++) start[sv1[i]] ++;
	for (i = 0; i < NE; i++) stinger_int_fetch_add(&start[sv1[i]], 1);

	/* Compute start index of each bucket */
	for (i = 1; i < NV; i++) start[i] += start[i-1];

	start --;

	/* Move edges into its bucket's segment */
	OMP("omp parallel for")
	MTA("mta assert no dependence")
	for (i = 0; i < NE; i++) {
//		int64_t index = start[sv1[i]] ++;
		int64_t index = stinger_int_fetch_add(&start[sv1[i]], 1);
		sv2[index] = sv1[i];
		ev2[index] = ev1[i];
		w2[index] = w1[i];
	}
}


void computeGraph(graph* G, graphSDG* SDGdata)
{
	int64_t i, NE, NV;
	int64_t *sv1, *ev1;

	NE  = SDGdata->numEdges;
	sv1 = SDGdata->startVertex;
	ev1 = SDGdata->endVertex;

	NV = 0;
	for (i = 0; i < NE; i++) NV = (NV > sv1[i]) ? NV : sv1[i];
	for (i = 0; i < NE; i++) NV = (NV > ev1[i]) ? NV : ev1[i];
	NV ++;

	alloc_graph (G, NV, NE);

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (i = 0; i < NV; i++) G->marks[i] = 0;

	/*-------------------------------------------------------------------------*/
	/* STEP 0: Sort edges                                                      */
	/*-------------------------------------------------------------------------*/

	SortStart(NV, NE, sv1, ev1, SDGdata->intWeight,
		G->startVertex, G->endVertex, G->intWeight, G->edgeStart);
}
