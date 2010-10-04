#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include "graph.h"

int delete_graph(V* graph, int nVertices)
{ int i;

   for (i = 0; i < nVertices; i++)
     if (graph[i].my_neighbors) free(graph[i].my_neighbors);

   if (graph) free(graph);
   return 0;
}


int rand_graph(int n, int m, V** graph, E** list)
{ int i,j,v1,v2,r,**M,*D;
  double *rn;
  E *L;

  L  = (struct edge *) malloc(sizeof(struct edge) * m);
  rn = (double *) malloc((2 * m + 5) * sizeof(double));

  j = 2 * m + 5;
#ifndef SMALL_PROBLEM
  prand_(&j, rn);
#else 
   { int i;
     for (i=0;i< j; i++) {
     rn[i]=drand48();
     }
   }
#endif

#pragma mta assert nodep
  for (i = 0; i < m; i++) {
      int j  = 2 * i;
      int v1 = (int)(rn[j++] * n);
      int v2 = (int)(rn[j++] * n);
      while (v2 == v1) v2 = (int)(rn[j++] * n);
      L[i].v1 = v1;
      L[i].v2 = v2;
   } 

   printf(" number of edges got is %d\n",i);

   M = malloc(sizeof(int *) * n);
   D = malloc(sizeof(int) * n);

   for (i = 0; i < n; i++) D[i] = 0;
   for (i = 0; i < m; i++) {D[L[i].v1]++; D[L[i].v2]++;}
   for (i = 0; i < n; i++) M[i] = malloc(sizeof(int) * (D[i]));

   printf("finished allocating structures\n");

   if (list == NULL ) free (L); /* not interested in Edgelist */
   else               *list = L;

   if (graph == NULL) {
      printf("not interested in adjacency list\n");
      return(0);
   } else {

      for (i = 0; i < n; i++) D[i] = 0;

      for (i = 0; i < m; i++) {
          r = 0;
          for (j = 0; j < D[L[i].v1]; j++)
             if (L[i].v2 == M[L[i].v1][j]) r = 1;
          if (r==1) continue;
	  M[L[i].v1][D[L[i].v1]] = L[i].v2;
	  M[L[i].v2][D[L[i].v2]] = L[i].v1;
	  D[L[i].v1]++;
	  D[L[i].v2]++;
      }   

      printf("finished generating the matrix\n");

      *graph = malloc(sizeof(V)*n);

      for (i = 0; i < n; i++) {
          (*graph)[i].n_neighbors  = D[i];
          (*graph)[i].my_neighbors = malloc(sizeof(int) * D[i]);
          for (j = 0; j < D[i]; j++) (*graph)[i].my_neighbors[j] = M[i][j];
   }  }

   for (i = 0; i < n; i++) free(M[i]);
   free(M); free(D);  return(0); 
}
