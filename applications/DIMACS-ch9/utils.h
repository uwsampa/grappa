#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "graph.h"
#define MAX_ALGS 5
extern unsigned m8[], m7[], m6[], m5[], m4[], m3[], m2[], m1[],
	v0[], v1[], v2[], v3[], v4[], v5[], v6[], *v[];

void randPerm(int, int, int*, int*);
void storeTimes(double*, double*, int, int);
void printTimingStats(double*, int);
int find(char**, int, char *);
#ifndef __MTA__
int int_fetch_add(int*, int);
int readfe(int*);
int writeef(int*, int);
#endif
