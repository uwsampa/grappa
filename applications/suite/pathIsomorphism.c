// Inspired by Ullman (1976) Subgraph-isomorphism algorithm
// Brandon Holt

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#ifdef __MTA__
#	include <machine/mtaops.h>
#	include <machine/runtime.h>
#else
#	include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include "stinger-atomics.h"

/* @param g: (directed) graph to search in
   @param vertices: pattern of vertex colors to search for (in order)
   @return matches: list of vertices that mark the beginning of the pattern */
graphint pathIsomorphism(graph* g, color_t* pattern, graphint** matches) {
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
	const graphint * restrict eV = g->endVertex; /* Edge domain */
	color_t * restrict marks = g->marks; /* Vertex domain */
		
	graphint *partials = (graphint*)xmalloc(NV*sizeof(graphint));
	graphint *matchstart = (graphint*)xmalloc(NV*sizeof(graphint));
	graphint np = 0;
	graphint p = 0;
	
	// copy vertices that match beginning of pattern
	for (graphint v=0; v<NV; v++) {
		if (marks[v] == pattern[p]) {
			partials[np] = v;
			matchstart[np] = v;
			np++;
		}
	}
	p++;
	while (pattern[p] != END) {
		color_t nextMark = pattern[p];
		graphint nextV, currV;
		graphint nextnp = 0;
		
		for (graphint i=0; i<np; i++) {
			currV = partials[i];
			for (graphint j=edge[currV]; j<edge[currV+1]; j++) {
				nextV = eV[j];
				if (marks[nextV] == nextMark) {
					partials[nextnp] = nextV;
					matchstart[nextnp] = matchstart[i]; // keep track of start of match
					nextnp++;
					break;
				}
			}
		}
		
		np = nextnp;
		p++;
	}
	
	*matches = (graphint*)xmalloc(np*sizeof(graphint));
	memcpy(*matches, matchstart, np*sizeof(graphint));
	free(partials);
	free(matchstart);
	
	return np;
}

static bool checkEdgesRecursive(const graph* g, graphint v, const color_t* c) {
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
	const graphint * restrict eV = g->endVertex; /* Edge domain */
	const color_t * restrict marks = g->marks; /* Vertex domain */
	
	if (*c == END) return true;
	
	for (graphint i=edge[v]; i<edge[v+1]; i++) {
		if (marks[i] == *c) {
			if (checkEdgesRecursive(g, eV[i], c+1)) return true;
		}
	}
	return false;
}

graphint pathIsomorphismPar(const graph* g, color_t* pattern, graphint** matches) {
	const graphint NV = g->numVertices;
	const color_t * restrict marks = g->marks; /* Vertex domain */
	
	graphint nm = 0; // Number of Matches

	graphint * restrict m = (graphint*)xmalloc(NV*sizeof(graphint));
	
	MTA("mta assert parallel") OMP("omp parallel for")
	for (graphint i=0; i<NV; i++) {
		if (marks[i] == *pattern) {
			if (checkEdgesRecursive(g, i, pattern+1)) {
				m[int_fetch_add(&nm, 1)] = i;
			}
		}
	}
	
	*matches = (graphint*)xmalloc(nm*sizeof(graphint));
	memcpy(*matches, m, nm*sizeof(graphint));
	free(m);
	
	return nm;
}

graphint pathIsomorphismSpaghetti(graph* g, color_t* pattern, graphint** matches) {
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
	color_t * restrict marks = g->marks; /* Vertex domain */
	
	int np = 0;
	while (pattern[np+1] != END) np++;
	const int npattern = np;
	
	graphint nm = 0; // Number of Matches
	
	graphint* m = (graphint*)xmalloc(NV*sizeof(graphint));

	MTA("mta may reorder m")
	for (graphint i=0; i<NV; i++) {
		graphint* currV = (graphint*)xmalloc(npattern*sizeof(graphint));
		
		if (marks[i] == *pattern) {
			currV[0] = i;
			
			bool found = false;
			int d = 1; // depth into pattern
			DEPTH: while (d) {
				for (graphint j=edge[currV[d-1]]; j<edge[currV[d-1]+1]; j++) {
					if (marks[j] == pattern[d]) {
						currV[d] = j;
						d++;
						if (d == npattern) { found = true; d = 0; }
						goto DEPTH;
					}
				}
				d--;
			}
			
			if (found) {
				m[int_fetch_add(&nm,1)] = i;
			}
		}
		free(currV);
	}
	
	*matches = (graphint*)xmalloc(nm*sizeof(graphint));
	memcpy(*matches, m, nm*sizeof(graphint));
	free(m);
	
	return nm;
}

/*	In order to get reproducible results for pathIsomorphism, mark for 
	each vertex should be based on something that will be consistent 
	with the structure of the graph rather than the ordering or implicit 
	numbering of vertices.
	Arbitrarily choosing: sum(outgoing weights) % (maxc-minc) + minc 
 */
void markColors(graph *g, color_t minc, color_t maxc) {
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart;
	const graphint * restrict w = g->intWeight;
	graphint * restrict marks = g->marks;

	for (graphint i=0; i<NV; i++) {
		int total = 0;
		for (graphint j = edge[i]; j < edge[i+1]; j++) {
			total += w[j];
		}
		marks[i] = (total % (maxc-minc)) + minc;
	}
}

void print_match(graph *dirg, color_t *pattern, graphint startVertex) {
	graphint v = startVertex;
	color_t *c = pattern;
	printf("match %d: %"DFMT"(%"DFMT")", 0, v, *c);
	c++;
	while (*c != END) {
		graphint nextV = -1;
		for (graphint i=dirg->edgeStart[v]; i<dirg->edgeStart[v+1]; i++) {
			if (dirg->marks[dirg->endVertex[i]] == *c) {
				nextV = dirg->endVertex[i];
				break;
			}
		}
		assert(nextV != -1);
		printf(" -> %"DFMT"(%"DFMT")", nextV, *c);
		v = nextV;
		c++;
	}
	printf("\n");
}
