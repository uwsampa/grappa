#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "defs.h"
#include "globals.h"
#include <sys/mta_task.h>
#include <machine/runtime.h>
#define MTA(x) _Pragma(x)

/* Set this variable to 1 to time each iteration */
#define TIMEBCPART 0

/* Edges whose weights are a multiple of 8 are filtered */
/* according to the SSCA#2 specification */
/* Set this to 0 to disable this edge filtering */
#define FILTEREDGES 0

double centrality(graph *G, double *BC, int Vs) {
  
  int num_srcs = Vs;  
  double timeBC, timep1, timep2, timer();
  
  int NE      = G->numEdges;
  int NV      = G->numVertices;
  int *sV     = G->startVertex;
  int *eV     = G->endVertex;
  int *start  = G->edgeStart;
  int *weight = G->intWeight;

  /* Allocate memory for data structures */
  int *Q     = (int *) malloc(NV * sizeof(int));
  int *dist  = (int *) malloc(NV * sizeof(int));
  int *sigma = (int *) malloc(NV * sizeof(int));
  int *marks = (int *) malloc((NV + 2) * sizeof(int));
  int *QHead = (int *) malloc(10 * SCALE * sizeof(int));
  int *child  = (int *) malloc(NE * sizeof(int));
  int *child_count = (int *) malloc(NV * sizeof(int));

  int j, k, s;
  int nQ, Qnext, Qstart, Qend;

  /* Reuse the dist memory in the accumulation phase */
  double *delta = (double *) dist;
  double avg_frac = 0.0; 
  MTA("mta use 100 streams")
  for (j = 0; j < NV; j++) BC[j] = 0.0;

  timeBC = timer();

/* Use |Vs| nodes to compute centrality values */
  for (s = 0; (s < NV) && (Vs > 0); s ++) {
 
      int d_phase;
      int num_visited;
      if (start[s+1] == start[s]) {
          continue; 
      } else {
          Vs --;
      }

#if TIMEBCPART      
      timep1 = timer();
      timep2 = timer();
#endif

#pragma mta assert nodep
MTA("mta use 100 streams")
      for (j = 0; j < NV; j++) {dist[j] = -1; 
          sigma[j] = marks[j] = child_count[j] = 0; }

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
#pragma mta assert no dependence
#pragma mta block dynamic schedule
MTA("mta use 100 streams")
      for (j = Qstart; j < Qend; j++) {
        int v = Q[j];
        int sigmav  = sigma[v];    
        int myStart = start[v];     
        int myEnd   = start[v+1];
        int ccount = 0;
        for (k = myStart; k < myEnd; k++) {
          int d, w, l;
#if FILTEREDGES
          if ((weight[k] % 8) == 0) continue;
#endif
          w = eV[k];                    
          d = dist[w];             
/* If node has not been visited, set distance and push on Q (but only once) */
          
          if (d < 0) {
             if (int_fetch_add(marks + w, 1) == 0) {
                dist[w] = d_phase; Q[int_fetch_add(&Qnext, 1)] = w;
                          
             }
             int_fetch_add(sigma + w, sigmav);
             l = myStart + ccount++;
             child[l] = w;
          } else if (d == d_phase) {
             int_fetch_add(sigma + w, sigmav);
             l = myStart + ccount++;
             child[l] = w;
          }
        }
        child_count[v] = ccount;
      }

/* If new nodes pushed onto Q */
      if (Qnext != QHead[nQ]) 
        {nQ++; QHead[nQ] = Qnext; 
          goto PushOnStack;}

#if TIMEBCPART
      timep1 = timer() - timep1;
      fprintf(stderr, "Traversal time: %9.6lf sec, visited: %d\n", timep1, QHead[nQ]);
      timep1 = timer();
#endif

#if 0
      /* Code snippet to count the size of child multiset */
      int sum = 0;
      for (j = 0; j < NV; j++) {
          sum = sum + child_count[j];
      }
      avg_frac += (double)sum/NE;
      fprintf(stderr, "child count: %d, fraction: %lf, visited: %d\n", sum,
              (double) sum/NE, QHead[nQ]);
#endif

      /* Dependence accumulation phase */
      nQ--;
MTA("mta use 100 streams")
      for (j=0; j<NV; j++) delta[j] = 0.0;

/* Pop nodes off of Q in the reverse order they were pushed on */
      for ( ; nQ > 1; nQ --) {
          Qstart = QHead[nQ-1];
          Qend   = QHead[nQ];
  /* For each v in the sublist AND for each w on v's list */
#pragma mta assert parallel
#pragma mta block dynamic schedule
#pragma mta assert no alias *sigma *Q *BC *delta *child *start *QHead
MTA("mta use 100 streams")
          for (j = Qstart; j < Qend; j++) {
              int v = Q[j]; 
              int myStart = start[v];
              int myEnd   = myStart + child_count[v];
              double sum = 0;
              double sigma_v = (double) sigma[v];
              for (k = myStart; k < myEnd; k++) {
                 int w = child[k];
                 sum += sigma_v * (1.0 + delta[w]) / (double) sigma[w];
              } 
              delta[v] = sum; 
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

  free(Q); free(dist); free(sigma); free(QHead);
  free(marks); free(child); free(child_count);

  return timeBC;
}
