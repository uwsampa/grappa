#ifndef COMPATIO_H_

#ifdef __MTA__
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <machine/mtaops.h>
#define flip64(v) MTA_BIT_MAT_OR(0x0102040810204080, v)
#define min(a,b) (a)<(b) ? (a) : (b)

#define BUFELEMS (1L<<10)

/// poor man's assert+message, allows you to call it with a { } after it with code to run if it fails
/// Usage: CHECK( x == 0 ) { fprintf(stderr, "Invalid value of 'x' (%ld)\n", x); }
/// Note, you need not call it with a block if you don't want to (i.e. `CHECK(x == 0);` is valid as well and will simply do the same as `assert(x == 0)`)
#define CHECK(b) for ( ; !(b) ; assert(b) )

inline size_t xmt_fread(void * dest, size_t sz, size_t ct, FILE * fin) {
  CHECK(sz % sizeof(int64_t) == 0) { fprintf(stderr, "invalid fread size (not a multiple of int64_t's)"); }

  ct *= sz/sizeof(int64_t);

	int64_t buf[BUFELEMS];
	int64_t i;

  int64_t * d = (int64_t*)dest;

	int64_t pos = 0;

	while (pos < ct) {
		int64_t n = min(ct-pos, BUFELEMS);
		int64_t res = fread(buf, sizeof(int64_t), n, fin);
		for (i=0; i<n; i++) {
			d[pos+i] = flip64(buf[i]);
		}
		pos += n;
	}
  return ct;
}
#define fread xmt_fread

inline size_t xmt_fwrite(const void * src, size_t sz, size_t ct, FILE * fout) {
  CHECK(sz % sizeof(int64_t) == 0) { fprintf(stderr, "invalid fwrite size (not a multiple of int64_t's)"); }

  ct *= sz/sizeof(int64_t);

	int64_t buf[BUFELEMS];
	int64_t i;

  int64_t * d = (int64_t*)src;

	int64_t pos = 0;

	while (pos < ct) {
		int64_t n = min(ct-pos, BUFELEMS);
		for (i=0; i<n; i++) {
			buf[i] = flip64(d[pos+i]);
		}
		int64_t res = fwrite(buf, sizeof(int64_t), n, fout);
		pos += n;
	}
  return ct;
}
#define fwrite xmt_fwrite
#endif /* defined(__MTA__) */

#endif /* COMPATIO_H_ */
