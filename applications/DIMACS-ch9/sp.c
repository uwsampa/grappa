#include "sp.h"

double NSSSP(graph* G, int s, int weight_type, int trial, int alg) {

	int graph_type;
    int i, maxWt;
    double *realWt;
    int *intWt;
    double time;
    double checksum;
	preprocess_graph(G, &graph_type);

    if (trial == 0) {
        if (weight_type == 2) {
       
            int m = G->m;
            realWt = (double *) malloc(m*sizeof(double));
            intWt = G->intWt;
            maxWt = 0;
            /* get max int weight */
            for (i=0; i<m; i++) {
                if (intWt[i] > maxWt)
                    maxWt = intWt[i];
            }
            fprintf(stderr, "Max. edge weight: %d\n", maxWt);
            #pragma mta assert nodep 
            for (i=0; i<m; i++) {
                realWt[i] = ((double) intWt[i])/maxWt; 
            }
            free(intWt);
            G->realWt = realWt; 
        }
    }
    
    checksum = 0;    
    if (alg == 0){
        time = delta_stepping(G, s, &checksum);
        fprintf(stderr, "Delta stepping, checksum: %lf\n", checksum); 
    } else {
        time = BellmanFord(G, s, &checksum);
        fprintf(stderr, "Bellman Ford, checksum: %lf\n", checksum); 
    }
    
    return time;

}

double P2P(graph* G, int s, int t) {

	double time = STConnectivity(G, s, t);
    return time;
}
 
void preprocess_graph(graph* G, int* graph_type) {


}

