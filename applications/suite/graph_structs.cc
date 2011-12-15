//
//  graph_structs.cc
//  AppSuite
//
//  Created by Brandon Holt on 12/14/11.
//  Copyright 2011 University of Washington. All rights reserved.
//
#include <sys/mman.h>

#include "defs.h"

graph::graph(int NV, int NE) {
	this->numEdges    = NE;
	this->numVertices = NV;
	this->map_size = (3 * NE + 2 * NV + 2) * sizeof (int);
	
	this->startVertex = (int *)xmmap (NULL, this->map_size, PROT_READ | PROT_WRITE,
									  MAP_PRIVATE | MAP_ANON, 0,0);
	this->endVertex   = &this->startVertex[NE];
	this->edgeStart   = &this->endVertex[NE];
	this->intWeight   = &this->edgeStart[NV+2];
	this->marks       = &this->intWeight[NE];	
}

graph::~graph() {
	munmap (this->startVertex, this->map_size);
}

graphSDG::graphSDG(int NE) {	
	this->numEdges    = NE;
	this->map_size = (3 * NE) * sizeof (int);
	this->startVertex = (int *)xmmap (NULL, this->map_size, PROT_READ | PROT_WRITE,
									  MAP_PRIVATE | MAP_ANON, 0,0);
	this->endVertex   = &this->startVertex[NE];
	this->intWeight   = &this->endVertex[NE];
}

graphSDG::~graphSDG() {
	munmap (this->startVertex, this->map_size);
}
