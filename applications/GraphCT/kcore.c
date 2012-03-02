#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include <math.h>
#include "stinger-atomics.h"

graph * genSubGraph(graph * G, int64_t NV, int64_t color)
{
	int64_t i;
	int64_t * mask = G->marks;
	int64_t remain = NV;

// Calculates 'remain' if NV is passed as 0 (most applications)
	if (remain == 0) {
		remain = 0;
		for (i = 0; i < G->numVertices; i++) {
			if (mask[i] == color)
				remain += mask[i];
		}
	}

	NV = G->numVertices;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;
	int64_t * weight = G->intWeight;

	graph * G2 = xmalloc(sizeof(graph));

	int64_t NE2 = 0;
	int64_t * mapper = xmalloc(sizeof(int64_t)*remain);
	int64_t * backmap = xmalloc(sizeof(int64_t)*NV);
	int64_t id = 0;

	OMP("omp parallel for")
	for (i = 0; i < NV; ++i)
	{
		if (mask[i] == color) {
			const int64_t begin = start[i];
			const int64_t end = start[i+1];
			int64_t v = stinger_int_fetch_add (&id, 1);
			mapper[v] = i;
			backmap[i] = v;
			for (int64_t j = begin; j < end; ++j)
				if(mask[eV[j]] == color)
					stinger_int_fetch_add(&NE2, 1);
		}
	}

	alloc_graph (G2, remain, NE2);
	int64_t * start2 = G2->edgeStart;

	OMP("omp parallel for")
	for (i = 0; i < remain+2; i++)
	{
		start2[i] = 0;
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	for (i = 0; i < NV; i++)
	{
		if (mask[i] == color)
		{
			int64_t begin = start[i];
			int64_t end = start[i+1];
			int64_t j;

			for (j = begin; j < end; j++)
			{
				if (mask[eV[j]] == color)
				{
					stinger_int_fetch_add(&start2[mapper[i]+2], 1);
				}
			}
		}
	}

	for (i = 1; i < remain; i++)
	{
		start2[i+2] += start2[i+1];
	}
	
	int64_t * weight2 = G2->intWeight;
	int64_t * eV2 = G2->endVertex;

	MTA("mta assert nodep")
	for (i = 0; i < remain; i++)
	{
		int64_t v = mapper[i];
		int64_t begin = start[v];
		int64_t end = start[v+1];
		int64_t j;

		for(j = begin; j < end; j++)
		{
			if(mask[eV[j]] == color)
			{
				weight2[start2[i+1]] = weight[j];
				eV2[start2[i+1]++] = backmap[eV[j]];
			}
		}
		G2->marks[i] = 0;
	}
	
	free(backmap);
	free(mapper);

	return G2;
}

graph * kcore(graph * G, int64_t k)
{
	int64_t NV = G->numVertices;
	int64_t NE = G->numEdges;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;
	int64_t * mask = G->marks;
	graph * G2;
	
	int64_t i;
	OMP("omp parallel for")
	for(i = 0; i < NV; i++)
	{
		mask[i] = 0;
	}

	int64_t * degree = (int64_t *) xmalloc(sizeof(int64_t)*NV);

	OMP("omp parallel for")
	for(i = 0; i < NV; i++)
	{
		degree[i] = start[i+1] - start[i];
	}

	OMP("omp parallel for")
	for(i = 0; i < NE; i++)
	{
//		degree[eV[i]]++;
		stinger_int_fetch_add(&degree[eV[i]], 1);
	}

	int64_t remain = NV;
	int64_t iter = 0;

	while (1)
	{
		iter++;
		int64_t count = 0;

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (i = 0; i < NV; i++)
		{
			if ((!mask[i]) && degree[i] < k) 
			{
				mask[i] = iter;
				stinger_int_fetch_add(&count, 1);
			}
		}
	
		if (count==0) break;
		remain -= count;

		OMP("omp parallel for")
		MTA("mta interleave schedule")
		for (i = 0; i < NV; i++)
		{
			int64_t begin = start[i];
			int64_t end = start[i+1];
			int64_t j;

			for(j = begin; j < end; j++)
			{
				if (mask[eV[j]] == iter)
					stinger_int_fetch_add(&degree[i], -1);
			}
		}
	}

	G2 = genSubGraph(G, remain, 0);

	return G2;
}
