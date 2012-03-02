// Modified from original version by Preston Briggs
// License: (created for Cray?)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

/*	Finds the number of _unique_ triangles in an undirected graph.
	Assumes that each vertex's outgoing edge list is sorted by endVertex.
 
	A triangle is 3 vertices that are connected to each other without going 
	through any other vertices. That is:
	
	        A              A
	       / \   not...   / \
		  B---C          B-D-C
	
	It is worth noting that this count excludes duplicate edges and self-edges 
	that are actually present in the graph.
 */
graphint triangles(graph *g) {
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
	const graphint * restrict eV = g->endVertex; /* Edge domain */
	graphint num_triangles = 0;
	
//	for (int i = 0; i < g->numEdges; i++) {
//		deprint("%3lld | %3lld\n", g->startVertex[i], eV[i]);
//	}
	
	for (graphint i = 0; i < NV; i++) {		
		graphint A = i; //= sV[i]; // edge[2*i + 0];
		
		const graphint *Astart = eV + edge[A];
		const graphint *Aedge; // = eV + edge[A]; // oldedge + 2*subject[2*A + 0];
		const graphint *Alimit = eV + edge[A+1]; // Aedge + 2*subject[2*A + 1];
		while (*Astart <= A) Astart++;
		//Aedge++; //skip the first one because that's 'B'? Aedge = oldedge + 2*i + 2;
		
		while (Astart < Alimit) {
			Aedge = Astart;
			
			graphint B = *Aedge; // oldedge[2*i + 1];
			const graphint *Bedge = eV + edge[B]; // oldedge + 2*subject[2*B + 0];
			const graphint *Blimit = eV + edge[B+1]; // Bedge + 2*subject[2*B + 1];
			while (*Bedge <= B || *Bedge < A) Bedge++;
			// deprint("A = %u, B = %u\n", A, B);
			while (Aedge < Alimit && Bedge < Blimit) {
				// deprint("*A = %u, *B = %u\n", *Aedge, *Bedge);
				if (*Aedge < *Bedge) {
					Aedge++;
				} else if (*Aedge > *Bedge) {
					Bedge++;
				} else {
//					deprint("<%llu %llu %llu>\n", A, B, *Aedge);
					num_triangles++;
					// advance to the end of the duplicate edges
					while (Aedge < Alimit && Aedge[1] == *Aedge) Aedge++;
					while (Bedge < Blimit && Bedge[1] == *Bedge) Bedge++;
					Aedge++;
					Bedge++;
				}
			}
			
			while (Astart < Alimit && Astart[1] == *Astart) Astart++; // jump over duplicates
			Astart++;
		}
	}
	return num_triangles;
}
