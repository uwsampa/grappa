#define _FILE_OFFSET_BITS 64
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/stat.h>
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif
#include "defs.h"
#include "xmt-luc.h"
#include "stinger-atomics.h"


char * remove_comments(char *filemem, int64_t stop)
{
	char buffer[1000];
	
	while ( sscanf(filemem, "%s\n", buffer) )
	{
		if (buffer[0] == stop) break;
		filemem = strchr(filemem+1, '\n');
	}
	return filemem;
}

static inline char * nextchar(char * buffer, char c)
{
	while(*(buffer++) != c && (*buffer) != 0);
	return buffer;
}

static inline char * readint(char * buffer, int64_t * n)
{
	int64_t ans = 0;
	while(*buffer != '\n' && *buffer != ' ' && *buffer != 0 )
	{
		ans *= 10;
		ans += (int64_t) (buffer[0] - '0');
		buffer++;
	}
	buffer++;
	*n = ans;
	return buffer;
}

static inline char * readline(char * buffer, int64_t * start, int64_t * end, int64_t * weight)
{
	buffer = nextchar(buffer, ' ');
	buffer = readint(buffer, start);
	buffer = readint(buffer, end);
	buffer = readint(buffer, weight);
	return buffer;
}

static int64_t io_ready = 0;

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

graph * parse_DIMACS_graph(char *filename) {

	int64_t i;
	int64_t NE, NV;
#if !defined(__MTA__)
	off_t filesize;
#else
	size_t filesize;
#endif

	io_init();
	
	extern void xmt_luc_stat (const char*, size_t*);
	xmt_luc_stat(filename, &filesize);

	printf("Filesize: %ld\n", (int64_t)filesize);

	char * filemem = (char *) xmmap (NULL, (filesize + 5) * sizeof (char),
					 PROT_READ | PROT_WRITE,
					 MAP_PRIVATE | MAP_ANON, 0, 0); 
	filemem[filesize] = 0;
	filemem[filesize+1] = 0;
	filemem[filesize+2] = 0;
	filemem[filesize+3] = 0;
	extern void xmt_luc_snapin (const char*, void*, size_t);
    xmt_luc_snapin (filename, filemem, filesize);

	char *ptr = filemem;
	char buffer[100];
	
	ptr = remove_comments(ptr, 'p');

	sscanf(ptr, "%s %*s %ld %ld\n", buffer, &NV, &NE);
	printf("NV: %ld, NE: %ld\n", NV, NE);
	ptr = strchr(ptr, '\n');

	ptr = remove_comments(ptr, 'a');

	int64_t *sV1 = xmalloc(NE * sizeof(int64_t));
	int64_t *eV1 = xmalloc(NE * sizeof(int64_t));
	int64_t *w1 = xmalloc(NE * sizeof(int64_t));

	int64_t edgecount = 0;

#if defined(__MTA__)
MTA("mta assert parallel")
MTA("mta dynamic schedule")
MTA("mta use 100 streams")
	for (i = 0; i < 5000; i++)
	{
		int64_t startV, endV, weight;
		char *myPtr = ptr + (i*(filesize/5000));
		char *endPtr = ptr + (i+1)*(filesize/5000);
		if(i == 4999) endPtr = filemem+filesize-1;
		if (myPtr > filemem+filesize-1) myPtr = endPtr+1;
		myPtr = nextchar(myPtr, '\n');
		int64_t j=0;
		while ( myPtr <= endPtr )
		{
			j++;
			myPtr = readline(myPtr,	&startV, &endV, &weight);
			int64_t k = stinger_int_fetch_add(&edgecount, 1);
			sV1[k] = startV;
			eV1[k] = endV;
			w1[k] = weight;
		}
	}
#else
	{
		int64_t startV, endV, weight;
		char *myPtr = ptr;
		char *endPtr = filemem+filesize-1;
		myPtr = nextchar(myPtr, '\n');
		int64_t j=0;
		while ( myPtr <= endPtr )
		{
			j++;
			myPtr = readline(myPtr,	&startV, &endV, &weight);
			int64_t k = int_fetch_add(&edgecount, 1);
			sV1[k] = startV;
			eV1[k] = endV;
			w1[k] = weight;
		}
	}
#endif

	printf("edges ingested: %d\n", edgecount);

	if (edgecount != NE) {
		printf("ERROR: Incorrect number of edges\n");
		return 0;
	}

#define CHECK_EXTREME(a,OP,X) \
	do { if (a OP X) { int64_t X2 = readfe (&X); if (a OP X2) X2 = a; writeef (&X, X2); } } while (0)

	int64_t minv = 2*NV, maxv = 0;
MTA("mta assert nodep")
	for (i = 0; i < NE; ++i) {
	  const int64_t sv = sV1[i], ev = eV1[i];
	  if (sv < ev) {
	    CHECK_EXTREME(sv, <, minv);
	    CHECK_EXTREME(ev, >, maxv);
	  } else {
	    CHECK_EXTREME(ev, <, minv);
	    CHECK_EXTREME(sv, >, maxv);
	  }
	}
	/* Check if the graph may have been 1-based, adjust if possible. */
	if (0 < minv && maxv == NV) {
	  --minv;
	  --maxv;
	  for (i = 0; i < NE; ++i) {
	    --sV1[i];
	    --eV1[i];
	  }
	}
	if (maxv >= NV) {
	  printf ("ERROR: Vertices out of range\n");
	  return 0;
	}

	graph * G = xmalloc(sizeof(graph));
	alloc_graph (G, NV, NE);

	printf("Begin sort...");
	SortStart(NV, NE, sV1, eV1, w1,
		  G->startVertex, G->endVertex, G->intWeight, G->edgeStart);
	printf("done.\n");

	free(sV1);
	free(eV1);
	free(w1);

MTA("mta assert nodep")
	for (i = 0; i < NV; i++) {
		G->marks[i] = 0;
	}

	munmap (filemem, (filesize + 5) * sizeof (char));
	
	return G;
}
