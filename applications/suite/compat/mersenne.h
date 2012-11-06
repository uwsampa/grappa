#ifndef _MERSENNE_H_
#define _MERSENNE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t mersenne_rand(void); 
void mersenne_seed(uint32_t seed);

#ifdef __cplusplus
}
#endif
#endif // _MERSENNE_H_
