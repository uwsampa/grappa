#include <stdio.h>
#include "defs.h"
#include "globals.h"
#include <malloc.h>
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include "stinger-atomics.h"

/* Input array *membership indicates the coloring by specifying an
   integer "color" for each vertex.  numColors indicates the number
   of colors where the colors are 0 to (numColors-1).  */

double computeModularityValue(graph *G, int64_t *membership, int64_t numColors)
{
	int64_t NV = G->numVertices;
	int64_t * start = G->edgeStart;
	int64_t * eV = G->endVertex;
	int64_t i,j;
	int64_t comm;
	double mod = 0.0;
	int64_t degree_u, degree_v;
	int64_t posmod = 0;
	int64_t negmod = 0;
	double modularity = 0.0;

	int64_t *sumtable = (int64_t *) xmalloc(numColors * sizeof(int64_t));

	OMP("omp parallel for")
	for (i = 0; i < numColors; i++)
	{
		sumtable[i] = 0;
	}

	OMP("omp parallel for")
	MTA("mta assert parallel")
	for (i = 0; i < NV; i++)
	{
		degree_v = start[i+1] - start[i];
		stinger_int_fetch_add(sumtable + membership[i], degree_v);
	}

	OMP("omp parallel for")
	MTA("mta assert parallel")
	for (i = 0; i < NV; i++)
	{
		comm = membership[i];
		int64_t myStart = start[i];
		int64_t myEnd = start[i+1];
		degree_u = myEnd - myStart;

		for (j = myStart; j < myEnd; j++)
		{
			if (membership[eV[j]] == comm)
			{
				stinger_int_fetch_add(&posmod, 1);
			}
		}

		stinger_int_fetch_add(&negmod, -degree_u*sumtable[membership[i]]);
	}		

	mod = posmod + negmod/(2.0*G->numEdges);
	printf("Modularity is: %9.6lf\n",mod/(2*G->numEdges));

	free(sumtable);

	return modularity;
}
