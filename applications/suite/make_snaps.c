#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

int64_t flip64(int64_t n) {
  union {
    int64_t i;
    char c[8];
  } u;
  char t;

  u.i = n;
  t = u.c[0]; u.c[0] = u.c[7]; u.c[7] = t;
  t = u.c[1]; u.c[1] = u.c[6]; u.c[6] = t;
  t = u.c[2]; u.c[2] = u.c[5]; u.c[5] = t;
  t = u.c[3]; u.c[3] = u.c[4]; u.c[4] = t;

  return u.i;
}

#define NBUF (1L<<26)
#define min(a,b) ((a)<(b)) ? (a) : (b)
#define CHECK(b) for ( ; !(b) ; assert(b) )

size_t read_array(char * name, size_t sz, size_t ct, FILE * fin, int64_t SCALE, int64_t edgefactor) {
  printf("reading %s...\n", name);

  CHECK(sz % sizeof(int64_t) == 0) { fprintf(stderr, "invalid fread size (not a multiple of int64_t's)"); }
  
  char fsnap[256];
  /*sprintf(fsnap, "/ufs2/home/users/holt225/snapshots/%s.%ld.%ld.snap", name, SCALE, edgefactor);*/
  sprintf(fsnap, "/scratch/tmp/holt225/snapshots/%s.%ld.%ld.snap", name, SCALE, edgefactor);
  FILE * fout = fopen(fsnap, "w");

  ct *= sz/sizeof(int64_t);

	int64_t * buf = malloc(NBUF*sizeof(int64_t));

	for (int64_t i=0; i<ct; i+= NBUF) {
		int64_t n = min(ct-i, NBUF);
		fread(buf, sizeof(int64_t), n, fin);
		for (int64_t j=0; j<n; j++) {
      // flip for XMT
			buf[j] = flip64(buf[j]);
		}
    fwrite(buf, sizeof(int64_t), ct, fout);
	}

  fclose(fout);
  free(buf);
  return ct;
}

int main(int argc, char* argv[]) {
  /*char fname[256];*/
  /*sprintf(fname, "ckpts/graph500.%ld.%ld.xmt.w.ckpt", SCALE, edgefactor);*/
  if (argc != 3) {
    fprintf(stderr, "usage: %s SCALE edgefactor\n", argv[0]);
    return 1;
  }
  int64_t SCALE = atol(argv[1]), edgefactor = atol(argv[2]);

  char fname[256];
  sprintf(fname, "ckpts/graph500.%ld.%ld.xmt.w.ckpt", SCALE, edgefactor);
  FILE * fin = fopen(fname, "r");
  if (!fin) {
    fprintf(stderr, "Unable to open file - %s\n", fname);
    return false;
  }
  printf("reading checkpoint: %s\n", fname);

  int64_t nedge, nv, nadj, nbfs;

  fread(&nedge, sizeof(nedge), 1, fin);
  fread(&nv,    sizeof(nv),    1, fin);
  fread(&nadj,  sizeof(nadj),  1, fin);
  fread(&nbfs,  sizeof(nbfs),  1, fin);

  read_array("edges", sizeof(int64_t)*2, nedge, fin, SCALE, edgefactor);
  read_array("xoff", sizeof(int64_t), 2*nv+2, fin, SCALE, edgefactor);
  read_array("xadj", sizeof(int64_t), nadj, fin, SCALE, edgefactor);

  return 0;
}
