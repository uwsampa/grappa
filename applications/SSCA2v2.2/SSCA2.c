#include "defs.h"

#ifdef _OPENMP
int NUM_THREADS;
#endif

int main(int argc, char** argv)
{
    /* Data structure for storing generated tuples in the 
     * Scalable Data Generation Stage -- see defs.h */
    graphSDG* SDGdata;  

    /* The graph data structure -- see defs.h */
    graph* G;
  
    /* Kernel 2 output */
    edge *maxIntWtList;
    INT_T maxIntWtListSize;

    /* Kernel 4 output */
    DOUBLE_T *BC;
    
    DOUBLE_T elapsed_time;

#ifdef _OPENMP
    if (argc != 3) {
        
        fprintf(stderr, "Usage: ./SSCA2 <No. of threads> <SCALE>\n");
        exit(-1);
    }
    NUM_THREADS = atoi(argv[1]);
    SCALE = atoi(argv[2]);
    omp_set_num_threads(NUM_THREADS);
#else
    if (argc != 2) {
        fprintf(stderr, "Usage: ./SSCA2 <SCALE>\n");
        exit(-1);
    }
    SCALE = atoi(argv[1]);
#endif

    /* ------------------------------------ */
    /*  Initialization -- Untimed           */
    /* ------------------------------------ */

    fprintf(stderr, "\nHPCS SSCA Graph Analysis Benchmark v2.2\n");
    fprintf(stderr, "Running...\n\n");

    init(SCALE);

#ifdef _OPENMP
    fprintf(stderr, "\nNo. of threads: %d\n", NUM_THREADS);
#endif
    fprintf(stderr, "SCALE: %d\n\n", SCALE);
 
    /* -------------------------------------------- */
    /*  Scalable Data Generator -- Untimed          */
    /* -------------------------------------------- */

#ifndef VERIFYK4
    fprintf(stderr, "Scalable Data Generator -- ");
    fprintf(stderr, "genScalData() beginning execution...\n");

    SDGdata  = (graphSDG *) malloc(sizeof(graphSDG));
    elapsed_time = genScalData(SDGdata);

    fprintf(stderr, "\n\tgenScalData() completed execution\n");
    fprintf(stderr, 
            "\nTime taken for Scalable Data Generation is %9.6lf sec.\n\n", 
            elapsed_time);
#else
    fprintf(stderr, "Generating 2D torus for Kernel 4 validation -- ");
    fprintf(stderr, "gen2DTorus() beginning execution...\n");

    SDGdata = (graphSDG *) malloc(sizeof(graphSDG));
    elapsed_time = gen2DTorus(SDGdata);

    fprintf(stderr, "\n\tgen2DTorus() completed execution\n");
    fprintf(stderr, 
            "\nTime taken for 2D torus generation is %9.6lf sec.\n\n", 
            elapsed_time);
#endif
    
    
    /* ------------------------------------ */
    /*  Kernel 1 - Graph Construction       */
    /* ------------------------------------ */
  
    /* From the SDG data, construct the graph 'G'  */
    fprintf(stderr, "\nKernel 1 -- computeGraph() beginning execution...\n");

    G = (graph *) malloc(sizeof(graph));
    /* Store the SDG edge lists in a compact representation 
     * which isn't modified in subsequent Kernels */
    elapsed_time = computeGraph(G, SDGdata);

    fprintf(stderr, "\n\tcomputeGraph() completed execution\n");
    fprintf(stderr, "\nTime taken for Kernel 1 is %9.6lf sec.\n\n", 
            elapsed_time);
    
    free(SDGdata);

    /* ---------------------------------------------------- */
    /*  Kernel 2 - Find max edge weight                     */
    /* ---------------------------------------------------- */
  
    fprintf(stderr, "\nKernel 2 -- getStartLists() beginning execution...\n");
  
    /* Initialize vars and allocate temp. memory for the edge list */
    maxIntWtListSize = 0;
    maxIntWtList = (edge *) malloc(sizeof(edge));

    elapsed_time = getStartLists(G, &maxIntWtList, &maxIntWtListSize);

    fprintf(stderr, "\n\tgetStartLists() completed execution\n\n");
    fprintf(stderr, "Max. int wt. list size is %d\n", maxIntWtListSize);
    fprintf(stderr, "\nTime taken for Kernel 2 is %9.6lf sec.\n\n", 
            elapsed_time);

    /* ------------------------------------ */
    /*  Kernel 3 - Graph Extraction         */
    /* ------------------------------------ */
  
    fprintf(stderr, "\nKernel 3 -- findSubGraphs() beginning execution...\n");

    elapsed_time = findSubGraphs(G, maxIntWtList, maxIntWtListSize);

    fprintf(stderr, "\n\tfindSubGraphs() completed execution\n");
    fprintf(stderr, "\nTime taken for Kernel 3 is %9.6lf sec.\n\n", 
            elapsed_time);
     
    free(maxIntWtList);
       
    /* ------------------------------------------ */
    /*  Kernel 4 - Betweenness Centrality         */
    /* ------------------------------------------ */
  
    fprintf(stderr, "\nKernel 4 -- betweennessCentrality() "
            "beginning execution...\n");  
    
    BC = (DOUBLE_T *) calloc(N, sizeof(DOUBLE_T));
    elapsed_time = betweennessCentrality(G, BC);
    fprintf(stderr, "\nTime taken for Kernel 4 is %9.6f sec.\n\n", 
            elapsed_time);
    fprintf(stderr, "TEPS score for Kernel 4 is %lf\n\n", 
            7*N*(1<<K4approx)/elapsed_time);
    free(BC);
    
    /* -------------------------------------------------------------------- */
    
    free(G->numEdges);
    free(G->endV);
    free(G->weight);
    free(G);

    return 0;
}
