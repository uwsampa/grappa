
#ifndef _COLLECTIVE_HPP
#define _COLLECTIVE_HPP

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "common.hpp"

#define COLL_MAX &collective_max
#define COLL_MIN &collective_min
#define COLL_ADD &collective_add
#define COLL_MULT &collective_mult

template< typename T >
T collective_add(const T& a, const T& b) {
  return a+b;
}
template< typename T >
T collective_max(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T collective_min(const T& a, const T& b) {
  return (a>b) ? a : b;
}
template< typename T >
T collective_mult(const T& a, const T& b) {
  return a*b;
}

#define HOME_NODE 0
extern Thread * reducing_thread;
extern Node reduction_reported_in;

// This class is just for holding the reduction
// value in a type-safe manner
template < typename T >
class Reductions {
  private:
    Reductions() {}
  public:
    static T reduction_result;
    static T final_reduction_result;
};

template <typename T>
T Reductions<T>::reduction_result;

template <typename T> 
T Reductions<T>::final_reduction_result;

template< typename T >
static void am_reduce_wake(T * val, size_t sz, void * payload, size_t psz) {
  Reductions<T>::final_reduction_result = *val;
  SoftXMT_wake(reducing_thread);
}

template< typename T, T (*Reducer)(const T&, const T&), T BaseVal>
static void am_reduce(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(SoftXMT_mynode() == HOME_NODE);

  if (reduction_reported_in == 0) Reductions<T>::reduction_result = BaseVal;
  Reductions<T>::reduction_result = Reducer(Reductions<T>::reduction_result, *val);

  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == SoftXMT_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < SoftXMT_nodes(); n++) {
      VLOG(5) << "waking " << n;
      T data = Reductions<T>::reduction_result;
      SoftXMT_call_on(n, &am_reduce_wake, &data);
    }
  }
}

template< typename T, T (*Reducer)(const T&, const T&) >
static void am_reduce_noinit(T * val, size_t sz, void* payload, size_t psz) {
  CHECK(SoftXMT_mynode() == HOME_NODE);
  
  if (reduction_reported_in == 0) Reductions<T>::reduction_result = *val; // no base val
  else Reductions<T>::reduction_result = Reducer(Reductions<T>::reduction_result, *val);
  
  reduction_reported_in++;
  VLOG(5) << "reported_in = " << reduction_reported_in;
  if (reduction_reported_in == SoftXMT_nodes()) {
    reduction_reported_in = 0;
    for (Node n = 0; n < SoftXMT_nodes(); n++) {
      VLOG(5) << "waking " << n;
      T data = Reductions<T>::reduction_result;
      SoftXMT_call_on(n, &am_reduce_wake, &data);
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
  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
  
  SoftXMT_call_on(HOME_NODE, &am_reduce<T,Reducer,BaseVal>, &myval);
  
  SoftXMT_suspend();
  
  return Reductions<T>::final_reduction_result;
}

template< typename T, T (*Reducer)(const T&, const T&) >
T SoftXMT_allreduce_noinit(T myval) {
  // TODO: do tree reduction to reduce amount of serialization at Node 0
  reducing_thread = CURRENT_THREAD;
  
  SoftXMT_call_on(HOME_NODE, &am_reduce_noinit<T,Reducer>, &myval);
  
  SoftXMT_suspend();
  
  return Reductions<T>::final_reduction_result;
}

#endif // _COLLECTIVE_HPP


