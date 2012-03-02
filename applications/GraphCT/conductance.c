#include <stdio.h>
#include "defs.h"
#include "globals.h"
#include <malloc.h>
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include "stinger-atomics.h"

/* Input array *membership indicates the "cut" using 0 and 1
   for each vertex in the graph.  */

double computeConductanceValue(graph * G, int64_t *membership)
{
	int64_t i,j;
	int64_t comm;
	double cond = 0.0;

	int64_t interST = 0;
	int64_t intraS = 0;
	int64_t intraT = 0;

OMP("omp parallel for")
MTA("mta assert parallel")
	for(i=0; i<G->numVertices; i++)
	{
		comm = membership[i];
		
		for(j=G->edgeStart[i]; j<G->edgeStart[i+1]; j++)
		{
			if(membership[G->endVertex[j]] == comm)
			{
				stinger_int_fetch_add(&interST, 1);
			}
			else if(comm == 1)
			{
				stinger_int_fetch_add(&intraS, 1);
			}
			else
			{
				stinger_int_fetch_add(&intraT, 1);
			}
		}

	}		

	if(intraS > intraT)
	{
		cond = interST/(double) (interST + intraT);
	}
	else
	{
		cond = interST/(double) (interST + intraS);
	}

	printf("Conductance is: %9.6lf\n", cond );

	return cond;
}
