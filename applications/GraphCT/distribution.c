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


void calculateDegreeDistributions(graph *G)
{
	int64_t sum = 0, sum_sq = 0;
	double average, avg_sq, variance, std_dev;
	int64_t maxDegree = 0;
	int64_t i;

#ifdef __MTA__
	for (i = 0; i < G->numVertices; i++) {
		int64_t degree = G->edgeStart[i+1] - G->edgeStart[i];
		sum_sq += degree*degree;
		sum += degree;
		if (degree > maxDegree)
			maxDegree = degree;
	}
#else
	OMP("omp parallel")
	{
	int64_t myMaxDegree = 0;
	OMP("omp for reduction(+:sum_sq) reduction(+:sum)")
	for (i = 0; i < G->numVertices; i++) {
		int64_t degree = G->edgeStart[i+1] - G->edgeStart[i];
		sum_sq += degree*degree;
		sum += degree;
		if (degree > myMaxDegree)
			myMaxDegree = degree;
	}

	if (myMaxDegree > maxDegree)
	{
		OMP("omp critical")
		{
			if (myMaxDegree > maxDegree) maxDegree = myMaxDegree;
		}

	}
	}
#endif

	average = (double) sum / G->numVertices;
	avg_sq = (double) sum_sq / G->numVertices;
	variance = avg_sq - (average*average);
	std_dev = sqrt(variance);

	printf("Maximum out-degree is:  %ld\n", maxDegree);
	printf("Average out-degree is:  %f\n", average);
	printf("Expected value of X^2:  %f\n", avg_sq);
	printf("Variance is:  %f\n", variance);
	printf("Standard deviation:  %f\n", std_dev);

}

void calculateComponentDistributions(graph *G, int64_t numColors, int64_t *max, int64_t *maxV)
{
	int64_t *cardinality;
	int64_t sum = 0, sum_sq = 0;
	double average, avg_sq, variance, std_dev;
	int64_t i;
	int64_t NV = G->numVertices;
	int64_t *color = G->marks;

	*max = 0;
	*maxV = 0;

	cardinality = xmalloc(NV * sizeof(uint64_t));

	OMP("omp parallel for")
	for (i = 0; i < NV; i++) {
		cardinality[i] = 0;
	}

	OMP("omp parallel for")
	for (i = 0; i < NV; i++) {
//		cardinality[color[i]]++;
		stinger_int_fetch_add(&cardinality[color[i]], 1);
	}

#ifdef __MTA__
	for (i = 0; i < NV; i++) {
		uint64_t degree = cardinality[i];
		sum_sq += degree*degree;
		sum += degree;
		
		if (degree > *max) {
			*max = degree;
		}
	}
#else
	OMP("omp parallel")
	{
	int64_t myMaxDegree = 0;
	OMP("omp for reduction(+:sum_sq) reduction(+:sum)")
	for (i = 0; i < NV; i++) {
		uint64_t degree = cardinality[i];
		sum_sq += degree*degree;
		sum += degree;
		
		if (degree > myMaxDegree) {
			myMaxDegree = degree;
		}
	}

	if (myMaxDegree > *max)
	{
		OMP("omp critical")
		{
			if (myMaxDegree > *max) *max = myMaxDegree;
		}
	}
	}
#endif

	uint64_t max2 = 0;
	int64_t checker = *max;

	MTA("mta assert nodep")
	for (i = 0; i < NV; i++)
	{
		uint64_t degree = cardinality[i];
		uint64_t tmp = (checker == degree ? i : 0);
		if(tmp > max2)
			max2 = tmp;
	}
	*maxV = max2;
	

	average = (double) sum / (double) numColors;
	avg_sq = (double) sum_sq / (double) numColors;
	variance = avg_sq - (average*average);
	std_dev = sqrt(variance);

	printf("Maximum # of vertices:  %ld\n", *max);
	printf("Average # of vertices:  %f\n", average);
	printf("Expected value of X^2:  %f\n", avg_sq);
	printf("Variance:  %f\n", variance);
	printf("Standard deviation:  %f\n", std_dev);

	free(cardinality);

}

