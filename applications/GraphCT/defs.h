#ifndef _DEFS_H
#define _DEFS_H

#include <stdlib.h>
#include <stdint.h>

#if defined(__MTA__)
#define int64_fetch_add int_fetch_add
#define MTA(x) _Pragma(x)
#if defined(MTA_STREAMS)
#define MTASTREAMS() MTA(MTA_STREAMS)
#else
#define MTASTREAMS() MTA("mta use 100 streams")
#endif
#else
#define MTA(x)
#define MTASTREAMS()
#endif

#if defined(_OPENMP)
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

/* helps some debugging tools */
#if defined(__GNUC__)
#define FNATTR_MALLOC __attribute__((malloc))
#define UNLIKELY(x) __builtin_expect((x),0)
#define LIKELY(x) __builtin_expect((x),1)
#else
#define FNATTR_MALLOC
#define UNLIKELY(x) x
#define LIKELY(x) x
#endif


typedef struct
{
	int64_t cmdsingle;
	int64_t cmdscript;
	char *infilename;
	char *outfilename;
	int64_t graph_type;
	int64_t kernel_id;
	int64_t scale;
	int64_t parameter1;
	int64_t parameter2;
	int64_t parameter3;
        int64_t printstats;
} singleKernelOptions;

typedef struct 
{ int64_t numEdges;
  int64_t * startVertex;	/* sorted, primary key   */
  int64_t * endVertex;	/* sorted, secondary key */
  int64_t * intWeight;	/* integer weight        */ 
  size_t map_size;
} graphSDG;

typedef struct /* the graph data structure */
{ int64_t numEdges;
  int64_t numVertices;
  int64_t * startVertex;    /* start vertex of edge, sorted, primary key      */
  int64_t * endVertex;      /* end   vertex of edge, sorted, secondary key    */
  int64_t * intWeight;      /* integer weight                                 */
  int64_t * edgeStart;      /* index into startVertex and endVertex list      */
  int64_t * marks;		/* array for marking/coloring of vertices	  */
  size_t map_size;
  int64_t undirected;
} graph;


/* Graph Generators */

void free_graph(graph *);
void alloc_graph (graph *, int64_t /*NV*/, int64_t /*NE*/);
void free_edgelist(graphSDG *);
void alloc_edgelist (graphSDG *, int64_t /*NE*/);
void getUserParameters(int64_t, int64_t);
void genScalData(graphSDG*, double, double, double, double);
void computeGraph(graph*, graphSDG*);
graph * makeUndirected(graph*);
int64_t graphio_b(graph*, char*, int64_t);
graph * genSubGraph(graph * G, int64_t NV, int64_t color);
graph * kcore(graph * G, int64_t k);
graph * parse_DIMACS_graph(char*);
int64_t fwrite_doubles(double*, int64_t, char*);
int64_t fwrite_ints(int64_t*, int64_t, char*);
int64_t graphio_el(graph *G, char *filename);

/* Analysis kernels */

void getStartLists(graph*, int64_t*, int64_t*);
void findSubGraphs(int64_t, int64_t*, graph*);
double centrality(graph*, double*, int64_t);
double kcentrality(graph*, double*, int64_t, int64_t);
double computeModularityValue(graph*, int64_t*, int64_t);
void calculateDegreeDistributions(graph*);
double computeConductanceValue(graph*, int64_t*);
int64_t connectedComponents(graph*);
int64_t calculateGraphDiameter(graph*, int64_t);
int64_t * calculateBFS(graph*, int64_t, int64_t);
void calculateComponentDistributions(graph*, int64_t, int64_t*, int64_t*);
double calculateTransitivityGlobal(graph* G);
double * calculateClusteringLocal(graph* G);
double * calculateTransitivityLocal(graph* G);
void agglomerative_partitioning (graph*, double*, int64_t);


/* Community Metrics */
double get_community_modularity_dir(const graph*, const int64_t*, int64_t);
double get_community_modularity_undir(const graph*, const int64_t*, int64_t);
double get_community_modularity(const graph*, const int64_t*, int64_t);
double get_single_community_modularity_undir(const graph*, const int64_t*, int64_t);
double get_single_community_modularity_dir(const graph*, const int64_t*, int64_t);
double get_single_community_modularity(const graph*, const int64_t*, int64_t);
double get_single_community_clustering_coefficient (const graph*, const int64_t*, int64_t);
double get_community_kl_dir(const graph*, const int64_t*, int64_t);
double get_community_kl_undir(const graph*, const int64_t*, int64_t);
double get_community_kl(const graph*, const int64_t*, int64_t);
double get_single_community_kl_undir(const graph*, const int64_t*, int64_t);
double get_single_community_kl_dir(const graph*, const int64_t*, int64_t);
double get_single_community_kl(const graph*, const int64_t*, int64_t);
double get_single_community_conductance(const graph*, const int64_t*, int64_t);







/* Utility functions */

void parseCommandLineOptions (int64_t, char**, singleKernelOptions*);
void SortStart(int64_t, int64_t, int64_t*, int64_t*, int64_t*, int64_t*, int64_t*, int64_t*, int64_t*);
void SortStart2(int64_t NV, int64_t NE, int64_t *sv1, int64_t *ev1, int64_t *ev2, int64_t *start);
int compare(const void * a, const void * b);
int64_t count_intersect(int64_t * start, int64_t * eV, int64_t * start2, int64_t * eV2, int64_t u, int64_t v);
int64_t dwrite(double*, int64_t, int64_t, int64_t, char*);
size_t count_intersections (const int64_t ai, const size_t alen, int64_t * a,
	const int64_t bi, const size_t blen, int64_t * b);
int64_t count_triangles (const size_t nv, int64_t * off,
	int64_t * ind, int64_t i);
void count_all_triangles (const size_t nv, int64_t * off,
	int64_t * ind, int64_t * restrict ntri);
void SortGraphEdgeLists(graph *G);
void graphCheck(graph *G);

void *xmalloc (size_t);
void *xcalloc (size_t, size_t);
void *xrealloc (void *, size_t);
void *xmmap (void *, size_t, int64_t, int64_t, int64_t, off_t);
void * xmmap_alloc (size_t sz);
void xmunmap (void * p, size_t sz);

double timer (void);
void stats_tic (char *);
void stats_toc (void);
void print_stats ();

#endif
