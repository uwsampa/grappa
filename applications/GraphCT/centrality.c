#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include "stinger-atomics.h"


/* Set this variable to 1 to time each iteration */
#define TIMEBCPART 0

double centrality(graph *G, double *BC, int64_t Vs)
{  
	int64_t num_srcs = 0;  
	double timeBC, timep1, timep2, timer();

	int64_t NE      = G->numEdges;
	int64_t NV      = G->numVertices;
	int64_t *eV     = G->endVertex;
	int64_t *start  = G->edgeStart;

	/* Allocate memory for data structures */
	int64_t *Q     = (int64_t *) xmalloc(NV * sizeof(int64_t));
	int64_t *dist  = (int64_t *) xmalloc(NV * sizeof(double));
	int64_t *sigma = (int64_t *) xmalloc(NV * sizeof(int64_t));
	int64_t *marks = (int64_t *) xmalloc((NV + 2) * sizeof(int64_t));
	int64_t *QHead = (int64_t *) xmalloc(100 * SCALE * sizeof(int64_t));
	int64_t *child  = (int64_t *) xmalloc(NE * sizeof(int64_t));
	int64_t *child_count = (int64_t *) xmalloc(NV * sizeof(int64_t));
	int64_t *explored = (int64_t *) xmalloc(sizeof(int64_t)*NV);

	int64_t j, k;
	int64_t nQ, Qnext, Qstart, Qend;

	/* Reuse the dist memory in the accumulation phase */
	double *delta = (double *) dist;

	OMP("omp parallel for")
	for (j = 0; j < NV; j++) {
		BC[j] = 0.0;
		explored[j] = 0;
	}

	timeBC = timer();

	int64_t x;

	/* Use |Vs| nodes to compute centrality values */
	for (x = 0; (x < NV) && (Vs > 0); x ++) {
		int64_t d_phase;
		int64_t s = rand() % NV;

		while(explored[s])
		{
			s = rand() % NV;
		}

		explored[s] = 1;
		if (start[s+1] == start[s]) {
			  continue; 
		} else {
			  Vs --;
		}
		num_srcs++;

#if TIMEBCPART      
		timep1 = timer();
		timep2 = timer();
#endif

		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (j = 0; j < NV; j++)
		{
			dist[j] = -1; 
			sigma[j] = marks[j] = child_count[j] = 0;
		}

#if TIMEBCPART      
		timep1 = timer() - timep1;
		fprintf(stderr, "Src: %d, initialization time: %9.6lf sec.\n", s, timep1);
		timep1 = timer();
#endif

		/* Push node i onto Q and set bounds for first Q sublist */
		Q[0]     = s;
		Qnext    = 1;

		nQ       = 1;
		QHead[0] = 0;
		QHead[1] = 1;

		dist[s]  = 0;
		marks[s] = 1;
		sigma[s] = 1;

PushOnStack:    /* Push nodes onto Q */

		/* Execute the nested loop for each node v on the Q AND
		for each neighbor w of v whose edge weight is not divisible by 8
		*/
		d_phase = nQ;
		Qstart = QHead[nQ-1];
		Qend = QHead[nQ];

		MTA("mta assert no dependence")
		MTA("mta block dynamic schedule")
		for (j = Qstart; j < Qend; j++)
		{
			int64_t v = Q[j];
			int64_t sigmav  = sigma[v];    
			int64_t myStart = start[v];     
			int64_t myEnd   = start[v+1];
			int64_t ccount = 0;

			for (k = myStart; k < myEnd; k++)
			{
				int64_t d, w, l;
				w = eV[k];                    
				d = dist[w];             
				/* If node has not been visited, set distance and push on Q (but only once) */

				if (d < 0) {
					if (stinger_int_fetch_add(&marks[w], 1) == 0)
					{
						dist[w] = d_phase;
						Q[stinger_int_fetch_add(&Qnext, 1)] = w;
					}
				 	stinger_int_fetch_add(&sigma[w], sigmav);
					l = myStart + ccount++;
					child[l] = w;
				}
				else if (d == d_phase)
				{
					stinger_int_fetch_add(&sigma[w], sigmav);
					l = myStart + ccount++;
					child[l] = w;
				}
			}
			child_count[v] = ccount;
		}


		/* If new nodes pushed onto Q */
		if (Qnext != QHead[nQ]) 
		{
			nQ++;
			QHead[nQ] = Qnext; 
			goto PushOnStack;
		}

#if TIMEBCPART
		timep1 = timer() - timep1;
		fprintf(stderr, "Traversal time: %9.6lf sec, visited: %d\n", timep1, QHead[nQ]);
		timep1 = timer();
#endif

#if 0
		/* Code snippet to count the size of child multiset */
		int64_t sum = 0;
		for (j = 0; j < NV; j++) {
		sum = sum + child_count[j];
		}
		avg_frac += (double)sum/NE;
		fprintf(stderr, "child count: %d, fraction: %lf, visited: %d\n", sum,
		  (double) sum/NE, QHead[nQ]);
#endif

		/* Dependence accumulation phase */
		nQ--;

		OMP("omp parallel for")
		for (j=0; j < NV; j++) delta[j] = 0.0;

		/* Pop nodes off of Q in the reverse order they were pushed on */
		for ( ; nQ > 1; nQ --)
		{
			Qstart = QHead[nQ-1];
			Qend   = QHead[nQ];
		/* For each v in the sublist AND for each w on v's list */
			MTA("mta assert parallel")
			MTA("mta block dynamic schedule")
			MTA("mta assert no alias *sigma *Q *BC *delta *child *start *QHead")
			for (j = Qstart; j < Qend; j++) {
				int64_t v = Q[j]; 
				int64_t myStart = start[v];
				int64_t myEnd   = myStart + child_count[v];
				double sum = 0;
				double sigma_v = (double) sigma[v];
				for (k = myStart; k < myEnd; k++) {
					int64_t w = child[k];
					sum += sigma_v * (1.0 + delta[w]) / (double) sigma[w];
				} 
				delta[v] = sum; 
				MTA("mta update")
				BC[v] += sum;
			}   
		}

#if TIMEBCPART 
		timep1 = timer() - timep1;
		fprintf(stderr, "Accumulation time: %9.6lf sec.\n", timep1);
		timep2 = timer() - timep2;
		fprintf(stderr, "Src: %d, total time: %9.6lf sec.\n", s, timep2);
#endif
	 
	}

	timeBC = timer() - timeBC;

	free(Q);
	free(dist);
	free(sigma);
	free(QHead);
	free(marks);
	free(child);
	free(child_count);

	return timeBC;
}
