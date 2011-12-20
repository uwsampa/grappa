// Originally from GraphCT, version 0.6.0
// License: GeorgiaTech

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#include "defs.h"

void free_graph(graph * g) {
	if(g != NULL)
		munmap(g->startVertex, g->map_size);
}

void alloc_graph(graph * g, int NV, int NE) {
	if (!g) return;
	
	g->numEdges    = NE;
	g->numVertices = NV;
	g->map_size = (3 * NE + 2 * NV + 2) * sizeof(int);
	
	g->startVertex = (int *)xmmap(NULL, g->map_size, PROT_READ | PROT_WRITE,
								   MAP_PRIVATE | MAP_ANON, 0,0);
	g->endVertex   = &g->startVertex[NE];
	g->edgeStart   = &g->endVertex[NE];
	g->intWeight   = &g->edgeStart[NV+2];
	g->marks       = &g->intWeight[NE];
}

void alloc_edgelist(edgelist * g, int NE) {
	if (!g) return;
	
	g->numEdges    = NE;
	g->map_size = (3 * NE) * sizeof(int);
	g->startVertex = (int *)xmmap(NULL, g->map_size, PROT_READ | PROT_WRITE,
								   MAP_PRIVATE | MAP_ANON, 0,0);
	g->endVertex   = &g->startVertex[NE];
	g->intWeight   = &g->endVertex[NE];
}

void free_edgelist(edgelist * g) {
	if (g)
		munmap(g->startVertex, g->map_size);
}

void print_edgelist(edgelist * g, FILE * f) {
	fprintf(f, "edgelist: %d\n", g->numEdges);
	
	for (int i=0; i<g->numEdges; i++) {
		fprintf(f, "\t(%6d, %6d) : %d\n",
				g->startVertex[i], g->endVertex[i], g->intWeight[i]);
	}
}

void print_edgelist_dot(edgelist* g, FILE* f) {
	fprintf(f, "digraph {\n");
	for (int i=0; i<g->numEdges; i++) {
		fprintf(f, "\t%6d -> %6d [weight=%d];\n",
				g->startVertex[i], g->endVertex[i], g->intWeight[i]);
	}
	fprintf(f, "}\n");
}

