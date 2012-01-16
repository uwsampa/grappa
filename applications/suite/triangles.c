// Modified from original version by Preston Briggs
// License: (created for Cray?)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"

graphint triangles(graph *g) {	
	const graphint NV = g->numVertices;
	const graphint * restrict edge = g->edgeStart; /* Edge domain */
//	const graphint * restrict sV = g->startVertex;
	const graphint * restrict eV = g->endVertex; /* Edge domain */
	graphint num_triangles = 0;
	
//	for (int i = 0; i < 100; i++) {
//		deprint("%3d | %3d\n", sV[i], eV[i]);
//	}
	
	for (graphint i = 0; i < NV; i++) {		
		graphint A = i; //= sV[i]; // edge[2*i + 0];
		
		const graphint *Aedge = eV + edge[A]; // oldedge + 2*subject[2*A + 0];
		const graphint *Alimit = eV + edge[A+1]; // Aedge + 2*subject[2*A + 1];
		while (*Aedge == A || *Aedge < A) Aedge++;
		//Aedge++; //skip the first one because that's 'B'? Aedge = oldedge + 2*i + 2;
		
		if (Aedge < Alimit) {
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
					// deprint("<%u %u %u>\n", A, B, *Aedge);
					num_triangles++;
					// advance to the end of the duplicate edges
					while (Aedge < Alimit && Aedge[1] == *Aedge) Aedge++;
					while (Bedge < Blimit && Bedge[1] == *Bedge) Bedge++;
					Aedge++;
					Bedge++;
				}
			}
		}
	}
	return num_triangles;
}
