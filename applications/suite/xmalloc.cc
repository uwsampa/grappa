//
//  xmalloc.cc
//  AppSuite
//
//  Created by Brandon Holt on 12/14/11.
//  Copyright 2011 University of Washington. All rights reserved.
//

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
xmmap (void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
	void * out;
	out = mmap (addr, len, prot, flags, fd, offset);
	if (MAP_FAILED != out) return out;
	perror ("mmap failed");
	abort ();
	return NULL;
}