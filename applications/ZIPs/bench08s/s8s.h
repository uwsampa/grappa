#define NMATRICES 2                     /* MK */
#define MATRIXSIZE 500000            /* MN */
//#define MATRIXSIZE 1000000            /* MN */
#define NONZERO 4                       /* ML */
#define DLEN 500                       /* MT */
//#define DLEN 100                       /* MT */
#define PART 50                        /* MD2 */
#define MD1 50		/* Max # steps in each subdivision of last pass*/

/* Macros for timing */
/*#define CPUSEC() (cpused() / (float)sysconf(_SC_CLK_TCK))*/
/*#define RTSEC() (rtclock() / (float)sysconf(_SC_CLK_TCK))*/
#define RTSEC() wall()
#define CPUSEC() cputime()
