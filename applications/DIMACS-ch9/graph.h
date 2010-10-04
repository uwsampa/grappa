#ifndef _GRAPH_H
#define _GRAPH_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "defs.h"
#include "utils.h"

#ifdef __MTA__
typedef unsigned __short32 RandomValue;
#else
typedef unsigned int RandomValue;
#endif

extern void giant_(int*, RandomValue*);

void loadGraph(graph*, int, int, int, int*, int*, int*, double*, int);
void random_Gnm(int, int, int, int, int, int**, int**, int**, double**, int);
void CraySF(int, unsigned int*, int*, int*, int, int, int, int**, int**, int**,
        double**, int);
void RMatSF(int, int, int, int, int, int**, int**, int**, double**, int);
void choosePartition(double, int*, int*, int, double, double, double, double);
void varyParams(double*, int, double*, double*, double*, double*);
void random_geometric(int, int, int, int, int, int**, int**, int**, double**,
        int);
void random_bullseye(int, int, int, int, int, int, int**, int**, int**,
        double**, int);
void random_2Dtorus(int, int, int, int, int*, int, int, int, int**, int**,
        int**, double**, int);
void random_3Dtorus(int, int, int, int, int, int*, int, int, int, int**, int**,
        int**, double**, int);
void random_hierarchical(int, int, int*, int, int, int, int**, int**,
        int**, double**, int);
void grid(int, int, int*, int*, int, int, int, int**, int**, int**,
        double**, int);
void readGraphFromFile(char*, int*, char*, int*, int*, int, int*, int**, int**, int**,
        double**, int);
void input_error(char *, int);
void genEdgeWeights(int, int, int, int, int**, double**, int);
#endif
