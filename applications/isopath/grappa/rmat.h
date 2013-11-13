#ifndef GRAPPA_RMAT_H
#define GRAPPA_RMAT_H

#include "common.h"

extern double A;
extern double B;
extern double C;
extern double D;
//extern mrg_state * prng_state;

void rmat_edgelist(tuple_graph* grin, int64_t SCALE);

#endif /* GRAPPA_RMAT_H */
