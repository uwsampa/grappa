#ifndef INLINE_H
#define INLINE_H

#define Weak __attribute__((weak))

#ifndef NO_INLINE
#define Inline static inline
#else /* NO_INLINE */
#define Inline Weak
#endif/* NO_INLINE */

#endif /* INLINE_H */
