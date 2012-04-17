
#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <stdint.h>

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

static inline unsigned long long rdtsc() {
  unsigned long long val;
  rdtscll(val);
  return val;
}

/// OMGWTFBBQ SoftXMT magic identity function
/// Use this to get a pointer to a template function inside a template function, etc.
template< typename T >
T * SoftXMT_magic_identity_function(T * t) {
  return t;
}

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

#endif
