////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

/// useful utilities

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdint.h>
#include <iostream>
#include <glog/logging.h>
#include <memory>
#include <algorithm>

using std::unique_ptr;

/// Construct unique_ptr more easily. (to be included in C++1y)
/// 
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// auto m = make_unique<MyClass>(a,5);
/// // equivalent to:
/// auto m = std::unique_ptr<MyClass>(new MyClass(a,5));
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#include <cstddef>
using std::nullptr_t;

#if defined(__MACH__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

#define ONE                 (1ULL)
#define KILO                (1024ULL * ONE)
#define MEGA                (1024ULL * KILO)
#define GIGA                (1024ULL * MEGA)
#define TERA                (1024ULL * GIGA)
#define PETA                (1024ULL * TERA)
#define CACHE_LINE_SIZE     (64ULL)
#define SIZE_OF_CACHE       (MEGA * 64ULL)
#define THOUSAND        (1000ULL * ONE)
#define MILLION         (1000ULL * THOUSAND)
#define BILLION         (1000ULL * MILLION)

// align ptr x to y boundary (rounding up)
#define ALIGN_UP(x, y) \
    ((((u_int64_t)(x) & (((u_int64_t)(y))-1)) != 0) ? \
        ((void *)(((u_int64_t)(x) & (~(u_int64_t)((y)-1)))+(y))) \
        : ((void *)(x)))

/// Use to deprecate old APIs
#define GRAPPA_DEPRECATED __attribute__((deprecated))

namespace Grappa {
  
  /// Specify whether tasks are bound to the core they're spawned on, or if they can be load-balanced (via work-stealing).
  enum class TaskMode { Bound /*default*/, Unbound };
    
  /// Specify whether an operation blocks until complete, or returns "immediately".
  enum class SyncMode { Blocking /*default*/, Async };
    
  
/// "Universal" wallclock time (works at least for Mac, and most Linux)
inline double walltime(void) {
#if defined(__MACH__)
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

} // namespace Grappa

 #define GRAPPA_TIME(var, block) \
      do { \
     double _tmptime = Grappa::walltime(); \
     block \
     var = Grappa::walltime()-_tmptime; \
   } while(0)

#define GRAPPA_TIMER(var) \
    for (double _tmpstart = Grappa::walltime(), _tmptime = -1; \
         _tmptime < 0; \
         var = _tmptime = Grappa::walltime() - _tmpstart)

#define GRAPPA_TIME_LOG(name) \
    for (double _tmpstart = Grappa::walltime(), _tmptime = -1; _tmptime < 0; \
         LOG(INFO) << name << ": " << (Grappa::walltime()-_tmpstart), _tmptime = 1)

#define GRAPPA_TIME_VLOG(level, name, indent) \
    VLOG(level) << indent << name << "..."; \
    for (double _tmpstart = Grappa::walltime(), _tmptime = -1; _tmptime < 0; \
       VLOG(level) << indent << "  (" << (Grappa::walltime()-_tmpstart) << " s)", _tmptime = 1)

#define GRAPPA_TIME_REGION(var) \
    for (double _tmpstart = Grappa::walltime(), _tmptime = -1; _tmptime < 0; \
         var += (Grappa::walltime()-_tmpstart), _tmptime = 1)

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

namespace bittwiddle {
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
    unsigned int r; // result of log2(v) will go here
    unsigned int shift;

    r =     (v > 0xFFFF) << 4; v >>= r;
    shift = (v > 0xFF  ) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF   ) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3   ) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);

    return r;
  }
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

/// Helper for invoking 'std::min_element' on containers.
template< typename Container, typename Comparator >
auto min_element(const Container& c, Comparator cmp) -> decltype(*c.begin()) {
  return *std::min_element(c.begin(), c.end(), cmp);
}

/// Helper for invoking 'std::min_element' on containers.
template< typename Container, typename Comparator >
auto min_element(const Container& c0, const Container& c1, Comparator cmp) -> decltype(*c0.begin()) {
   auto m0 = min_element(c0, cmp);
   auto m1 = min_element(c1, cmp);
   return cmp(m0,m1) ? m0 : m1;
}

/// Range type that represents the values `[start,end)`.
/// Only valid for types that define `<`, `==`, and `++`.
template< typename T >
struct Range { T start, end; };

/// Helper for invoking 'std::min_element' on a Range.
template< typename T, typename Comparator >
T min_element(Range<T> r, Comparator cmp) {
  T best = r.start;
  for (T e = r.start; e < r.end; e++) {
    if (cmp(e,best)) {
      best = e;
    }
  }
  return best;
}

/// range for block distribution
using range_t = Range<int64_t>;


inline std::ostream& operator<<(std::ostream& o, const range_t& r) {
  o << "<" << r.start << "," << r.end << ">";
  return o;
}

inline range_t blockDist(int64_t start, int64_t end, int64_t rank, int64_t numBlocks) {
	int64_t numElems = end-start;
	int64_t each     = numElems / numBlocks;
  int64_t remain   = numElems % numBlocks;
	int64_t mynum    = (rank < remain) ? each+1 : each;
	int64_t mystart  = start + ((rank < remain) ? (each+1)*rank : (each+1)*remain + (rank-remain)*each);
	range_t r = { mystart, mystart+mynum };
  return r;
}

struct block_offset_t { int64_t block, offset; };

inline block_offset_t indexToBlock(int64_t index, int64_t numElements, int64_t numBlocks) {
  block_offset_t result;
	int64_t each   = numElements / numBlocks,
          remain = numElements % numBlocks;
	if (index < (each+1)*remain) {
		result = { index / (each+1), index % (each+1) };
	} else {
		index -= (each+1)*remain;
		result = { remain + index/each, index % each };
	}
  // VLOG(1) << "result.block = " << result.block << ", remain = " << remain << ", index = " << index << ", each = " << each;
  return result;
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

  namespace impl {
    // A small helper for Google logging CHECK_NULL().
    template <typename T>
    inline T* CheckNull(const char *file, int line, const char *names, T* t) {
      if (t != NULL) {
        google::LogMessageFatal(file, line, new std::string(names));
      }
      return t;
    }
  }

#define CHECK_NULL(val)                                              \
  Grappa::impl::CheckNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

#ifdef NDEBUG
#define DCHECK_NULL(val)                                              \
  Grappa::impl::CheckNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))
#else
#define DCHECK_NULL(val)                        \
  ;
#endif  


#define MPI_CHECK( mpi_call )                                           \
  do {                                                                  \
    int retval;                                                         \
    if( (retval = (mpi_call)) != 0 ) {                                  \
      char error_string[MPI_MAX_ERROR_STRING];                          \
      int length;                                                       \
      MPI_Error_string( retval, error_string, &length);                 \
      LOG(FATAL) << "MPI call failed: " #mpi_call ": "                  \
                 << error_string;                                       \
    }                                                                   \
  } while(0)

  /// @}

}

namespace Grappa {
  
/// @addtogroup Utility
/// @{

namespace impl{

static inline void prefetcht0(const void *p) {
    __builtin_prefetch(p, 0, 3);
}

static inline void prefetchnta(const void *p) {
    __builtin_prefetch(p, 0, 0);
}

static inline void prefetcht2(const void *p) {
    __builtin_prefetch(p, 0, 1);
}

}

/// @}

}
#endif
