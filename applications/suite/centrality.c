// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"

#ifdef __MTA__
#	include <sys/mta_task.h>
#	include <machine/runtime.h>
#else
#	include "compat/xmt-ops.h"
#endif
#include "stinger-atomics.h"


double centrality(graph *g, double *bc, graphint Vs) {
	graphint num_srcs = 0;
	
	graphint NE      = g->numEdges;
	graphint NV      = g->numVertices;
	graphint *eV     = g->endVertex;
	graphint *start  = g->edgeStart;
	
	/* Allocate memory for data structures */
	graphint *Q     = (graphint *) xmalloc(NV * sizeof(graphint));
	graphint *dist  = (graphint *) xmalloc(NV * sizeof(double));
	graphint *sigma = (graphint *) xmalloc(NV * sizeof(graphint));
	graphint *marks = (graphint *) xmalloc((NV + 2) * sizeof(graphint));
	graphint *QHead = (graphint *) xmalloc(100 * SCALE * sizeof(graphint));
	graphint *child  = (graphint *) xmalloc(NE * sizeof(graphint));
	graphint *child_count = (graphint *) xmalloc(NV * sizeof(graphint));
	graphint *explored = (graphint *) xmalloc(sizeof(graphint)*NV);
	
	graphint j, k;
	graphint nQ, Qnext, Qstart, Qend;
	
	/* Reuse the dist memory in the accumulation phase */
	double *delta = (double *) dist;
	
	OMP("omp parallel for")
	for (j = 0; j < NV; j++) {
		bc[j] = 0.0;
		explored[j] = 0;
	}
		
	graphint x;
	
	/* Use |Vs| nodes to compute centrality values */
	for (x = 0; (x < NV) && (Vs > 0); x ++) {
		graphint d_phase;
		graphint s = rand() % NV;
		
		while(explored[s]) {
			s = rand() % NV;
		}
		
		explored[s] = 1;
		if (start[s+1] == start[s]) {
			continue; 
		} else {
			Vs --;
		}
		num_srcs++;
				
		OMP("omp parallel for")
		MTA("mta assert nodep")
		for (j = 0; j < NV; j++) {
			dist[j] = -1; 
			sigma[j] = marks[j] = child_count[j] = 0;
		}
				
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
		for (j = Qstart; j < Qend; j++) {
			graphint v = Q[j];
			graphint sigmav  = sigma[v];    
			graphint myStart = start[v];     
			graphint myEnd   = start[v+1];
			graphint ccount = 0;
			
			for (k = myStart; k < myEnd; k++) {
				graphint d, w, l;
				w = eV[k];                    
				d = dist[w];             
				/* If node has not been visited, set distance and push on Q (but only once) */
				
				if (d < 0) {
					if (stinger_int64_fetch_add(&marks[w], 1) == 0) {
						dist[w] = d_phase;
						Q[stinger_int64_fetch_add(&Qnext, 1)] = w;
					}
				 	stinger_int64_fetch_add(&sigma[w], sigmav);
					l = myStart + ccount++;
					child[l] = w;
				} else if (d == d_phase) {
					stinger_int64_fetch_add(&sigma[w], sigmav);
					l = myStart + ccount++;
					child[l] = w;
				}
			}
			child_count[v] = ccount;
		}
		
		/* If new nodes pushed onto Q */
		if (Qnext != QHead[nQ]) {
			nQ++;
			QHead[nQ] = Qnext; 
			goto PushOnStack;
		}
				
		/* Dependence accumulation phase */
		nQ--;
		
		OMP("omp parallel for")
		for (j=0; j < NV; j++) delta[j] = 0.0;
		
		/* Pop nodes off of Q in the reverse order they were pushed on */
		for ( ; nQ > 1; nQ --) {
			Qstart = QHead[nQ-1];
			Qend   = QHead[nQ];
			/* For each v in the sublist AND for each w on v's list */
			MTA("mta assert parallel")
			MTA("mta block dynamic schedule")
			MTA("mta assert no alias *sigma *Q *bc *delta *child *start *QHead")
			for (j = Qstart; j < Qend; j++) {
				graphint v = Q[j]; 
				graphint myStart = start[v];
				graphint myEnd   = myStart + child_count[v];
				double sum = 0;
				double sigma_v = (double) sigma[v];
				for (k = myStart; k < myEnd; k++) {
					graphint w = child[k];
					sum += sigma_v * (1.0 + delta[w]) / (double) sigma[w];
				} 
				delta[v] = sum; 
				MTA("mta update")
				bc[v] += sum;
			}   
		}
	}
		
	free(Q);
	free(dist);
	free(sigma);
	free(QHead);
	free(marks);
	free(child);
	free(child_count);

	double bc_total = 0;
	for (graphint i=0; i<NV; i++) { bc_total += bc[i]; }
	
	return bc_total/NV;
}
