#ifndef MM_OPS_H
#define MM_OPS_H

/**
 * This kernel is not supported in the Cray environment.
 */
#ifndef _CRAYC

#include <assert.h>
#include <bench.h>

/************************************************
 * This header defines operations on vectors:   *
 *                                              *
 * - Logical operations                         *
 *   - vec_and                                  *
 *   - vec_or                                   *
 *   - vec_xor                                  *
 * - Pocketed arithmetic                        *
 *   NOTE: right shifts are logical (unsigned)  *
 *   - vec_shl32 (32-bit pocketed shift left)   *
 *   - vec_shr32 (32-bit pocketed shift right)  *
 *   - vec_rol32 (32-bit pocketed rotate left)  *
 *   - vec_ror32 (32-bit pocketed rotate right) *
 *   - vec_add32 (32-bit pocketed add)          *
 *   - all above operations on 64-bit pockets   *
 * - Getters/Setters                            *
 *   - vec_set32 (set a single 32-bit pocket)   *
 *   - vec_get32 (get a single 32-bit pocket)   *
 *   - vec_set32_1 (set all 32-bit pockets to   *
 *                  a single value)             *
 *   - all above operations on 64-bit pockets   *
 *                                              *
 * VEC_WIDTH must be defined to be a supported  *
 * vector width (in bits). Currently available  *
 * in this file:                                *
 *                                              *
 * - VEC_WIDTH == 32                            *
 * - VEC_WIDTH == 64                            *
 *   - Only support for logical ops in general  *
 *   - Itanium (WITH pockets, 32-bit get/set)   *
 * - VEC_WIDTH == 128                           *
 *   - SSE2
 ************************************************/
#ifndef VEC_WIDTH
#define VEC_WIDTH 64
#endif

#if VEC_WIDTH == 128
#if defined(__SSE2__)
#include <emmintrin.h>

typedef __m128i vec_t;

/* Logical operations */
#define vec_and(x,y) _mm_and_si128((x),(y))
#define vec_xor(x,y) _mm_xor_si128((x),(y))
#define vec_or(x,y)  _mm_or_si128((x),(y))

/* Pocketed arithmetic */
#define vec_shl32(x,n) _mm_slli_epi32((x),(n))
#define vec_shr32(x,n) _mm_srli_epi32((x),(n))
#define vec_shl64(x,n) _mm_slli_epi64((x),(n))
#define vec_shr64(x,n) _mm_srli_epi64((x),(n))
#define vec_add32(x,y) _mm_add_epi32((x),(y))
#define vec_add64(x,y) _mm_add_epi64((x),(y))
#define vec_rol32(x,n) vec_or(vec_shl32((x),(n)), vec_shr32((x),32-(n)))
#define vec_ror32(x,n) vec_or(vec_shr32((x),(n)), vec_shl32((x),32-(n)))
#define vec_rol64(x,n) vec_or(vec_shl64((x),(n)), vec_shr64((x),64-(n)))
#define vec_ror64(x,n) vec_or(vec_shr64((x),(n)), vec_shl64((x),64-(n)))

/* Getters / Setters
 * Structure for easier access into __m128is.
 */
typedef union {
  struct {
    uint64 u[2];
  }as64;
  vec_t m;
  struct {
    uint32 w[4];
  }as32;
} um128;

#define vec_set32_1(val) _mm_set1_epi32(val)

static inline vec_t vec_set64_1(uint64 val) {
  vec_t x;
  ((um128 *)&x)->as64.u[0] = val;
  ((um128 *)&x)->as64.u[1] = val;
  return x;
}
static inline void vec_set32(vec_t *dst, uint32 val, int pocket) {
  assert(pocket >= 0 && pocket <= 3);
  ((um128 *)dst)->as32.w[pocket] = val;
}
static inline void vec_set64(vec_t *dst, uint64 val, int pocket) {
  assert(pocket >= 0 && pocket <= 1);
  ((um128 *)dst)->as64.u[pocket] = val;
}
static inline uint32 vec_get32(vec_t *src, int pocket) {
  assert(pocket >= 0 && pocket <= 3);
  return ((um128 *)src)->as32.w[pocket];
}
static inline uint64 vec_get64(vec_t *src, int pocket) {
  assert(pocket >= 0 && pocket <= 1);
  return ((um128 *)src)->as64.u[pocket];
}

#else /* defined(__SSE2__) */
#error "VEC_WIDTH == 128, but no supported implementation defined."
#endif /* defined(__SSE2__) */

#elif VEC_WIDTH == 64
typedef uint64 vec_t;

/* Logical operations */
#define vec_and(x,y) ((x)&(y))
#define vec_xor(x,y) ((x)^(y))
#define vec_or(x,y)  ((x)|(y))

/* Pocketed arithmetic */
#if defined(__ia64) || defined(__ia64__)
#  if defined(__INTEL_COMPILER)
#    define vec_shl32(x,n) _Asm_pshl4((x),(n))
#    define vec_shr32(x,n) _Asm_pshr4_u((x),(n))
#    define vec_add32(x,y) _Asm_padd4((x),(y))
#    define vec_set32_1(val) _Asm_unpack4_l((val),(val))
#  elif defined(__GNUC__)
static inline uint64 vec_shl32(uint64 x, unsigned int n) {
  asm("pshl4 %0=%1,%2" : "=r"(x) : "r"(x), "r"(n));
  return x;
}
static inline uint64 vec_shr32(uint64 x, unsigned int n) {
  asm("pshr4.u %0=%1,%2" : "=r"(x) : "r"(x), "r"(n));
  return x;
}
static inline uint64 vec_add32(uint64 x, uint64 y) {
  asm("padd4 %0=%1,%2" : "=r"(x) : "r"(x), "r"(y));
  return x;
}
static inline uint64 vec_set32_1(uint64 x) {
  asm("unpack4.l %0=%1,%1" : "=r"(x) : "r"(x), "r"(x));
  return x;
}
#  else
#    error "__ia64__ defined, but unknown compiler in use."
#  endif
#else /* !defined(__ia64__) */
#  warning "Using very slow 32-bit pocketed operations."
static inline uint64 vec_shl32(uint64 x, unsigned int n) {
  return (x & ((((1ul<<(32-n))-1)<<32) | ((1ul<<(32-n))-1))) << n;
}
static inline uint64 vec_shr32(uint64 x, unsigned int n) {
  return ((x>>n) & ((((1ul<<(32-n))-1)<<32) | ((1ul<<(32-n))-1)));
}
static inline uint64 vec_add32(uint64 x, uint64 y) {
  uint64 sum = x + y;
  uint64 carry = (sum ^ x ^ y) & (1uL << 32);
  return sum - carry;
}
static inline uint64 vec_set32_1(uint64 val) {
  return (val << 32) | val;
}
#endif

#define vec_rol32(x,n) vec_or(vec_shl32((x),(n)), vec_shr32((x),32-(n)))
#define vec_ror32(x,n) vec_or(vec_shr32((x),(n)), vec_shl32((x),32-(n)))

#define vec_shl64(x,n) ((x)<<(n))
#define vec_shr64(x,n) ((x)>>(n))
#define vec_add64(x,y) ((x)+(n))
#define vec_rol64(x,n) vec_or(vec_shl64((x),(n)), vec_shr64((x),32-(n)))
#define vec_ror64(x,n) vec_or(vec_shr64((x),(n)), vec_shl64((x),32-(n)))

/* Getters/Setters (vec_set32_1 defined above) */
#define vec_set64_1(val) (val)
static inline void vec_set32(vec_t *dst, uint32 val, int pocket) {
  assert(pocket >= 0 && pocket <= 1);
  if (pocket == 0) {
    *dst = (*dst & 0xffffffff00000000uL) | val;
  } else {
    *dst = (*dst & 0xffffffff) | ((uint64)val << 32);
  }
}
static inline void vec_set64(vec_t *dst, uint64 val, int pocket) {
  assert(pocket == 0);
  *dst = val;
}
static inline uint32 vec_get32(vec_t *src, int pocket) {
  assert(pocket >= 0 && pocket <= 1);
  if (pocket == 0) {
    return (uint32)(*src & 0xffffffff);
  } else {
    return (uint32)(*src >> 32);
  }
}
static inline uint64 vec_get64(vec_t *src, int pocket) {
  assert(pocket == 0);
  return *src;
}

#elif VEC_WIDTH == 32
typedef uint32 vec_t;

/* Logical operations */
#define vec_and(x,y) ((x)&(y))
#define vec_xor(x,y) ((x)^(y))
#define vec_or(x,y)  ((x)|(y))

/* Pocketed arithmetic */
#define vec_shl32(x,n) ((x)<<(n))
#define vec_shr32(x,n) ((x)>>(n))
#define vec_add32(x,y) ((x)+(y))
#define vec_rol32(x,n) vec_or(vec_shl32((x),(n)), vec_shr32((x),32-(n)))
#define vec_ror32(x,n) vec_or(vec_shr32((x),(n)), vec_shl32((x),32-(n)))

/* Getters / Setters */
#define vec_set32_1(val) (val)

static inline void vec_set32(vec_t *dst, uint32 val, int pocket) {
  assert(pocket == 0);
  *dst = val;
}
static inline uint32 vec_get32(vec_t *src, int pocket) {
  assert(pocket == 0);
  return *src;
}
#else /* VEC_WIDTH */
#error "Unsupported VEC_WIDTH setting."
#endif /* VEC_WIDTH */

#endif /* _CRAYC */
#endif /* MM_OPS_H */
