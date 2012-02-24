
#ifndef __COMMON_HPP__
#define __COMMON_HPP__

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

/// OMGWTFBBQ SoftXMT magic identity function
/// Use this to get a pointer to a template function inside a template function, etc.
template< typename T >
T * SoftXMT_magic_identity_function(T * t) {
  return t;
}

#endif
