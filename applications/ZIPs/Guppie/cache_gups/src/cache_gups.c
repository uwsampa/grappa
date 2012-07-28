#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <sys/unistd.h>
#include <sys/time.h>

/* LUPS Code - For the Power7, use the following arguments: */
/* ./a.out -T16384 [for L1 UPS since POWER7 L1 is 32 KB */
/* ./a.out -T131072 [for L2 UPS since POWER7 L2 is 256 KB */
/* ./a.out -T2097152 [for L3 UPS since POWER7 L3 is 4MB */
/* The number of updates is controlled via -U */


#define LCG_MUL64 6364136223846793005ULL
#define LCG_ADD64 1

/* Types used by program (should be 64 bits) */
typedef unsigned long long uint64;
typedef unsigned long long u64Int;
typedef long long s64Int;

/* Log size of main table (suggested: half of global memory) */
#define LTABSIZE 18
uint64 TABSIZE = (1L << LTABSIZE);
uint64 NUPDATE = (1 << 19);

uint64 *globalTable;

u64Int
HPCC_starts(s64Int n)
{
    u64Int mul_k, add_k, ran, un;
    
    mul_k = LCG_MUL64;
    add_k = LCG_ADD64;
    
    ran = 1;
    for (un = (u64Int)n; un; un >>= 1) 
        {
            if (un & 1)
                ran = mul_k * ran + add_k;
            add_k *= (mul_k + 1);
            mul_k *= mul_k;
        }
    
    return(ran);
}

double
GetTime ()
{
    struct timeval tp;
    (void) gettimeofday( &tp, NULL );
    return (tp.tv_sec + ((double) tp.tv_usec)/1000000.0);
}

uint64 body_core(int num_uops) 
{
    register u64Int ran1;
    register uint64 *table;
    register uint64 TABMASK;
    register uint64 i;

    ran1    = HPCC_starts(random());
    table   = globalTable;
    TABMASK = TABSIZE - 1;
    
    for (i = 0; i < num_uops; i += 8) 
	{
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	    ran1 = LCG_MUL64 * ran1 + LCG_ADD64; table[ran1 & TABMASK] ^= ran1;
	}

    return(ran1 + table[0]);
}

int main(int argc, char *argv[])
{
    uint64 i;
    uint64 num_updates;
    int c;
    double start_second, end_second, elapsed_seconds, sum;

    /* Parse command line */
    while ((c = getopt(argc, argv, "T:U:h")) != -1) {
        switch (c) {
        case 'T': TABSIZE  = strtol(optarg, 0, 0); break;
        case 'U': NUPDATE  = strtol(optarg, 0, 0); break;
        case 'h':
            exit(0);
        }
    }

    /* Print parameters for run */
    fprintf(stdout, "Table = %p\n", globalTable);
    fprintf(stdout, "  main table size   = %ld bytes\n", TABSIZE);
    fprintf(stdout, "  Number of updates = %ld\n", NUPDATE);
    
    if ((TABSIZE & (TABSIZE - 1)))
    {
        fprintf (stdout, "TABSIZE must be a power of 2\n");
	exit (1);
    }
    TABSIZE >>= 3;	// So we have it in terms of uint64's
        

    /* Allocate memory */
    globalTable = valloc(sizeof(uint64) * TABSIZE);

    /* init the table with some value */
    for (i = 0; i < TABSIZE; i++) {
        globalTable[i] = i;
    }
    fprintf(stdout, "Init done\n");
    
    num_updates = NUPDATE;

    /* To warm up everything */
    sum = body_core(num_updates);
    fprintf(stdout, "Warm up done\n");

    /* Only count the second time around */
    start_second = GetTime();
    sum += body_core(num_updates);
    end_second = GetTime();

    elapsed_seconds = end_second - start_second;
    fprintf(stdout, "Test done\n");
    fprintf(stdout, "Rate: %5f GUPS (%e), sum=%e\n",
            num_updates * 1e-9 / elapsed_seconds, elapsed_seconds, sum);
}
