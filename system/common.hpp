
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// useful utilities

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdint.h>

#if defined(__MTA__)
#include <sys/mta_task.h>
#include <machine/runtime.h>
#elif defined(__MACH__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#define BILLION 1000000000
#define MILLION 1000000

/// "Universal" wallclock time (works at least for Mac, MTA, and most Linux)
inline double Grappa_walltime(void) {
#if defined(__MTA__)
	return((double)mta_get_clock(0) / mta_clock_freq());
#elif defined(__MACH__)
	static mach_timebase_info_data_t info;
	mach_timebase_info(&info);
	uint64_t now = mach_absolute_time();
	now *= info.numer;
	now /= info.denom;
	return 1.0e-9 * (double)now;
#else
	struct timespec tp;
#if defined(CLOCK_PROCESS_CPUTIME_ID)
#define CLKID CLOCK_PROCESS_CPUTIME_ID
#elif  defined(CLOCK_REALTIME_ID)
#define CLKID CLOCK_REALTIME_ID
#endif
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return (double)tp.tv_sec + (double)tp.tv_nsec / BILLION;
#endif
}

#define GRAPPA_TIME(var, block) \
   	do { \
		double _tmptime = Grappa_walltime(); \
		block \
		var = Grappa_walltime()-_tmptime; \
	} while(0)

/// Compute ratio of doubles, returning 0 when divisor is 0
template< typename T, typename U >
static inline double nanless_double_ratio( T x, U y ) {
  return y == 0 ? 0.0 : static_cast<double>(x) / static_cast<double>(y);
}

/// Disable copy constructor and assignment operator.
/// Put this in your class' private declarations.
/// (from google public C++ coding standards)
#define DISALLOW_COPY_AND_ASSIGN( Name )	\
  Name( const Name & );				\
  void operator=( const Name & )

/// Sign extension.
/// From Stanford bit-twiddling hacks.
template <typename T, unsigned B>
inline T signextend(const T x)
{
  struct {T x:B;} s;
  return s.x = x;
}

/// Base 2 log of 32-bit number.
/// Modified from Stanford bit twiddling hacks.
inline unsigned int log2( unsigned int v ) {
  register unsigned int r; // result of log2(v) will go here
  register unsigned int shift;

  r =     (v > 0xFFFF) << 4; v >>= r;
  shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
  shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
  shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
  r |= (v >> 1);

  return r;
}

/// Read 64-bit timestamp counter.
#define rdtscll(val) do {					\
    unsigned int __a,__d;					\
    asm volatile("rdtsc" : "=a" (__a), "=d" (__d));		\
    (val) = ((unsigned long)__a) | (((unsigned long)__d)<<32);	\
  } while(0)

/// Read 64-bit timestamp counter.
static inline unsigned long long rdtsc() {
  unsigned long long val;
  rdtscll(val);
  return val;
}

/// OMGWTFBBQ Grappa magic identity function
/// Use this to get a pointer to a template function inside a template function, etc.
template< typename T >
T * Grappa_magic_identity_function(T * t) {
  return t;
}

/// range for block distribution
struct range_t { int64_t start, end; };

inline range_t blockDist(int64_t start, int64_t end, int64_t rank, int64_t numBlocks) {
	int64_t numElems = end-start;
	int64_t each   = numElems / numBlocks,
  remain = numElems % numBlocks;
	int64_t mynum = (rank < remain) ? each+1 : each;
	int64_t mystart = start + ((rank < remain) ? (each+1)*rank : (each+1)*remain + (rank-remain)*each);
	range_t r = { mystart, mystart+mynum };
  return r;
}

#define GET_TYPE(member) BOOST_PP_TUPLE_ELEM(2,0,member)

#define GET_NAME(member) BOOST_PP_TUPLE_ELEM(2,1,member)

#define CAT_EACH(r, data, elem) BOOST_PP_CAT(elem, data)

#define AUTO_CONSTRUCTOR_DETAIL_PARAM(r, data, member) \
GET_TYPE(member) GET_NAME(member) 

#define DECL_W_TYPE(r, data, member) \
GET_TYPE(member) GET_NAME(member);

#define AUTO_CONSTRUCTOR_DETAIL_INIT(r, data, member) \
GET_NAME(member) ( GET_NAME(member) )

#define AUTO_CONSTRUCTOR_DETAIL(className, members) \
className(BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
AUTO_CONSTRUCTOR_DETAIL_PARAM, BOOST_PP_EMPTY, members))) : \
BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM( \
AUTO_CONSTRUCTOR_DETAIL_INIT, BOOST_PP_EMPTY, members)) \
{} 

#define AUTO_CONSTRUCTOR(className, members) \
AUTO_CONSTRUCTOR_DETAIL(className, members)

#define AUTO_DECLS(members) \
BOOST_PP_SEQ_FOR_EACH(CAT_EACH, ,BOOST_PP_SEQ_TRANSFORM(DECL_W_TYPE, BOOST_PP_EMPTY, members))

struct Functor {
  void operator()();
};

#define FUNCTOR(name, members) \
struct name : public Functor { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()() const; \
}; \
inline void name::operator()() const


// fast pseudo-random number generator 0 to 32768
// http://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor/
static unsigned int g_seed;
inline void fast_srand( int seed ) {
  g_seed = seed;
}
inline int fast_rand() {
  g_seed = (214013*g_seed+2531011);
  return (g_seed>>16)&0x7FFF;
}



namespace Grappa {
  
  /// @addtogroup Utility
  /// @{
  
  /// Get string containing name of type.
  template< typename T >
  const char * typename_of( ) { 
    // how big is the name of the type of this function?
    static const int size = sizeof(__PRETTY_FUNCTION__);
    
    // make a modifiable copy that's that big
    static char fn_name[ size ] = { '\0' };
    
    // copy the name into the modifiable copy
    static const char * strcpy_retval = strncpy( fn_name, __PRETTY_FUNCTION__, size );
    
    // find the start of the type name
    static const char with[] = "[with T = ";
    static const char * name = strstr( fn_name, with ) + sizeof( with ) - 1;
    
    // erase the bracket at the end of the string
    static const char erase_bracket = fn_name[size-2] = '\0';
    
    // whew. return the string we built.
    return name;
  }
  
  /// Get string containing name of type
  template< typename T >
  const char * typename_of( const T& unused ) { 
    return typename_of<T>();
  }
  
  /// @}

#endif
