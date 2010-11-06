#ifndef _primes_32_h_
#define _primes_32_h_

#ifndef ANSI_ARGS
#ifdef __STDC__
#define ANSI_ARGS(args) args
#else
#define ANSI_ARGS(args) ()
#endif
#endif

int getprime_32 ANSI_ARGS((int need, int *prime_array, int offset));

#define MAXPRIME 11863285 /* sqrt(2)*2^23 + 2 */
#define MINPRIME 3444     /* sqrt(MAXPRIME) */
#define MAXPRIMEOFFSET 779156

#endif
