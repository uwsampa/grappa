#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "defs.h"
#include "globals.h"
#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#else
#include "compat/xmt-ops.h"
#endif
#include <math.h>
#include <sys/mman.h>

/** Compute the conductance of a single community.  Note that the
  definition is identical for directed and undirected graphs.

  @param g A graph_t, either directed or undirected.  The computations
    differ for directed v. undirected graphs.
  @param membership An array mapping each vertex in g to its component.
  @param component The component of interest.

  @return Conductance, a volume-weighted cutsize measure.  The extreme
    cases, where the community induces no cut, return HUGE_VAL.
*/
double 
get_single_community_conductance(const graph *G, const int64_t * membership,
				 int64_t component)
{
     int64_t u;
     int64_t degree_in = 0, degree_out = 0;
     int64_t cross_edges = 0;

     OMP("omp parallel for reduction(+:degree_in) reduction(+:degree_out) \
	 reduction(+:cross_edges)")
	  for(u=0; u<G->numVertices; u++) {
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end = G->edgeStart[u+1];
	       const int64_t u_degree = u_edge_end - u_edge_start;

	       if(membership[u] == component)
	       {
		    int64_t j;
		    degree_in += u_degree;	

		    for(j=u_edge_start; j<u_edge_end; j++)
		    {
			 int64_t v = G->endVertex[j];
			 if(membership[v] != component)
			      cross_edges++;
		    }
	       }
	       else
	       {
		    degree_out += u_degree;	
	       }
	  }

     if(degree_in < degree_out)
     {
	  if (cross_edges)
	       return cross_edges / (double) degree_in;
	  else return HUGE_VAL;
     }
     else 
     {
	  if (cross_edges)
	       return cross_edges / (double) degree_out;
	  else return HUGE_VAL;
     }
}
