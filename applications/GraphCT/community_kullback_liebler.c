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

#if !defined(NAN)
#define NAN (0.0/0.0)
#endif

#ifdef __MTA__
double Log2(double n)
{
	return log(n) / log(2);
}
#endif

double
get_community_kl_dir(const graph *G, const int64_t *membership,
			     int64_t num_components)
{
     int64_t u;
     int64_t n, m;
     double m_inv;
     double kldiv;
     int64_t *Lss, *Lsplus, *Lpluss;

     n = G->numVertices;
     m = G->numEdges;
     m_inv = 1.0/(G->numEdges);

     Lss = xcalloc (3*num_components, sizeof (*Lsplus));
     Lsplus = &Lss[num_components];
     Lpluss = &Lsplus[num_components];

     OMP("omp parallel for")
	  for(u=0; u<n; u++) {
	       const int64_t cid = membership[u];
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end   = G->edgeStart[u+1];
	       int64_t lss = 0, lsplus = 0;

	       if (cid >= 0) {
		    int64_t j;
		    for (j=u_edge_start; j<u_edge_end; j++) {
			 const int64_t v = G->endVertex[j];
			 const int64_t vcid = membership[v];
			 if (vcid == cid)
			      ++lss;
			 else {
			      ++lsplus;
			      if (vcid >= 0)
				   OMP("omp atomic") ++Lpluss[vcid];
			 }
		    }
		    OMP("omp atomic") Lss[cid] += lss;
		    OMP("omp atomic") Lsplus[cid] += lsplus;
	       }
	  }

     kldiv = 0.0;
     {
	  int64_t j;
	  for (j = 0; j < num_components; j++) {
	       double p, q;
	       p = Lss[j] * m_inv;
	       q = Lsplus[j] * m_inv;
	       q *= q;
               if (p != q)
#ifdef __MTA__
				kldiv += p * Log2 (p / q);
#else
	            kldiv += p * log2 (p / q);
#endif
	  }
     }

     free(Lss);

     return kldiv;
}

double
get_community_kl_undir(const graph *G, const int64_t *membership,
			       int64_t num_components)
{
     int64_t u;
     int64_t n, m;
     double m_inv;
     double kldiv;
     int64_t *Lss, *Lsplus;

     n = G->numVertices;
     m = G->numEdges;
     m_inv = 1.0/(G->numEdges);

     Lss = xcalloc (2*num_components, sizeof (*Lsplus));
     Lsplus = &Lss[num_components];

     OMP("omp parallel for")
	  for(u=0; u<n; u++) {
	       const int64_t cid = membership[u];
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end   = G->edgeStart[u+1];
	       int64_t lss = 0, lsplus = 0;

	       if (cid >= 0) {
		    int64_t j;
		    for (j=u_edge_start; j<u_edge_end; j++) {
			 const int64_t v = G->endVertex[j];
			 const int64_t vcid = membership[v];
			 /* Examine only the upper triangle and diagonal. */
			 if (v < u) continue;
			 if (vcid == cid)
			      ++lss;
			 else
			      ++lsplus;
		    }
		    OMP("omp atomic") Lss[cid] += lss;
		    OMP("omp atomic") Lsplus[cid] += lsplus;
	       }
	  }

     kldiv = 0.0;
     {
	  int64_t j;
	  for (j = 0; j < num_components; j++) {
	       double p, q;
	       p = Lss[j] * m_inv;
	       q = 0.5 * Lsplus[j] * m_inv;
	       q *= q;
               if (p != q)
#ifdef __MTA__
				kldiv += p * Log2 (p / q);
#else
	            kldiv += p * log2 (p / q);
#endif
	  }
     }

     free(Lss);

     return kldiv;
}

/** Compute the Kullback-Liebler divergence of the components induced
  by the membership array relative to a uniform base model.  The
  component is induced by the decomposition in the membership array.

  The mechanics of the modularity computations upon which these models
  are based are described in McCloskey and Bader, "Modularity and
  Graph Algorithms", presented at UMBC Sept. 2009.

  @param g A graph_t, either directed or undirected.  The computations
    differ for directed v. undirected graphs.
  @param membership An array mapping each vertex in g to its component.
  @param num_components The number of components, at least as large as
    the largest number in membership.

  @return The Kullback-Liebler divergence from a uniform base model,
  or NaN if the arguments are invalid.
*/
double
get_community_kl(const graph *G, const int64_t *membership,
			 int64_t num_components)
{
     if (!G || !membership || num_components <= 0) return NAN;
     if (!G->numVertices || !G->numEdges) return 0.0;
     if (G->undirected)
	  return get_community_kl_undir (G, membership, num_components);
     else
	  return get_community_kl_dir (G, membership, num_components);
}

double
get_single_community_kl_undir(const graph *G, const int64_t *membership,
				      int64_t which_component)
{
     int64_t n, m;
     double kldiv, p, q;
     int64_t Ls = 0, Vols = 0, Xs, u;

     n = G->numVertices;
     m = G->numEdges/2;

     OMP("omp parallel for reduction(+:Ls) reduction(+:Vols)")
	  for(u=0; u<n; u++) {
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end   = G->edgeStart[u+1];
	       const int64_t u_degree     = u_edge_end - u_edge_start;
	       int64_t j;

	       if (membership[u] == which_component) {
		    for(j=u_edge_start; j<u_edge_end; j++) {
			 const int64_t v = G->endVertex[j];
			 if (membership[v] == which_component)
			      ++Ls; /* double-counted */
		    }
		    Vols += u_degree;
	       }
	  }

     assert (!(Ls % 2));
     Ls /= 2;
     Xs = Vols - Ls;
     p = Ls/(double)m;
     q = 0.5 * Xs / (double)m;
     q *= q;
#ifdef __MTA__
	 kldiv = p * Log2 (p / q);
#else
     kldiv = p * log2 (p / q);
#endif

     return kldiv;
}

double
get_single_community_kl_dir(const graph *G, const int64_t *membership,
				    int64_t which_component)
{
     int64_t n, m;
     double kldiv, p, q;
     int64_t Ls = 0, Vols = 0, Xs, u;

     n = G->numVertices;
     m = G->numEdges;

     OMP("omp parallel for reduction(+:Ls) reduction(+:Vols)")
	  for(u=0; u<n; u++) {
	       const int64_t u_edge_start = G->edgeStart[u];
	       const int64_t u_edge_end   = G->edgeStart[u+1];
	       const int64_t u_degree     = u_edge_end - u_edge_start;
	       int64_t j;

	       if (membership[u] == which_component) {
		    for(j=u_edge_start; j<u_edge_end; j++) {
			 const int64_t v = G->endVertex[j];
			 if (membership[v] == which_component)
			      ++Ls; /* double-counted */
		    }
		    Vols += u_degree;
	       }
	  }

     assert (!(Ls % 2));
     Ls /= 2;
     Xs = Vols - Ls;
     p = Ls / (double)m;
     q = Xs / (double)m;
     q *= q;
     kldiv = p * log (p / q);

     return kldiv;
}

/** Compute the Kullback-Liebler divergence of a single induced
  component relative to a uniform base model.  The component is
  induced by the decomposition in the membership array.  The
  get_community_kl() routine is more efficient than computing each KL
  contribution separately.

  The mechanics of the modularity computations upon which these models
  are based are described in McCloskey and Bader, "Modularity and
  Graph Algorithms", presented at UMBC Sept. 2009.

  @param g A graph_t, either directed or undirected.  The computations
    differ for directed v. undirected graphs.
  @param membership An array mapping each vertex in g to its component.
  @param which_component The component of interest.

  @return The Kullback-Liebler divergence from a uniform base model,
  or NaN if the arguments are invalid.
*/
double
get_single_community_kl(const graph *G, const int64_t *membership,
				int64_t which_component)
{
     if (!G || !membership) return NAN;
     if (G->undirected)
	  return get_single_community_kl_undir (G, membership, which_component);
     else
	  return get_single_community_kl_dir (G, membership, which_component);
}
