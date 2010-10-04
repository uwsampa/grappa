#ifndef _GRAPH_H_
#define _GRAPH_H_

typedef struct vertex{
   int self;
   int n_neighbors;
   int *my_neighbors;
} V; 

typedef struct edge {
  int v1, v2;
  int workspace; /* auxillary space*/
} E;
 
int rand_graph(int n, int m, V** graph, E** list);
int delete_graph(V* graph, int nVertices);

#endif /*_GRAPH_H_*/
