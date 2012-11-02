#ifndef COMPATIO_H_

/// poor man's assert+message, allows you to call it with a { } after it with code to run if it fails
/// Usage: CHECK( x == 0 ) { fprintf(stderr, "Invalid value of 'x' (%ld)\n", x); }
/// Note, you need not call it with a block if you don't want to (i.e. `CHECK(x == 0);` is valid as well and will simply do the same as `assert(x == 0)`)
#define CHECK(b) for ( ; !(b) ; assert(b) )

#include <stdint.h>
#include <stdio.h>
#include "timer.h"

#ifdef __MTA__
#include <assert.h>
#include <machine/mtaops.h>
#include <snapshot/client.h>
#include <unistd.h>
#define flip64(v) MTA_BIT_MAT_OR(0x0102040810204080, v)

#define BUFELEMS (1L<<10)

inline size_t xmt_fread(void * dest, size_t sz, size_t ct, FILE * fin) {
  CHECK(sz % sizeof(int64_t) == 0) { fprintf(stderr, "invalid fread size (not a multiple of int64_t's)"); }

  ct *= sz/sizeof(int64_t);

	int64_t buf[BUFELEMS];
	int64_t i;

  int64_t * d = (int64_t*)dest;

	int64_t pos = 0;

	while (pos < ct) {
		int64_t n = MIN(ct-pos, BUFELEMS);
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
		int64_t n = MIN(ct-pos, BUFELEMS);
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

inline size_t fread_plus(void * buf, size_t elt_size, size_t nelt, FILE * fin, char * name, int64_t SCALE, int64_t edgefactor) {
  double t = timer();
#ifdef __MTA__
  // first look for XMT snapshot
  char fsnap[256];
  //sprintf(fsnap, "/ufs2/home/users/holt225/snapshots/%s.%ld.%ld.snap", name, SCALE, edgefactor);
  sprintf(fsnap, "/scratch/tmp/holt225/snapshots/%s.%ld.%ld.snap", name, SCALE, edgefactor);
  int result = access(fsnap, F_OK);

  //if (access(fsnap, F_OK) == 0) {
  // assume we can find a snapshot because we can't actually check if it's on /scratch
  if (1) {
    if (snap_restore(fsnap, buf, nelt*elt_size, NULL) != SNAP_ERR_OK) {
      fprintf(stderr, "error: problem with snapshot!\n");
      return 0;
    }
    fseek(fin, nelt*elt_size, SEEK_CUR);
    printf("%s_snapshot_read_time: %g\n", name, timer()-t);
  } else {
    fprintf(stderr, "unable to find snapshot\n");
#endif
    fread(buf, elt_size, nelt, fin);
    printf("%s_read_time: %g\n", name, timer()-t);
#ifdef __MTA__
    // now generate snapshot for next time
    snap_snapshot(fsnap, buf, nelt*elt_size, NULL);
    printf("%s_snapshot_write_time: %g\n", name, timer()-t);
  }
#endif
  return nelt;
}


#endif /* COMPATIO_H_ */
