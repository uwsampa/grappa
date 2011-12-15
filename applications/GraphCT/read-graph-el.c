#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <errno.h>
#include <assert.h>

#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <getopt.h>

#include "defs.h"
#include "xmt-luc.h"
#include "read-graph-el.h"

#if defined(__MTA__)
#define MTA(x) _Pragma(x)
#else
#define MTA(x)
#endif

#if defined(_OPENMP)
#include <omp.h>
#define OMP(x) _Pragma(x)
#else
#define OMP(x)
#endif

static int io_ready = 0;

static void
io_init (void)
{
#if defined(__MTA__)
    if (!io_ready) {
	extern void xmt_luc_io_init (void);
	xmt_luc_io_init ();
    }
#endif
    io_ready = 1;
}

MTA("mta inline")
MTA("mta expect parallel context")
static int64_t bs64 (int64_t)
#if defined(__GNUC__)
    __attribute__((const))
#endif
    ;

int64_t
bs64 (int64_t xin)
{
    uint64_t x = (uint64_t)xin; /* avoid sign-extension issues */
    x = (x >> 32) | (x << 32);
    x = ((x & ((uint64_t)0xFFFF0000FFFF0000ull)) >> 16)
	| ((x & ((uint64_t)0x0000FFFF0000FFFFull)) << 16);
    x = ((x & ((uint64_t)0xFF00FF00FF00FF00ull)) >> 8)
	| ((x & ((uint64_t)0x00FF00FF00FF00FFull)) << 8);
    return x;
}

static void
bs64_n (size_t n, int64_t * restrict d)
{
  OMP("omp parallel for")
  MTA("mta assert nodep")
  for (size_t k = 0; k < n; ++k)
    d[k] = bs64 (d[k]);
}

static ssize_t
xread(int fd, void *buf, size_t len)
{
    ssize_t lenread;
    while (1) {
	lenread = read(fd, buf, len);
	if ((lenread < 0) && (errno == EAGAIN || errno == EINTR))
	    continue;
	return lenread;
    }
}

static void
free_el (struct el * g)
{
  if (!g || !g->mem) return;
  xmunmap (g->mem, g->memsz);
  memset (g, 0, sizeof (*g));
}

void
read_graph_el (const char * fname,
	       struct el * g)
{
    const uint64_t endian_check = 0x1234ABCDul;
    size_t sz;
    int64_t *mem;

    io_init ();
    xmt_luc_stat (fname, &sz);

    if (sz % sizeof (*mem)) {
	fprintf (stderr, "graph file size is not a multiple of sizeof (int64_t)\n");
	abort ();
    }

    mem = xmmap_alloc (sz);

#if !defined(__MTA__)
    {
	int fd;
	if ( (fd = open (fname, O_RDONLY)) < 0) {
	    perror ("Error opening initial graph");
	    abort ();
	}
	if (sz != xread (fd, mem, sz)) {
	    perror ("Error reading initial graph");
	    abort ();
	}
	close (fd);
    }
#else /* __MTA__ */
    {
	extern void xmt_luc_snapin (const char*, void*, size_t);
	xmt_luc_snapin (fname, mem, sz);
    }
#endif /* __MTA__ */

    if (endian_check != *mem)
	bs64_n (sz / sizeof (*mem), mem);

    g->nv = mem[1];
    g->ne = mem[2];

    g->mem = mem;
    g->memsz = sz;

    g->d = &mem[3];
    g->el = &mem[3+g->nv];
    g->free = free_el;
}

void
free_graph_el (struct el * g)
{
  if (g && g->mem && g->free) g->free (g);
}
