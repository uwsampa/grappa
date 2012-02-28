
#ifndef _COLLECTIVE_HPP
#define _COLLECTIVE_HPP

#include "SoftXMT.hpp"

int64_t collective_max(int64_t a, int64_t b);
int64_t collective_min(int64_t a, int64_t b);
int64_t collective_add(int64_t a, int64_t b);
int64_t collective_mult(int64_t a, int64_t b);

#define COLL_MAX &collective_max
#define COLL_MIN &collective_min
#define COLL_ADD &collective_add
#define COLL_MULT &collective_mult


int64_t SoftXMT_collective_reduce( int64_t (*commutative_func)(int64_t, int64_t), Node home_node, int64_t myValue, int64_t initialValue );


#endif // _COLLECTIVE_HPP


