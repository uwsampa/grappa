#ifndef _DEFS_H
#define _DEFS_H

/* A simple directed graph representation */
typedef struct 
{
	/* No. of edges, represented by m */
	int m;

	/* No. of vertices, represented by n */
	int n;
	
	int* endV;
	int* numEdges;
	int* intWt;
	double* realWt;
	int weight_type;

} graph;


typedef struct {

        int* vals;
        int size;
        int count;
        int min_loc;

} Bucket;

typedef struct {

        int numBuckets;
        Bucket* B;

} SFBucket;

typedef struct n{
        int val;
        struct n *next;
} list;

#endif
