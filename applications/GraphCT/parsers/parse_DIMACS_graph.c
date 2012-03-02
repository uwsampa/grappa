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


void SortStart(NV, NE, sv1, ev1, w1, sv2, ev2, w2, start)
  int64_t NV, NE, *sv1, *ev1, *w1, *sv2, *ev2, *w2, *start;
{ int64_t i;
  for (i = 0; i < NV + 2; i++) start[i] = 0;

  start += 2;

/* Histogram key values */
MTA("mta assert no alias *start *sv1")
  for (i = 0; i < NE; i++) start[sv1[i]] ++;

/* Compute start index of each bucket */
  for (i = 1; i < NV; i++) start[i] += start[i-1];

  start --;

/* Move edges into its bucket's segment */
MTA("mta assert no dependence")
  for (i = 0; i < NE; i++) {
      int64_t index = start[sv1[i]] ++;
      sv2[index] = sv1[i];
      ev2[index] = ev1[i];
      w2[index] = w1[i];
} }

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

inline char * nextchar(char * buffer, char c)
{
	while(*(buffer++) != c && (*buffer) != 0);
	return buffer;
}

inline char * readint(char * buffer, int64_t * n)
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

inline char * readline(char * buffer, int64_t * start, int64_t * end, int64_t * weight)
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

int main(int argc, char *argv[]) {

	int64_t i;
	int64_t NE, NV;
#if !defined(__MTA__)
	off_t filesize;
#else
	size_t filesize;
#endif

	io_init();

	if (argc != 2) {
		printf("Usage: %s input_filename\n", argv[0]);
		return 0;
	}

	xmt_luc_stat(argv[1], &filesize);

	printf("Filesize: %ld\n", filesize);

	char * filemem = (char *) xmmap (NULL, (filesize + 5) * sizeof (char),
					 PROT_READ | PROT_WRITE,
					 MAP_PRIVATE | MAP_ANON, 0, 0);
	filemem[filesize] = 0;
	filemem[filesize+1] = 0;
	filemem[filesize+2] = 0;
	filemem[filesize+3] = 0;
	xmt_luc_snapin (argv[1], filemem, filesize);

	char *ptr = filemem;
	
	ptr = remove_comments(ptr, 'p');

	sscanf(ptr, "%*s %*s %ld %ld\n", &NV, &NE);
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

	printf("edges ingested: %ld\n", edgecount);

	if (edgecount != NE) {
		printf("ERROR: Incorrect number of edges\n");
		return 0;
	}

#if defined(__MTA__)
#define CHECK_EXTREME(a,OP,X) \
	do { if (a OP X) { int64_t X2 = readfe (&X); if (a OP X2) X2 = a; writeef (&X, X2); } } while (0)
#else
#define CHECK_EXTREME(a,OP,X) \
	do { if (a OP X) X = a; } while (0)
#endif

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

	int64_t *sV2 = xmalloc(NE * sizeof(int64_t));
	int64_t *eV2 = xmalloc(NE * sizeof(int64_t));
	int64_t *w2 = xmalloc(NE * sizeof(int64_t));
	int64_t *start = xmalloc((NV+2) * sizeof(int64_t));

	printf("Begin sort...");
	SortStart(NV, NE, sV1, eV1, w1, sV2, eV2, w2, start);
	printf("done.\n");

	free(sV1);
	free(eV1);
	free(w1);
	free(sV2);

	uint32_t *output = xmalloc( (3+NV+NE+NE)*sizeof(uint32_t));

	output[0] = (uint32_t) NE;
	output[1] = (uint32_t) NV;
	output[2] = 0;

	for (i = 0; i < NV; i++) {
		output[3+i] = (uint32_t) start[i];
	}

	for (i = 0; i < NE; i++) {
		output[3+NV+i] = (uint32_t) eV2[i];
		output[3+NV+NE+i] = (uint32_t) w2[i];
	}

	int64_t filenamelen = strlen(argv[1]);
	char *newname = xmalloc( (filenamelen+10) * sizeof(char));
	strcpy(newname, argv[1]);
	strcat(newname, ".bin");

	remove(newname);

	int64_t newfilesize = 3+NV+NE+NE;

	xmt_luc_snapout(newname, output, newfilesize*sizeof(uint32_t));

	free(eV2);
	free(w2);
	free(start);
	free(output);
	munmap(filemem,  (filesize + 5) * sizeof (char));
	
	return 0;
}
