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


double * calculateTransitivityLocal(graph* G)
{
	int64_t NV = G->numVertices;
	int64_t NE = G->numEdges;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;

	int64_t * inDegree = (int64_t *) xmalloc(NV*sizeof(int64_t));

	int64_t i;
	int64_t * sV = xmalloc(sizeof(int64_t)*NE);
	double * cc_array = (double *) xmalloc(sizeof(double)*NV);

	OMP("omp parallel for")
	for (i = 0; i < NV; i++)
	{
		inDegree[i] = 0;
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	MTA("mta interleave schedule")
	for(i = 0; i < NV; i++)
	{
		int64_t begin = start[i];
		int64_t end = start[i+1];
		int64_t j;

		MTA("mta assert no alias")
		MTA("mta assert nodep")
		for(j = begin; j < end; j++)
		{
			sV[j] = i;
			stinger_int_fetch_add(&inDegree[eV[j]], 1);
		}
	}
	
	int64_t * start2 = xmalloc(sizeof(int64_t)*(NV+2));
	int64_t * eV2 = xmalloc(sizeof(int64_t)*NE);

	SortStart2(NV, NE, eV, sV, eV2, start2);

	OMP("omp parallel for")
	MTA("mta assert parallel")
	for(i=0; i<NV; i++)
	{
		int64_t begin = start[i];
		int64_t end = start[i+1];
		int64_t begin2 = start2[i];
		int64_t end2 = start2[i+1];
		qsort(eV+begin, end-begin, sizeof(int64_t), compare);
		qsort(eV2+begin2, end2-begin2, sizeof(int64_t), compare);
	}

	OMP("omp parallel for")
	MTA("mta assert nodep")
	MTA("mta interleave schedule")
	for(i = 0; i < NV; i++)
	{
		int64_t begin = start[i];
		int64_t end = start[i+1];
		int64_t begin2 = start2[i];
		int64_t end2 = start2[i+1];
		int64_t outDegree = end - begin;
		int64_t local_num = 0;
		int64_t local_den = 0;
		
		int64_t j;
		for(j = begin2; j < end2; j++)
		{
			local_num += count_intersect(start, eV, start, eV, i, eV2[j]);
		}

		j = begin;
		int64_t k = begin2;
		
		while(j < end && k < end2)
		{
			if(eV[j] == eV2[k]) 
			{
				local_den++;
				j++;
				k++;
			}
			else if(eV[j] < eV2[k]) j++;
			else k++;
		}
		local_den = outDegree*inDegree[i] - local_den;
		if(local_den == 0.0) local_den = 1.0;

		cc_array[i] = local_num/(double) local_den;
	}

	return cc_array;
}
