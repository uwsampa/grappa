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

#include "compat/mersenne.h"

static int64_t nedge_traversed;

/// Computes the approximate vertex betweenness centrality on an unweighted
/// graph using 'Vs' source vertices. Returns the average centrality.
double centrality(graph *g, double *bc, graphint Vs, int64_t* total_nedge) {
  graphint num_srcs = 0;
  nedge_traversed = 0;

  graphint NV      = g->numVertices;
  graphint *eV     = g->endVertex;
  graphint *start  = g->edgeStart;

  int computeAllVertices = Vs == NV;
  
  /* Allocate memory for data structures */
  graphint *Q     = (graphint *) xmalloc(NV * sizeof(graphint));
  graphint *Q2    = (graphint *) xmalloc(NV * sizeof(graphint));
  graphint *dist  = (graphint *) xmalloc(NV * sizeof(graphint));
  double *delta  = (graphint *) xmalloc(NV * sizeof(double));
  graphint *sigma = (graphint *) xmalloc(NV * sizeof(graphint));
  graphint *marks = (graphint *) xmalloc((NV + 2) * sizeof(graphint));
  graphint *QHead = (graphint *) xmalloc(100 * SCALE * sizeof(graphint));
  graphint *child_count = (graphint *) xmalloc(NV * sizeof(graphint));
  graphint *explored = (graphint *) xmalloc(sizeof(graphint)*NV);
  graphint *revisited = (graphint *) xmalloc(sizeof(graphint)*NV);
  
  graphint j, k;
  graphint nQ, Qnext, Qstart, Qend;
  
  /* Reuse the dist memory in the accumulation phase */
  //  double *delta = (double *) dist;
  
  /*srand(12345);*/
  mersenne_seed(12345);
  
  OMP("omp parallel for")
  for (j = 0; j < NV; j++) {
    bc[j] = 0.0;
    explored[j] = 0;
  }
  
  graphint x;
  
  /* Use |Vs| nodes to compute centrality values */
  for (x = 0; (x < NV) && (Vs > 0); x ++) {
    graphint d_phase;
    graphint s;
    graphint leaves = 0;
    
    if (computeAllVertices) {
      s = x;
    } else {
      double d;
      do {
        s = (graphint)(mersenne_rand() % NV);
        deprint("s (%ld)\n", s);
      } while (explored[s]);
      explored[s] = 1;
    }
    
    deprint("degree (%ld)\n", start[s+1]-start[s]);

    if (start[s+1] == start[s]) {
      continue; 
    } else {
      Vs--;
    }
    num_srcs++;
    
    deprint("s = %lld\n", s);
      
    OMP("omp parallel for")
    MTA("mta assert nodep")
    for (j = 0; j < NV; j++) {
      dist[j] = -1; 
      sigma[j] = marks[j] = 0;
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
      
      nedge_traversed += myEnd - myStart;
      for (k = myStart; k < myEnd; k++) {
        graphint d, w;
        w = eV[k];
        d = dist[w];
        /* If node has not been visited, set distance and push on Q (but only once) */
        
        if (d < 0) {
          if (stinger_int64_fetch_add(&marks[w], 1) == 0) {
            dist[w] = d_phase;
            Q[stinger_int64_fetch_add(&Qnext, 1)] = w;
          }
          stinger_int64_fetch_add(&sigma[w], sigmav);
          ccount++;
        } else if (d == d_phase) {
          stinger_int64_fetch_add(&sigma[w], sigmav);
          ccount++;
        }
      }
      child_count[v] = ccount; //if zero, v is a digraph leaf
      if (ccount == 0) Q2[leaves++] = v; 
    }

    /* If new nodes pushed onto Q */
    if (Qnext != QHead[nQ]) {
      nQ++;
      QHead[nQ] = Qnext; 
      goto PushOnStack;
    }

    fprintf(stderr, "leaves = %d sigma[12] = %d Qnext = %d \n", leaves, sigma[12], Qnext);    
    OMP("omp parallel for")
    for (j=0; j < NV; j++) delta[j] = 0.0, revisited[j] = 0;
    Qstart = 0;
    Qend = leaves;
    Qnext = Qend;
    for (j = Qstart; j < Qend; j++) revisited[Q2[j]] = 1;
  PushOnStack2:
    nedge_traversed += Qend - Qstart;
    for (j = Qstart; j < Qend; j++) {
      graphint w = Q2[j];
      graphint myStart = start[w];
      graphint myEnd = start[w+1];
      for (k = myStart; k < myEnd; k++) {
        graphint v = eV[k];
        if (dist[v] && dist[v] == dist[w]-1) {
	  fprintf(stderr, "%d,%d\t%g \n", v,w, sigma[v] * (1.0 + delta[w]) / (double) sigma[w]);
	  delta[v] += sigma[v] * (1.0 + delta[w]) / (double) sigma[w];
	}
        if (!revisited[v]++) Q2[Qnext++] = v;
      }
    }
    if (Qnext != Qend) {
      Qstart = Qend;
      Qend = Qnext;
      goto PushOnStack2;
    }
    for (j = 0; j < NV; j++) bc[j] += delta[j];
    //    for (j = 0; j < NV; j++) fprintf(stderr, "delta[%d]=%g\n", j, delta[j]);
  }
  
  free(Q);
  free(dist);
  free(sigma);
  free(QHead);
  free(marks);
  free(child_count);
  
  double bc_total = 0;
  for (graphint i=0; i<NV; i++) { bc_total += bc[i]; }
 
  *total_nedge = nedge_traversed;

  return bc_total/NV;
}
