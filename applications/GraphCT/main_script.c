#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
// #include <sys/resource.h>
#include <unistd.h>
#include <assert.h>

#ifdef __MTA__
#include <sys/mta_task.h>
#include <machine/runtime.h>
#endif

#include "globals.h"
#include "defs.h"


int64_t isint(char * buffer);
void bfs(graph * G, int64_t v);
void scriptCheck ( char * scriptname, int64_t run);



int main(int argc, char **argv)
{ 
	int64_t scale;
	FILE *outfp;
	outfp = fopen("results.txt", "a");
#ifdef __MTA__
	mta_suspend_event_logging();
#endif
	// setrlimit(16); 

	singleKernelOptions options;
	options.cmdsingle = 0;
	options.cmdscript = 1;
	parseCommandLineOptions(argc, argv, &options);
	
	scale = SCALE = options.scale;

	fprintf(outfp, "\n------------------------------------------------------------\n");
	fprintf(outfp, "Input filename: %s\n", options.infilename);


	/*------------------------------------------------------------------------- */
	/*       Preamble -- Untimed                                                */
	/*------------------------------------------------------------------------- */

	/* User Interface: Configurable parameters, and global program control. */
	printf("GraphCT - Tools & Kernels for Massive Graph Analysis:\n");
	printf("Running...\n\n");
	fflush (stdout);

	scriptCheck(options.infilename, 1);

	return 0;
}

