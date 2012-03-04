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

/** Compute the clustering coefficient for a single community.

  @param g A graph_t, either directed or undirected.  The computations
    differ for directed v. undirected graphs.
  @param membership An array mapping each vertex in g to its component.
  @param community The community of interest.

  @return Clustering coefficient.  The extreme cases without any
    triangles or open triples returns 0.
*/
double
get_single_community_clustering_coefficient (const graph *G,
					     const int64_t * membership,
					     int64_t community)
{
     //fprintf(stderr, "in get clustering coefficients\n");
     int64_t u;

     long numtri = 0;
     long numopen = 0;
     double retval;

     OMP("omp parallel for reduction(+:numopen) reduction(+:numtri)")
	  for(u=0; u<G->numVertices; u++) {
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end = G->edgeStart[u+1];
	       int64_t u_degree = 0, j;

	       if(membership[u] != community) continue;

	       for(j=u_edge_start; j<u_edge_end; j++)
	       {
		    const int64_t v = G->endVertex[j];
		    if(membership[v] == community)
			 u_degree++;
	       }
	       for(j=u_edge_start; j<u_edge_end; j++)
	       {
		    const int64_t v = G->endVertex[j];
		    if(membership[v] == community)
		    {
			 const int64_t v_edge_start = G->edgeStart[v];
			 const int64_t v_edge_end = G->edgeStart[v+1];
			 int64_t v_degree = 0;
			 int64_t k = u_edge_start;
			 int64_t l = v_edge_start;
			 int64_t nt = 0;

			 while(k < u_edge_end && l < v_edge_end)
			 {
			      const int64_t w = G->endVertex[k];
			      const int64_t x = G->endVertex[l];
			      if(membership[w] == community) v_degree++;
			      if(membership[w] == community && w == x) nt++;
			      if(w == x)
			      {
				   k++;
				   l++;
			      }
			      else if(w > x) l++;
			      else k++;
			 }
			 numopen += v_degree;
			 numtri += nt;
		    }
	       }
	  }

     assert (numopen >= 0);
     assert (numtri >= 0);
     if (numopen) {
	  retval = numtri/(double)numopen;
     } else if (numtri)
	  retval = HUGE_VAL;
     else
	  retval = 0.0;
     assert (retval >= 0.0);
     return retval;
}
