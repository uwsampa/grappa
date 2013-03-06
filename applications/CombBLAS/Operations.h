/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.2 -------------------------------------------------*/
/* date: 10/06/2011 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/
/*
 Copyright (c) 2011, Aydin Buluc
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

/**
 * Operations used in parallel reductions and scans
 **/

#ifndef _OPERATIONS_H_
#define _OPERATIONS_H_

#include <iostream>
#include <functional>
#include <cmath>
#include <mpi.h>

using namespace std;


template<typename T>
struct myset: public std::unary_function<T, T>
{
  myset(T myvalue): value(myvalue) {};
  /** @returns value regardless of x */
  const T& operator()(const T& x) const
  {
    return value;
  } 
  T value;
};

template<typename T>
struct identity : public std::unary_function<T, T>
{
  /** @returns x itself */
  const T operator()(const T& x) const
  {
	return x;
  }
};


// Because identify reports ambiguity in PGI compilers
template<typename T>
struct myidentity : public std::unary_function<T, T>
{
  /** @returns x itself */
  const T operator()(const T& x) const
  {
	return x;
  }
};


template<typename T>
struct totality : public std::unary_function<T, bool>
{
  /** @returns true regardless */
  bool operator()(const T& x) const
  {
	return true;
  }
};
	
	
template<typename T>
struct safemultinv : public std::unary_function<T, T>
{
  const T operator()(const T& x) const
  {
	T inf = std::numeric_limits<T>::max();
    	return (x == 0) ? inf:(1/x);
  }
};


template<typename T1, typename T2>
struct bintotality : public std::binary_function<T1, T2, bool>
{
  /** @returns true regardless */
  bool operator()(const T1& x, const T2 & y) const
  {
        return true;
  }
};



/**
 * binary_function<Arg1, Arg2, Result>
 * This is left untemplated because pow() only makes sense for 
 * <double, int, double> , <double, double, double> , <float, float, float>
 * and C++ can automatically upcast each case to <double, double, double>
 */
struct exponentiate : public std::binary_function<double, double, double> 
{
    double operator()(double x, double y) const { return std::pow(x, y); }
};


/**
 *  @brief Compute the maximum of two values.
 *
 *  This binary function object computes the maximum of the two values
 *  it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_MAX.
 */
template<typename T>
struct maximum : public std::binary_function<T, T, T>
{
  /** @returns the maximum of x and y. */
  const T operator()(const T& x, const T& y) const
  {
    return x < y? y : x;
  }
};


/**
 *  @brief Compute the minimum of two values.
 *
 *  This binary function object computes the minimum of the two values
 *  it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_MIN.
 */
template<typename T>
struct minimum : public std::binary_function<T, T, T>
{
  /** @returns the minimum of x and y. */
  const T operator()(const T& x, const T& y) const
  {
    return x < y? x : y;
  }
};

/**
 *  @brief Compute the bitwise AND of two integral values.
 *
 *  This binary function object computes the bitwise AND of the two
 *  values it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_BAND.
 */
template<typename T>
struct bitwise_and : public std::binary_function<T, T, T>
{
  /** @returns @c x & y. */
  T operator()(const T& x, const T& y) const
  {
    return x & y;
  }
};


/**
 *  @brief Compute the bitwise OR of two integral values.
 *
 *  This binary function object computes the bitwise OR of the two
 *  values it is given. When used with MPI and a type @c T that has an
 *  associated, built-in MPI data type, translates to @c MPI_BOR.
 */
template<typename T>
struct bitwise_or : public std::binary_function<T, T, T>
{
  /** @returns the @c x | y. */
  T operator()(const T& x, const T& y) const
  {
    return x | y;
  }
};

/**
 *  @brief Compute the logical exclusive OR of two integral values.
 *
 *  This binary function object computes the logical exclusive of the
 *  two values it is given. When used with MPI and a type @c T that has
 *  an associated, built-in MPI data type, translates to @c MPI_LXOR.
 */
template<typename T>
struct logical_xor : public std::binary_function<T, T, T>
{
  /** @returns the logical exclusive OR of x and y. */
  T operator()(const T& x, const T& y) const
  {
    return (x || y) && !(x && y);
  }
};

/**
 *  @brief Compute the bitwise exclusive OR of two integral values.
 *
 *  This binary function object computes the bitwise exclusive OR of
 *  the two values it is given. When used with MPI and a type @c T that
 *  has an associated, built-in MPI data type, translates to @c
 *  MPI_BXOR.
 */
template<typename T>
struct bitwise_xor : public std::binary_function<T, T, T>
{
  /** @returns @c x ^ y. */
  T operator()(const T& x, const T& y) const
  {
    return x ^ y;
  }
};


// MPIOp: A class that has a static op() function that takes no arguments and returns the corresponding MPI_Op
// if and only if the given Op has a mapping to a valid MPI_Op
// No concepts checking for the applicability of Op on the datatype T at the moment
// In the future, this can be implemented via metafunction forwarding using mpl::or_ and mpl::bool_

template <typename Op, typename T> 
struct MPIOp
{
};

template<typename T> struct MPIOp< maximum<T>, T > {  static MPI_Op op() { return MPI_MAX; } };
template<typename T> struct MPIOp< minimum<T>, T > {  static MPI_Op op() { return MPI_MIN; } };
template<typename T> struct MPIOp< std::plus<T>, T > {  static MPI_Op op() { return MPI_SUM; } };
template<typename T> struct MPIOp< std::multiplies<T>, T > {  static MPI_Op op() { return MPI_PROD; } };
template<typename T> struct MPIOp< std::logical_and<T>, T > {  static MPI_Op op() { return MPI_LAND; } };
template<typename T> struct MPIOp< std::logical_or<T>, T > {  static MPI_Op op() { return MPI_LOR; } };
template<typename T> struct MPIOp< logical_xor<T>, T > {  static MPI_Op op() { return MPI_LXOR; } };
template<typename T> struct MPIOp< bitwise_and<T>, T > {  static MPI_Op op() { return MPI_BAND; } };
template<typename T> struct MPIOp< bitwise_or<T>, T > {  static MPI_Op op() { return MPI_BOR; } };
template<typename T> struct MPIOp< bitwise_xor<T>, T > {  static MPI_Op op() { return MPI_BXOR; } };


#endif


