#ifndef _DEFS_H
#define _DEFS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include "sprng.h"

/* Uncomment this line, or use the flag -DDIAGNOSTIC for 
   iverbose benchmark output */
/* #define DIAGNOSTIC */

/* Uncomment this line, or use the flag -DVERIFYK4 to 
   generate a 2D torus */
/* #define VERIFYK4 */

#define INT_T int
#define DOUBLE_T double

#if defined(MASSIVE_GRAPH)
#define VERT_T long
#define LONG_T long
#elif defined(LARGE_GRAPH)
#define VERT_T int
#define LONG_T long
#else
#define VERT_T int
#define LONG_T int
#endif

#define WEIGHT_T VERT_T

#ifdef _OPENMP
    extern int NUM_THREADS;
#endif

/* Data structure for representing tuples
 * in the Scalable Data Generator */
typedef struct 
{
    /* Edge lists */
    VERT_T* startVertex;
    VERT_T* endVertex;
    WEIGHT_T* weight;

    /* No. of edges */
    LONG_T m;

    /* No. of vertices */
    LONG_T n;

} graphSDG;

/* The graph data structure*/
typedef struct
{ 
    LONG_T n;
    LONG_T m;

    /* Directed edges out of vertex vi (say, k edges -- v1, v2, ... vk) 
     * are stored in the contiguous block endV[numEdges[i] .. numEdges[i+1]]
     * So, numEdges[i+1] - numEdges[i] = k in this case */
    VERT_T* endV;
    LONG_T* numEdges;
    WEIGHT_T* weight;

} graph;

/* Edge data structure for Kernel 2 */
typedef struct
{
    VERT_T startVertex;
    VERT_T endVertex;
    WEIGHT_T w;
    LONG_T e;
} edge;

/* Predecessor list data structure for Kernel 4 */
typedef struct
{
    VERT_T* list;
    VERT_T count;
    LONG_T degree;
} plist;


/* Global variables */
extern INT_T SCALE;
extern LONG_T N;
extern LONG_T M;

/* R-MAT (graph generator) parameters */
extern DOUBLE_T A;
extern DOUBLE_T B;
extern DOUBLE_T C;
extern DOUBLE_T D;

extern WEIGHT_T MaxIntWeight;
extern INT_T SubGraphPathLength;
extern INT_T K4approx;

/* Function declarations */

void init(int);
double genScalData(graphSDG*);
double gen2DTorus(graphSDG*);

/* The four kernels */
double computeGraph(graph*, graphSDG*);
double getStartLists(graph*, edge**, INT_T*);
double findSubGraphs(graph*, edge*, INT_T);
double betweennessCentrality(graph*, DOUBLE_T *);

/* other useful routines */
void prefix_sums(LONG_T*, LONG_T*, LONG_T*, LONG_T);
DOUBLE_T get_seconds(void);
#endif
