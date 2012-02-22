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


static int
intcmp (const void *pa, const void *pb)
{
	const int64_t a = *(const int64_t *) pa;
	const int64_t b = *(const int64_t *) pb;

	return a - b;
}

/*
  Form transpose.
  Sort transpose.
  Count output degrees.
  Allocate output.
  Merge.
*/

graph *
makeUndirected(graph *G)
{
	graph * out = NULL;
	int64_t NV, NE;
	int64_t * restrict xoff, * restrict xadj, * restrict newoff;
	int64_t sum;

	if (!G) return NULL;
	out = xmalloc (sizeof (*out));
	NV = G->numVertices;
	NE = G->numEdges;

	xoff = (int64_t *)xmmap (NULL, (2*(NV+1) + 2*NE) * sizeof (int64_t), PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANON, 0,0);
	xadj = &xoff[NV+1];
	newoff = &xadj[2*NE];

	memset (xoff, 0, (2*(NV+1) + 2*NE) * sizeof (int64_t));

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NE; ++k)
		stinger_int_fetch_add (&xoff[1+G->endVertex[k]], 1);

	sum = 0;
	for (int64_t k = 1; k <= NV; ++k)
	{
		int64_t tmp = xoff[k];
		xoff[k] = sum;
		sum += tmp;
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t k = 0; k < NE; ++k)
	{
		const int64_t i = G->endVertex[k];
		const int64_t loc = stinger_int_fetch_add (&xoff[i+1], 1);
		xadj[2*loc] = G->startVertex[k];
		xadj[1+2*loc] = G->intWeight[k];
	}

	assert (xoff[NV] == NE);

	OMP("omp parallel for")
	MTA("mta assert parallel")
	for (int64_t k = 0; k < NV; ++k)
		if (xoff[k] < xoff[k+1])
			qsort (&xadj[2*xoff[k]], xoff[k+1] - xoff[k], 2*sizeof (int64_t), intcmp);

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t i = 0; i < NV; ++i) 
	{
		int64_t deg = xoff[i+1] - xoff[i];
		const int64_t adjend = G->edgeStart[i+1];

		for (int64_t k = G->edgeStart[i]; k != adjend; ++k)
		{
			const int64_t j = G->endVertex[k];
			void * foo = NULL;
      
			foo = bsearch (&j, &xadj[2*xoff[i]], xoff[i+1] - xoff[i], 2*sizeof (int64_t), intcmp);
      
			if (!foo) ++deg;
		}
		newoff[i] = deg;
	}

	sum = 0;
	for (int64_t i = 0; i < NV; ++i)
	{
		int64_t tmp = newoff[i];
		newoff[i] = sum;
		sum += tmp;
	}
	newoff[NV] = sum;

	assert (sum <= 2*NE);

	alloc_graph (out, NV, sum);

	MTA("mta assert nodep")
	for (int64_t i = 0; i < NV; ++i)
	{
		int64_t kout = newoff[i];
		int64_t deg = xoff[i+1] - xoff[i];
		const int64_t adjend = G->edgeStart[i+1];

		for (int64_t k = 0; k < deg; ++k)
		{
			out->startVertex[kout] = i;
			out->endVertex[kout] = xadj[2*(k + xoff[i])];
			out->intWeight[kout] = xadj[1+2*(k + xoff[i])];
			++kout;
		}

		for (int64_t k = G->edgeStart[i]; k != adjend; ++k)
		{
			const int64_t j = G->endVertex[k];
			int64_t * ploc = NULL;

			MTA("mta assert nodep")
			for (int64_t kk = newoff[i]; kk < newoff[i]+deg; ++kk)
			{
				if (j == out->endVertex[kk]) ploc = &out->endVertex[kk];
			}

			if (!ploc)
			{
				out->startVertex[kout] = i;
				out->endVertex[kout] = j;
				out->intWeight[kout] = G->intWeight[k];
				++kout;
			}
			else
			{
				int64_t loc = (ploc - out->endVertex) / sizeof (int64_t);
				out->intWeight[loc] += G->intWeight[k];
			}
		}
		assert (kout == newoff[i+1]);
	}

	assert (sum == newoff[NV]);

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t i = 0; i <= NV; ++i) out->edgeStart[i] = newoff[i];

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (int64_t i = 0; i < NV; ++i) out->marks[i] = 0;

	munmap (xoff, (NV+1 + 2*NE) * sizeof (int64_t));

	return out;
}
