#include "graph.h"

void loadGraph(graph* G, int weight_type, int n, int m, 
	int* srcs, int* dests, int* int_weights, double* real_weights, int
    undirected) {

	int i, j, u, vp;
	int* degree;
	int *numEdges, *endV;
	RandomValue* tmp;
	double *realWt;
	int *intWt;

	G->n = n;
    if (undirected)
	    G->m = 2*m;
    else
        G->m = m;

	G->weight_type = weight_type;

	degree = (int *) calloc(n, sizeof(int));

	#pragma mta trace "loop 2 start"
	for (i=0; i<m; i++) {
#ifdef __MTA__
		int_fetch_add(degree+srcs[i], 1);
#else
		degree[srcs[i]]++;
#endif

        if (undirected) {
#ifdef __MTA__
		    int_fetch_add(degree+dests[i], 1);
#else
		    degree[dests[i]]++;
#endif
        }
	}

	#pragma mta trace "numEdges malloc start" 
	numEdges = (int *) malloc((n+2)*sizeof(int));
	numEdges[0] = 0;
	#pragma mta trace "numEdges malloc end"

	#pragma mta block schedule	
	for (i=1; i<=n; i++) {
		numEdges[i] = degree[i-1];
	}

	for (i=1; i<=n; i++) {
		numEdges[i] = numEdges[i] + numEdges[i-1];
	}
	numEdges[n+1] = numEdges[n];
    if (undirected) 
	    endV = (int *) malloc(2*m*sizeof(int));
    else
	    endV = (int *) malloc(m*sizeof(int));

    if (undirected) {
        if (weight_type == 1) {
            realWt = (double *) malloc(2*m*sizeof(double));
        } 

        if (weight_type == 2) {
            intWt = (int *) malloc(2*m*sizeof(int));
        }
    } else {
        if (weight_type == 1) {
            realWt = (double *) malloc(m*sizeof(double));
        }

        if (weight_type == 2) {
            intWt = (int *) malloc(m*sizeof(int));
        }

    }

    switch (weight_type) {

        case(0):

#ifdef __MTA__
#pragma mta assert nodep
#endif
            for (i=0; i<m; i++) {
                u = srcs[i];
#ifdef __MTA__
                vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                vp = numEdges[u] + (degree[u]--) - 1;
#endif
                endV[vp] = dests[i];

                if (undirected) {
                    u = dests[i];
#ifdef __MTA__
                    vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                    vp = numEdges[u] + (degree[u]--) - 1;
#endif
                    endV[vp] = srcs[i];
                } 
            }
            free(real_weights);
            free(int_weights);
            break;

        case(1):
#pragma mta trace "loop start"
#ifdef __MTA__
#pragma mta assert nodep
#endif
            for (i=0; i<m; i++) {
                u = srcs[i];
#ifdef __MTA__
                vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                vp = numEdges[u] + (degree[u]--) - 1;
#endif
                endV[vp] = dests[i];
                realWt[vp] = real_weights[i];

                if (undirected) {
                    u = dests[i];
#ifdef __MTA__
                    vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                    vp = numEdges[u] + (degree[u]--) - 1;
#endif
                    endV[vp] = srcs[i];
                    realWt[vp] = real_weights[i];
                } 
            }
#pragma mta trace "loop end"
            free(real_weights);
            free(int_weights);
            break;

        case(2):

#ifdef __MTA__
#pragma mta assert nodep 
#endif
            for (i=0; i<m; i++) {
                u = srcs[i];
#ifdef __MTA__
                vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                vp = numEdges[u] + (degree[u]--) - 1;
#endif
                endV[vp] = dests[i];
                intWt[vp] = int_weights[i];

                if (undirected) {
                    u = dests[i];
#ifdef __MTA__
                    vp = numEdges[u] + int_fetch_add(degree+u, -1) - 1;
#else
                    vp = numEdges[u] + (degree[u]--) - 1;
#endif
                    endV[vp] = srcs[i];
                    intWt[vp] = int_weights[i];
                }
            }
            free(real_weights);
            free(int_weights);
            break;

        default:
            break;

    }

    G->numEdges = numEdges;
    G->endV = endV;
    if (weight_type == 2)
        G->intWt = intWt;
    if (weight_type == 1)
        G->realWt = realWt;

    /* 
    for (i=0; i<m; i++) {
        fprintf(stderr, "[%d %d] ", srcs[i], dests[i]);
    }
    fprintf(stderr, "\n");


    for (i=0; i<G->n; i++) {

        for (j=G->numEdges[i]; j<G->numEdges[i+1]; j++) {

           if (weight_type == 1) 
               fprintf(stdout, "[%d %d %lf] ", i, G->endV[j], G->realWt[j]);
           if (weight_type == 2)
               fprintf(stderr, "[%d %d %d] ", i, G->endV[j], G->intWt[j]);
           if (weight_type == 0) 
               fprintf(stderr, "[%d %d] ", i, G->endV[j]);
        }
    }
    */			

    free(degree);

}

void random_Gnm(int n, int m, int weight_type, 
        int weight_dist, int max_weight, int** srcsPtr, int** destsPtr, int** int_weightsPtr,
        double** real_weightsPtr, int undirected) {

    int i;
    int *srcs, *dests;
    RandomValue *randVals1, *randVals2;
    int nrvals;

    srcs = (int *) malloc(m*sizeof(int));
    dests = (int *) malloc(m*sizeof(int));

    nrvals = m-n;
    randVals1 = (RandomValue *) malloc(nrvals*sizeof(RandomValue));
    randVals2 = (RandomValue *) malloc(nrvals*sizeof(RandomValue));

    giant_(&nrvals, randVals1);
    giant_(&nrvals, randVals2);

#ifdef __MTA__
#pragma mta assert nodep 
#endif
    for (i=0; i<n-1; i++) {
        srcs[i]  = i;
        dests[i] = i+1;	
    }

    srcs[n-1]  = n-1;
    dests[n-1] = 0;


#ifdef __MTA__
#pragma mta assert nodep
#endif
    for (i=n; i<m; i++) {
#ifdef __MTA__
        srcs[i]  = randVals1[i-n] % n;
        dests[i] = randVals2[i-n] % n;
#else
        srcs[i]  = randVals1[i-n] % ((unsigned int) n);
        dests[i] = randVals2[i-n] % ((unsigned int) n);
#endif
    }

    free(randVals1);
    free(randVals2);

#ifdef PERMUTE_NODES
    randPerm(n, m, srcs, dests);
#endif

    genEdgeWeights(m, weight_type, weight_dist, max_weight, int_weightsPtr,
            real_weightsPtr, undirected);

    *srcsPtr = srcs;
    *destsPtr = dests;	
}

void CraySF(int levels, unsigned int* details, int *nPtr, int *mPtr, 
        int weight_type, int weight_dist, int max_weight, int** srcsPtr, int** destsPtr, 
        int** int_weightsPtr, double** real_weightsPtr, int undirected) { 


    int i, j, k;
    int *srcs, *dests;
    int n, m;
    RandomValue *randVals;
    int nrvals;
    int e, n1, edgeCount;

    n = 0;
    m = 0;

#ifdef __MTA__
#pragma mta assert parallel
#endif
    for (i = 0; i < levels; i++) {
        if (details[2*i] > n) n = details[2*i];
        m += details[2*i + 1];
    }

    if (n <= 1 || m == 0) {
        fprintf(stderr, "\nbad args to CraySF graph generator\n");
    }

    *nPtr = n;
    *mPtr = m;

    srcs  = (int *) malloc(m*sizeof(int));
    dests = (int *) malloc(m*sizeof(int));

    edgeCount = 0;

    for (i = 0; i < levels; i++) {

        fprintf(stderr, "starting level %d\n", i);
        n1 = details[2*i];
        e = details[2*i + 1];

        randVals = (RandomValue *) malloc(e*sizeof(RandomValue));

        giant_(&e, randVals);

#ifdef __MTA__
#pragma mta assert parallel
#endif
        for (j = 0; j<e; j++) {
            k = edgeCount + j;
            srcs[k] = randVals[j] % n;
        }

        giant_(&e, randVals);


#ifdef __MTA__
#pragma mta assert parallel
#endif
        for (j = 0; j<e; j++) {
            k = edgeCount + j;
            dests[k] = randVals[j] % n;
        }

        edgeCount += e;
        free(randVals);
    }

#ifdef PERMUTE_NODES
    randPerm(n, m, srcs, dests);
#endif

    genEdgeWeights(m, weight_type, weight_dist, max_weight, int_weightsPtr,
            real_weightsPtr, undirected);

    *srcsPtr = srcs;
    *destsPtr = dests;
}

void RMatSF(int SCALE, int d, int weight_type, int weight_dist, int max_weight, 
        int** srcsPtr, int** destsPtr, int** int_weightsPtr, double**
        real_weightsPtr, int undirected) {

    int *srcs, *dests;
    int i, j, k, n, n1, m;
    RandomValue *rv, *rv_type;
    int randValSize, rv_index;
    int numEdgesPerPhase, numEdgesAdded, numEdgesToBeAdded;
    double *rv_p, p, a0, b0, c0, d0;

    n = 1<<SCALE;
    m = d*n;

    srcs = (int *) malloc(m*sizeof(int));
    dests = (int *) malloc(m*sizeof(int));

    /* First create a Hamiltonian cycle, so that we have a
       connected graph */

    rv_type = (RandomValue *) malloc(n*sizeof(RandomValue));
    giant_(&n, rv_type);

    n1 = n-1;
#ifdef __MTA__
#pragma mta assert nodep
#endif
    for (i=0; i<n1; i++) {
        srcs[i] = i;
        dests[i] = i+1;
    }

    srcs[n-1]  = n-1;
    dests[n-1] = n-1;

    free(rv_type);
    /* We need 13*SCALE random numbers for adding one edge */
    /* The edges are generated in phases, in order to be able to
       reproduce the same graph for a given SCALE value */

    /* We fill an array of random numbers of size 13*SCALE*numEdgesPerPhase
       in each phase */
#ifdef __MTA__
    numEdgesPerPhase = 1<<20;
#else
    numEdgesPerPhase = 1<<16;
#endif
    randValSize = numEdgesPerPhase*13*SCALE;
    rv = (RandomValue *) malloc(randValSize * sizeof(RandomValue));
    rv_type = (RandomValue *) malloc(numEdgesPerPhase * sizeof(RandomValue));
    rv_p = (double *) malloc(randValSize * sizeof(double));

    /* Start adding edges */
    a0 = 0.45; b0 = 0.15; c0 = 0.15; d0 = 0.25;

    numEdgesAdded = n;
    fprintf(stderr, "Adding Edges ... ");
    while (numEdgesAdded < m) {

        giant_(&randValSize, rv);

#ifdef __MTA__
#pragma mta assert nodep
#endif
        for (i=0; i<randValSize; i++)
            rv_p[i] = ((double) rv[i])/4294967295.0;

        giant_(&numEdgesPerPhase, rv_type);

        if (m - numEdgesAdded < numEdgesPerPhase) {
            numEdgesToBeAdded = m - numEdgesAdded;
        } else {
            numEdgesToBeAdded = numEdgesPerPhase;
        }

#ifdef __MTA__
#pragma mta assert parallel
#endif
        for (i=0; i<numEdgesToBeAdded; i++) {
            double a = a0; double b = b0; double c = c0; double d = d0;
            int u = 1;
            int v = 1;
            int step = n/2;

            for (j=0; j<SCALE; j++) {
                choosePartition(rv_p[13*(i*SCALE+j)], &u, &v, step, a, b, c, d);
                step = step/2;
                varyParams(rv_p, 13*(i*SCALE+j), &a, &b, &c, &d);
            }

            srcs[numEdgesAdded+i] = u-1;
            dests[numEdgesAdded+i] = v-1;

        }

        numEdgesAdded = numEdgesAdded + numEdgesToBeAdded;
        fprintf(stderr, "%d ", numEdgesAdded); 
    }
    fprintf(stderr, "\n");
#ifdef PERMUTE_NODES
    randPerm(n, m, srcs, dests);
#endif
    free(rv);
    free(rv_p);
    free(rv_type);

    genEdgeWeights(m, weight_type, weight_dist, max_weight, int_weightsPtr,
            real_weightsPtr, undirected);

    *srcsPtr = srcs;
    *destsPtr = dests;

}

void choosePartition(double p, int *u, int *v, int step, double a, double b, double c, double d) {

    if (p < a) {
        /* Do nothing */
    } else if ((p >= a) && (p < a+b)) {
        *v += step;
    } else if ((p >= a+b) && (p < a+b+c)) {
        *u += step;
    } else if ((p >= a+b+c) && (p < a+b+c+d)) {
        *u += step;
        *v += step;
    }

}


void varyParams(double* rv_p, int offset, double* a, double* b, double* c, double* d) {

    double v, S;

    /* Allow a max. of 10% variation */
    v = 0.10;
    if (rv_p[offset+1] > 0.5)
        *a += *a * v * rv_p[offset+5];
    else
        *a -= *a * v * rv_p[offset+6];
    if (rv_p[offset+2] > 0.5)
        *b += *b * v * rv_p[offset+7];
    else
        *b += *b * v * rv_p[offset+8];
    if (rv_p[offset+3] > 0.5)
        *c += *c * v * rv_p[offset+9];
    else
        *c -= *c * v * rv_p[offset+10];
    if (rv_p[offset+4] > 0.5)
        *d += *d * v * rv_p[offset+11];
    else
        *d -= *d * v * rv_p[offset+12];

    S = *a + *b + *c + *d;

    *a = *a/S;
    *b = *b/S;
    *c = *c/S;
    *d = *d/S;
}

void random_geometric(int SCALE, int d, int weight_type, int weight_dist, int max_weight, 
        int** srcs, int** dests, int** int_weights, double** real_weights, int
        undirected) {



}

void random_bullseye(int SCALE, int d, int orbits, int weight_type, int
        weight_dist, int max_weight, 
        int** srcs, int** dests, int** int_weights, double** real_weights, int
        undirected) {


}

void random_2Dtorus(int SCALE, int x_pow, int y_pow, int d, int *mPtr, int weight_type,
        int weight_dist, int max_weight, 
        int** srcs, int** dests, int** int_weights, double** real_weights, int
        undirected) {


}

void random_hierarchical(int SCALE, int d, int *mPtr, int weight_type, int
        weight_dist, int max_weight, 
        int** srcs, int** dests, int** int_weights, double** real_weights, int
        undirected) {


}

void grid(int x_pow, int y_pow, int *nPtr, int *mPtr, int weight_type,
        int weight_dist, int max_weight, 
        int** srcsPtr, int** destsPtr, int** int_weightsPtr, double**
        real_weightsPtr, int
        undirected) {

        int *srcs, *dests;
        short *types, *types2;
        int i, j, n, m, x, y, numEdges, numEdgesAdded, eid;

        assert(x_pow<33);
        assert(y_pow<33);

        x = 1<<x_pow;
        y = 1<<y_pow;

        n = x*y;
        m = 4*n-2*(x+y);
        *nPtr = n;
        *mPtr = m;
       
        srcs = (int *) malloc(m*sizeof(int));
        dests = (int *) malloc(m*sizeof(int));
        
        #pragma mta assert nodep
        for (i=1; i<x-1; i++) {
            // #pragma mta assert nodep
            for (j=1; j<y-1; j++) {
                   
                int eid = (j-1)*(x-2)+i-1;
                srcs[4*eid] = j*x+i;
                dests[4*eid] = j*x+i+1;

                srcs[4*eid+1] = j*x+i;
                dests[4*eid+1] = (j-1)*x+i;

                srcs[4*eid+2] = j*x+i;
                dests[4*eid+2] = (j+1)*x+i;

                srcs[4*eid+3] = j*x+i;
                dests[4*eid+3] = j*x+i-1;
            }
        }

        numEdgesAdded = 4*(x-2)*(y-2); 

        srcs[numEdgesAdded+1] = 0;
        dests[numEdgesAdded+1] = 1;

        srcs[numEdgesAdded+2] = 0;
        dests[numEdgesAdded+2] = y;

        srcs[numEdgesAdded+3] = x-1;
        dests[numEdgesAdded+3] = x-2;

        srcs[numEdgesAdded+4] = x-1;
        dests[numEdgesAdded+4] = 2*x-1;

        srcs[numEdgesAdded+5] = x*(y-1);
        dests[numEdgesAdded+5] = x*(y-1)+1;

        srcs[numEdgesAdded+6] = x*(y-1);
        dests[numEdgesAdded+6] = x*(y-2);

        srcs[numEdgesAdded+7] = x*(y-1)+x-1;
        dests[numEdgesAdded+7] = x*(y-1)+x-2;

        srcs[numEdgesAdded] = x*(y-1)+x-1;
        dests[numEdgesAdded] = (x-1)*(y-1)+x-1;
      
        numEdgesAdded = numEdgesAdded + 8;

        #pragma mta assert nodep
        for (i=1; i<x-1; i++) {
            eid = numEdgesAdded + 6*(i-1);
            srcs[eid] = i;
            dests[eid] = i+1;
    
            srcs[eid+1] = i;
            dests[eid+1] = i+x;

            srcs[eid+2] = i;
            dests[eid+2] = i-1;

            srcs[eid+3] = x*(y-1)+i;
            dests[eid+3] = x*(y-1)+i+1;

            srcs[eid+4] = x*(y-1)+i;
            dests[eid+4] = x*(y-1)+i-1;

            srcs[eid+5] = x*(y-1)+i;
            dests[eid+5] = x*(y-2)+i;
        }

        numEdgesAdded += 6*(x-2);

        #pragma mta assert nodep
        for (i=1; i<y-1; i++) {
            eid = numEdgesAdded + 6*(i-1);
            srcs[eid] = (i+1)*x-1;
            dests[eid] = i*x-1;
    
            srcs[eid+1] = (i+1)*x-1;
            dests[eid+1] = (i+1)*x-2;

            srcs[eid+2] = (i+1)*x-1;
            dests[eid+2] = (i+2)*x-1;

            srcs[eid+3] = i*x;
            dests[eid+3] = i*x+1;

            srcs[eid+4] = i*x;
            dests[eid+4] = (i-1)*x;

            srcs[eid+5] = i*x;
            dests[eid+5] = (i+1)*x;
        }

        numEdgesAdded += 6*(y-2);
        genEdgeWeights(m, weight_type, weight_dist, max_weight, int_weightsPtr,
            real_weightsPtr, undirected);

        *srcsPtr = srcs;
        *destsPtr = dests;
}


void readGraphFromFile(char* fn, int* sourcePtr, char *desc, int *nPtr, int *mPtr, int weight_type, int *max_weight, 
        int** srcsPtr, int** destsPtr, int** int_weightsPtr, double**
        real_weightsPtr, int
        undirected) {
#ifndef __MTA__
    int n, m, numChars, numLines, retVal, *weights;
    FILE *file;
    char *buf;
    int i, x, lineNum, edgesSeen;
    int *starts;
    int *srcs, *dests, source;
    struct stat info;
    int weight, index, arcsStart;
    int edges, from, to;

    if ((file = fopen(fn, "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", fn);
        exit(1);
    }
    
    buf = (char *) malloc(100*sizeof(char));

    while (fgets(buf, 100, file) != NULL)  {

        switch (buf[0]) {

            case 'c': /* skip */
            case '\0': 
                break;
            case 'p':
                sscanf(buf, "%*c %*2s %d %d", &n, &m);
                fprintf(stderr, "nodes: %d, edges: %d\n", n, m);
                srcs = (int *) malloc(m*sizeof(int));
                dests = (int *) malloc(m*sizeof(int));
                weights = (int *) malloc(m*sizeof(int));
                source = -1;            
                edges = 0;
                break;
            case 'a':
                sscanf(buf,"%*c %d %d %d", &from, &to, &weight);
                srcs[edges] = from;
                dests[edges] = to;
                weights[edges] = weight;
                edges++;
                break;
            case 't':
                break;
        }

    }
    
    if (desc == NULL) desc = "No description.";
    
    free(buf);
 
    *nPtr = n;
    *mPtr = m;
    *int_weightsPtr = weights;
    *real_weightsPtr = (double *) malloc(sizeof(double));
    *srcsPtr = srcs;
    *destsPtr = dests;
    *sourcePtr = source;
    fclose(file);
#else
    int n, m, numChars, numLines, retVal, *weights;
    FILE *file;
    char *buf;
    int i, x, lineNum, edgesSeen;
    int *starts;
    int *srcs, *dests, source;
    struct stat info;
    int index, arcsStart;

    if ((file = fopen(fn, "r")) == NULL) {
        fprintf(stderr, "Cannot open file %s\n", fn);
        exit(1);
    }

    stat(fn, &info);
    numChars = info.st_size;
    buf = (char *) malloc((numChars+1)*sizeof(char));
   
    /* read file */
    fread(buf,sizeof(char),numChars,file);

    // find number of newlines
    numLines = 0;
    for (i=0; i<numChars; i++) {
        if (buf[i] == '\n')
            numLines++;
    }
    
    starts = (int *) malloc(sizeof(int)*(numLines+1));
    starts[0] = 0;
    x=1;
    // find start of each line
    for (i=1; i<numChars; i++) {
        if (buf[i] == '\n')
            starts[x++] = i+1;
    }
    starts[numLines] = numChars;

    // look for the line with the problem size
    lineNum = 0;
    while (1) {
        if (buf[ starts[lineNum] ] == 'p')
            break;
        lineNum++;
    }

    // line looks like p sp <vertices> <edges>
    if (2 != (retVal = sscanf(buf+starts[lineNum],"%*c %*2s %d %d",&n,&m)))
        input_error("number of vertices and/or edges not supplied",lineNum);
 
    srcs  = (int *) malloc(m*sizeof(int));
    dests = (int *) malloc(m*sizeof(int));
    weights = (int *) malloc(m*sizeof(int));

    source = -1;
    edgesSeen = 0;
    arcsStart = -1;
    for (i=lineNum+1; i<numLines; i++) {

        //for (int j= starts[i]; j< starts[i+1]; j++) 
        //    printf("%c",buf[j]);
        switch(buf[starts[i]]) {
            // skip empty lines
            case 'c':
            case '\n':
            case '\0':
                break;

            // problem desciption
            case 't':
                desc = (char*)malloc(sizeof(char)*(starts[i+1] - starts[i]));
                strncpy(desc,buf+starts[i]+2,starts[i+1]-starts[i]-3);
                break;

            // source description
            case 'n':
                if (source != -1)
                    input_error("multiple source vertices specified",i);
                if (1 != sscanf(buf+starts[i],"%*c %d",&source))
                    input_error("source vertex id not supplied",i);
                if (source < 1 || source > n)
                    input_error("invalid source vertex id",i);
                //printf("source = %i\n",source);
                break;

            // edge (arc) description
            case 'a':
                arcsStart = i;
                break;
        }
        if (arcsStart != -1)
            break;
    } 

    #pragma mta assert nodep
    for (i=arcsStart; i<numLines; i++) {
        int from, to, weight;
        sscanf(buf+starts[i],"%*c %d %d %d",&from,&to,&weight);
        
        --from;
        --to;
        // printf("edge (%i,%i), %i\n",from,to,weight);
        index = i-arcsStart;
        srcs[index] = from;
        dests[index] = to;
        weights[index] = weight;
    }
    if (desc == NULL) desc = "No description.";
    
    free(starts);
    free(buf);
 
    *nPtr = n;
    *mPtr = m;
    *int_weightsPtr = weights;
    *real_weightsPtr = (double *) malloc(sizeof(double));
    *srcsPtr = srcs;
    *destsPtr = dests;
    *sourcePtr = source;
#endif
}

#pragma mta inline
void input_error(char *error_msg, int line) {

    fprintf(stderr,"line %i: %s\n",line,error_msg);
    exit(1);
}


void genEdgeWeights(int m, int weight_type, int weight_dist, int max_weight,
        int** int_weightsPtr, double** real_weightsPtr, int undirected) {

    RandomValue* randVals;
    int i, pow, max_pow;
    int* int_weights;
    double* real_weights;
 
    if (weight_type == 2) {

        int_weights = (int *) malloc(m*sizeof(int));
        real_weights = (double *) malloc(sizeof(double)); 
        randVals = (RandomValue *) malloc(m*sizeof(RandomValue));

        giant_(&m, randVals);

        if (weight_dist == 0) {
            #pragma mta assert parallel
            for (i=0; i<m; i++) {
#ifdef __MTA__
                int_weights[i] = randVals[i] % (max_weight+1);
#else
                int_weights[i] = randVals[i] % ((unsigned int) max_weight);
#endif
            }
        } else if (weight_dist == 1) {

            #pragma mta assert parallel
            for (i=0; i<m; i++) {

                max_pow = log10(max_weight)/0.301;
                assert(max_pow > 0);
                pow = randVals[i] % max_pow; 
                int_weights[i] = 1<<(pow+1);  
        
            }
        }

        free(randVals);

    } else if (weight_type == 1) {

        int_weights = (int *) malloc(sizeof(int));
        real_weights = (double *) malloc(m*sizeof(double));
        randVals = (RandomValue *) malloc(m*sizeof(RandomValue));

        giant_(&m, randVals);

        if (weight_dist == 0) {
            #pragma mta assert parallel
            for (i=0; i<m; i++) {
                real_weights[i] = ((double) randVals[i])/4294967295.0;
            }
        } else if (weight_dist == 1) {

            #pragma mta assert parallel
            for (i=0; i<m; i++) {
                max_pow = log(max_weight)/0.301;
                assert(max_pow>0);
                pow = randVals[i] % max_pow;
                real_weights[i] = ((double) (1<<(pow+1)))/(1<<max_pow);
            }

        }
            
        free(randVals);

    } else {
        int_weights = (int *) malloc(sizeof(int));
        real_weights = (double *) malloc(sizeof(double));
    }

    *int_weightsPtr = int_weights;
    *real_weightsPtr = real_weights;
}
