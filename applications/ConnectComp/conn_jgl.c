#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include <sys/mta_task.h>
#include <machine/mtaops.h>
#include <machine/runtime.h>
#include "graph.h"

#define min_label(x, y) ((label [x] < label [y]) ? label [x] : label [y]); 
#define max_label(x, y) ((label [x] < label [y]) ? label [y] : label [x]); 

extern double phan1, phan2;
extern double time1, time2, time3;

double timer()
{ return((double)mta_get_clock(0) / mta_clock_freq()); }

double phantoms()
{ double p  = (double) mta_get_num_teams();
  double ph = (double) mta_get_task_counter(RT_PHANTOM);

//  return ( (ph / p) / mta_clock_freq() );
  return ( (ph / p) / 2E9);
}


int conn_comp_SV ( E* El, int n_vertices, int n_edges )
{ int edge, vertex, greatest_representative, change, n_components, *label;

  time1 = timer();
  phan1 = phantoms();

  change        = 1;
  n_components  = 0;
  label         = malloc(sizeof(int) * n_vertices);

  /* "Label" is used to identify connected subgraphs.  Each vertex always 
     has as its label, the index (from 0 to n_vertices) of the least numbered
     vertex in a connected subgraph that (1) contains both the subject vertex 
     and the vertex identified by label and (2) was identified as a connected
     subgraph during the previous outer iteration of the algorithm.  Labels are 
     non-increasing and subgraphs are non-decreasing in size throughout
     the algorithm.  */

  /* Initialize each vertex to be a subcomponent consisting only of itself. */

  for (vertex = 0; vertex < n_vertices; vertex++) label [vertex] = vertex;

  time2 = timer();

  /* Each iteration through the following while loop merges subcomponents 
     into larger components as connections between components are discovered.
     At the beginning of each iteration, the graph is partitioned into
     (non-maximal) connected subgraphs.  All vertices in a given connected 
     subgraph share the same label, the index of the least numbered vertex 
     in that subgraph.  Thus, each such subgraph is uniquely identified by 
     the one vertex in it for which the "fixed point" property
           label [vertex] = label [ label [vertex] ]
     holds.
  */

  while (change) {

    change = 0;

#pragma mta assert parallel
    for (edge = 0; edge < n_edges; edge++) {
        int u, v; 

	/* Find the vertices  u  and  v  representing the subgraphs
           in which the endpoints of the edge (v1 and v2) lie.  If
           v1 and v2 lie in what are presently identified as separate
           subgraphs, one of these representatives will have a larger
           label than the other.  If so, merge that representative's
           subgraph into the lower indexed subgraph and note that a
	   merge has been found.
	*/

	u = min_label( El[edge].v1, El[edge].v2 );
	v = max_label( El[edge].v1, El[edge].v2 );
	if ( u < v ) {label [v] = u; change = 1;}
     }

    if ( change ) 
      {
#pragma mta assert parallel
    for (vertex = 0; vertex < n_vertices; vertex++) 

      /* Adjust the labels on non-representative vertices in connected 
         subgraphs that were absorbed into other connected subgraphs in the 
         previous step; the labels are reset to identify the index, now lower,
         of the least numbered vertex in the combined subgraphs.  In
         analyzing the parallel indeterminancy of this loop, note that at
         the beginning of this for loop, only the vertices that represent
         the combined subgraphs satisfy the fixed point property.  The
	 same will necessarily be true at the completion of this loop.
      */ 
        while (label [vertex] != label [ label [vertex] ] ) 
           label [vertex] = label [ label [vertex] ];
     }
 }

  time3 = timer();
  phan2 = phantoms();
  greatest_representative = -1;
  for (vertex = 0; vertex < n_vertices; vertex++) 
      if (label [vertex] > greatest_representative) 
         {n_components++; greatest_representative = label [vertex];}

  return (n_components);
}
