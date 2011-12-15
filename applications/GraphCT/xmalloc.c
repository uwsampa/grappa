#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "defs.h"


MTA("mta inline")
void *
xmalloc (size_t size)
{
  void * out;
  if ( (out = malloc (size)) ) return out;
  perror ("malloc failed");
  abort ();
  return NULL;
}

MTA("mta inline")
void *
xcalloc (size_t nmemb, size_t size)
{
  void * out;
  if ( (out = calloc (nmemb, size)) ) return out;
  perror ("calloc failed");
  abort ();
  return NULL;
}

MTA("mta inline")
void *
xrealloc (void *ptr, size_t size)
{
  void * out;
  if ( (out = realloc (ptr, size)) ) return out;
  perror ("realloc failed");
  abort ();
  return NULL;
}

MTA("mta inline")
void *
xmmap (void *addr, size_t len, int64_t prot, int64_t flags, int64_t fd, off_t offset)
{
  void * out;
  out = mmap (addr, len, prot, flags, fd, offset);
  if (MAP_FAILED != out) return out;
  perror ("mmap failed");
  abort ();
  return NULL;
}

void *
xmmap_alloc (size_t sz)
{
  //return xmmap (NULL, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0);
  return xmalloc (sz);
}

void
xmunmap (void * p, size_t sz)
{
  //munmap (p, sz);
  free (p);
}
