
#ifndef _COLLECTIVE_HPP
#define _COLLECTIVE_HPP

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "common.hpp"

int64_t collective_max(int64_t a, int64_t b);
int64_t collective_min(int64_t a, int64_t b);
int64_t collective_add(int64_t a, int64_t b);
int64_t collective_mult(int64_t a, int64_t b);

#define COLL_MAX &collective_max
#define COLL_MIN &collective_min
#define COLL_ADD &collective_add
#define COLL_MULT &collective_mult


int64_t SoftXMT_collective_reduce( int64_t (*commutative_func)(int64_t, int64_t), Node home_node, int64_t myValue, int64_t initialValue );

template< typename T >
inline T coll_add(const T& a, const T& b) {
  return a+b;
}

#define HOME_NODE 0
extern Thread * reducing_thread;
extern int64_t reduction_result;
extern Node reduction_reported_in;

template< typename T >
static void am_reduce_wake(T * val, size_t sz, void * payload, size_t psz) {
  reduction_result = *val;
  SoftXMT_wake(reducing_thread);
}

template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
static void am_reduce(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(SoftXMT_mynode() == HOME_NODE);
  if (reduction_reported_in == 0) reduction_result = BaseVal;
  reduction_result = (int64_t)Reducer((T)reduction_result, *val);
  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == SoftXMT_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < SoftXMT_nodes(); n++) {
      VLOG(5) << "waking " << n;
      SoftXMT_call_on(n, &am_reduce_wake, (T*)(&reduction_result));
    }
  }
}

/// Global reduction across all nodes, returning the completely reduced value to everyone.
/// Notes:
///  - this suffices as a global barrier across *all nodes*
///  - as such, only one instance of this can be running at a given time
///  - and it must be called by every node or deadlock will occur
template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
T SoftXMT_allreduce(T myval) {
  BOOST_STATIC_ASSERT( sizeof(T) <= 8 );
  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
  
  SoftXMT_call_on(0, &am_reduce<T,Reducer,BaseVal>, &myval);
  
  SoftXMT_suspend();
  
  return reduction_result;
}

#endif // _COLLECTIVE_HPP


