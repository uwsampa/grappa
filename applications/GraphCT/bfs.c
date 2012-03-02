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


/* mode = 0:  return an array of furthest vertices
   mode = 1:  return the longest distance
*/

int64_t * calculateBFS(graph *G, int64_t startV, int64_t mode)
{
	int64_t NV = G->numVertices;
	int64_t *eV = G->endVertex;
	int64_t *start = G->edgeStart;

	int64_t *Q = xmalloc(NV * sizeof(int64_t));
	int64_t *dist = xmalloc(NV * sizeof(int64_t));
	int64_t *marks = xmalloc((NV + 2) * sizeof(int64_t));
	int64_t *QHead = xmalloc(10 * SCALE * sizeof(int64_t));

	int64_t maxDist;
	int64_t count;
	int64_t *results;
	int64_t j, k, s;
	int64_t nQ, Qnext, Qstart, Qend;
	int64_t d_phase;

	OMP("omp parallel for")
	for (j = 0; j < NV; j++) {
		dist[j] = -1;
		marks[j] = 0;
	}

	s = startV;

/* Push node s onto Q and set bounds for first Q sublist */
	Q[0] = s;
	Qnext = 1;
	nQ = 1;
	QHead[0] = 0;
	QHead[1] = 1;
	dist[s] = 0;
	marks[s] = 1;

PushOnStack:    /* Push nodes onto Q */

/* Execute the nested loop for each node v on the Q AND
   for each neighbor w of v  */
	d_phase = nQ;
	Qstart = QHead[nQ-1];
	Qend = QHead[nQ];

	OMP("omp parallel for schedule(dynamic)")
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

	maxDist = 0;
	for (j = 0; j < NV; j++) {
		if (dist[j] > maxDist) {
			maxDist = dist[j];
		}
	}

	if (mode == 0) {
		count = 0;
		for (j = 0; j < NV; j++) {
			if (dist[j] == maxDist) {
				count++;
			}
		}

		results = xmalloc((count + 1) * sizeof(int64_t));
		results[0] = count;
		k = 1;
		for (j = 0; j < NV; j++) {
			if (dist[j] == maxDist) {
				results[k] = j;
				k++;
			}
		}
	}

	free(Q);
	free(dist);
	free(marks);
	free(QHead);

	if (mode == 0) return results;
	else return (int64_t *) maxDist;

}

