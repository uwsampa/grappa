#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "defs.h"
#include "globals.h"
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include <sys/mman.h>
#include "stinger-atomics.h"


int64_t diameter_core(graph *G, int64_t s, int64_t *Q, int64_t *dist, int64_t *marks, int64_t *QHead, int64_t *diameter, int64_t x);

int64_t calculateGraphDiameter(graph *G, int64_t Vs)
{
	int64_t NV = G->numVertices;
	int64_t *start = G->edgeStart;
	int64_t *explored = (int64_t *) xmalloc (sizeof(int64_t) * NV);
	int64_t j;
	int64_t maxDist = 0;

	OMP("omp parallel for")
	for (j = 0; j < NV; j++)
	{
		explored[j] = j;
	}

	double *rn;
	rn = (double *) xmalloc (Vs * sizeof(double));
	prand(Vs, rn);

	MTA("mta assert nodep")
	for (j = 0; j < Vs; j++)
	{
		int64_t swap = (int64_t) (rn[j] * (NV - j)) + j;
		int64_t tmp = explored[swap];
		explored[swap] = explored[j];
		explored[j] = tmp;
	}

	int64_t x;
	int64_t k = 0;

#if defined(__MTA__)
#define INC 256
#else
#define INC 10
#endif

	int64_t buffsz = INC * NV * sizeof(int64_t) +
				 INC * NV * sizeof(int64_t) +
				 INC * (NV + 2) * sizeof(int64_t) +
				 INC * 10 * SCALE * sizeof(int64_t);
	
	int64_t *buff = (int64_t *) xmmap (NULL, buffsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);

	int64_t *Qbig = (int64_t *) buff;
	int64_t *distbig = (int64_t *) &buff[INC * NV];
	int64_t *marksbig = (int64_t *) &distbig[INC * NV];
	int64_t *QHeadbig = (int64_t *) &marksbig[INC * (NV + 2)];

	int64_t *diameter = (int64_t *) xmalloc (INC * sizeof(int64_t));

	for (x = 0; x < INC; x++)
	{
		diameter[x] = 0;
	}

	int64_t numSkipped = 0;

	OMP("omp parallel for")
	MTA("mta assert parallel")
	MTA("mta loop future")
	for (x = 0; x < INC; x++)
	{
		int64_t *Q = Qbig + x*NV;
		int64_t *dist = distbig + x*NV;
		int64_t *marks = marksbig + x*(NV+2);
		int64_t *QHead = QHeadbig + x*(10*SCALE);

		for (int64_t claimedk = stinger_int_fetch_add(&k, 1); claimedk < Vs; claimedk = stinger_int_fetch_add(&k, 1))
		{
			int64_t s = explored[claimedk];
			if (start[s+1] == start[s]) {
				stinger_int_fetch_add(&numSkipped, 1);
			} else {
				diameter_core(G, s, Q, dist, marks, QHead, diameter, x);
			}
		}
	}

	for (x = 0; x < INC; x++)
	{
		/* printf("%d: %d\n", x, diameter[x]); */
		if (diameter[x] > maxDist) {
			maxDist = diameter[x];
		}
	}

	free(diameter);
	munmap(buff, buffsz);
	free(explored);
	free(rn);

	return maxDist;
}



	
int64_t diameter_core(graph *G, int64_t s, int64_t *Q, int64_t *dist, int64_t *marks, int64_t *QHead, int64_t *diameter, int64_t x)
{
	int64_t NV = G->numVertices;
	int64_t *eV = G->endVertex;
	int64_t *start = G->edgeStart;

	int64_t maxDist = 0;

	int64_t j, k;
	int64_t nQ, Qnext, Qstart, Qend;

	int64_t d_phase;

	OMP("omp parallel for")
	for (j = 0; j < NV; j++) {
		dist[j] = -1;
		marks[j] = 0;
	}

/* Push node s onto Q and set bounds for first Q sublist */
	Q[0] = s;
	Qnext = 1;
	nQ = 1;
	QHead[0] = 0;
	QHead[1] = 1;
	dist[s] = 0;
	marks[s] = 1;

PushOnStack:	/* Push nodes onto Q */

/* Execute the nested loop for each node v on the Q AND
for each neighbor w of v  */
	d_phase = nQ;
	Qstart = QHead[nQ-1];
	Qend = QHead[nQ];

	OMP("omp parallel for")
	MTA("mta assert no dependence")
	MTA("mta block dynamic schedule")
	for (j = Qstart; j < Qend; j++) {
		int64_t v = Q[j];
		int64_t myStart = start[v];
		int64_t myEnd = start[v+1];
		for (k = myStart; k < myEnd; k++) {
			int64_t d, w;
			w = eV[k];
			d = dist[w];
/* If node has not been visited, set distance and push on Q */
			if (d < 0) {
				if (stinger_int_fetch_add(&marks[w], 1) == 0) {
					dist[w] = d_phase;
					Q[stinger_int_fetch_add(&Qnext, 1)] = w;
				}
			}
		}
	}

	if (Qnext != QHead[nQ]) {
		nQ++;
		QHead[nQ] = Qnext;
		goto PushOnStack;
	}

	for (j = 0; j < NV; j++) {
		if (dist[j] > maxDist) {
			maxDist = dist[j];
		} 
	}

	if (maxDist > diameter[x]) {
		diameter[x] = maxDist;
	}

	return maxDist;

}

