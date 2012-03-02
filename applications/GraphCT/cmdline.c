#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#if !defined(__MTA__)
#include "compat/xmt-ops.h"
#endif

#define SCRATCHLEN 8192
static double scratch[SCRATCHLEN];

void parseCommandLineOptions (int64_t argc, char **argv, singleKernelOptions *options)
{
	int64_t argIndex;
	char *infilename, *outfilename, *kernel;
	int64_t graph_type, kernel_id = 0;

	if (options->cmdsingle)
	{
		if (argc < 2)
		{
			printf("\n Usage: %s -i <graph filename> -t <graph type> -o <output filename> -s <random seed>\n\t-z <kernel> [-d <diameter>] [--printstats]\n\n", argv[0]);
			exit(-1);
		}
	}

	if (options->cmdscript)
	{
		if (argc != 3)
		{
			printf("\n Usage: %s -i <script filename> \n\n", argv[0]);
			exit(-1);
		}
	}


	argIndex = 0;
	infilename = NULL;
	outfilename = NULL;
	kernel = (char *) calloc (500, sizeof(char));

	options->scale = 0;
	options->printstats = 0;

	while (argIndex < argc)
	{
		if (strcmp(argv[argIndex], "-i")==0)
		{
			if (infilename) {
				fprintf (stderr, "Already specified input file.\n");
				abort ();
			}
			infilename = (char *) calloc (500, sizeof(char));
			strcpy(infilename, argv[++argIndex]);
		}

		else if (strcmp(argv[argIndex], "--printstats")==0)
		{
			options->printstats = 1;
		}

		else if (strcmp(argv[argIndex], "-s")==0)
		{
			int64_t seed = atoi(argv[++argIndex]);
			if(seed > 0)
			{
				printf("Generating %ld numbers to seed prand...\n", seed);
				while (seed > 0) {
				  const int64_t n = (seed < SCRATCHLEN? seed : SCRATCHLEN);
				  prand(n, scratch);
				  seed -= n;
				}
			}
			printf("Done seeding.\n");
		}

		else if (strcmp(argv[argIndex], "-o")==0)
		{
			if (outfilename) {
				fprintf (stderr, "Already specified output file.\n");
				abort ();
			}
			outfilename = (char *) calloc (500, sizeof(char));
			strcpy(outfilename, argv[++argIndex]);
		}

		else if (strcmp(argv[argIndex], "-t")==0)
		{
			if (strcmp(argv[++argIndex], "rmat")==0)
				graph_type = 3;
			else if (strcmp(argv[argIndex], "dimacs")==0)
				graph_type = 2;
			else if (strcmp(argv[argIndex], "binary")==0)
				graph_type = 1;
			else if (strcmp(argv[argIndex], "el")==0)
				graph_type = 4;
		}
		
		else if (strcmp(argv[argIndex], "-z")==0)
		{
			strcpy(kernel, argv[++argIndex]);

			if (strcmp(kernel, "kcentrality")==0)
			{
				kernel_id = 1;
				options->parameter1 = atoi(argv[++argIndex]);  // k
				options->parameter2 = atoi(argv[++argIndex]);  // Vs
			}
			else if (strcmp(kernel, "modularity")==0)
			{
				kernel_id = 2;
			}
			else if (strcmp(kernel, "degree")==0)
			{
				kernel_id = 3;
			}
			else if (strcmp(kernel, "conductance")==0)
			{
				kernel_id = 4;
			}
			else if (strcmp(kernel, "components")==0)
			{
				kernel_id = 5;
			}
			else if (strcmp(kernel, "clustering")==0)
			{
				kernel_id = 6;
			}
			else if (strcmp(kernel, "transitivity")==0)
			{
				kernel_id = 7;
			}
			else if (strcmp(kernel, "diameter")==0)
			{
				kernel_id = 8;
				options->parameter1 = atoi(argv[++argIndex]);  // Vs
			}
			else if (strcmp(kernel, "partition:heavy")==0)
			{
				kernel_id = 9;
				options->parameter1 = 1;
			}
			else if (strcmp(kernel, "partition:cnm")==0)
			{
				kernel_id = 9;
				options->parameter1 = 2;
			}
			else if (strcmp(kernel, "partition:mb")==0)
			{
				kernel_id = 9;
				options->parameter1 = 3;
			}
			else if (strcmp(kernel, "partition:conductance")==0)
			{
				kernel_id = 9;
				options->parameter1 = 4;
			}
		}

		else if (strcmp(argv[argIndex], "-d")==0)
		{
			options->scale = atoi(argv[++argIndex]);
			printf("diameter=%ld\n", options->scale);
		}

		argIndex++;
	}


	options->graph_type = graph_type;
	options->kernel_id = kernel_id;
	options->infilename = infilename;
	options->outfilename = outfilename;

//	printf("%s\n%s\n%d\n%d\n", infilename, outfilename, graph_type, kernel_id);

//	return 0;

}

int64_t dwrite (double *array, int64_t size, int64_t number, int64_t offset, char *filename)
{
	FILE *fp;
	int64_t i;
	
	if (offset != 0) return -1;

	fp = fopen(filename, "w");

	for (i = 0; i < number; i++)
	{
		fprintf(fp, "%20.16e\n", array[i]);
	}

	fclose(fp);

	return size*number;
}


