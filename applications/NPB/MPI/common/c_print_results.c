/*****************************************************************/
/******     C  _  P  R  I  N  T  _  R  E  S  U  L  T  S     ******/
/*****************************************************************/
#include <stdlib.h>
#include <stdio.h>

void c_print_results( char   *name,
                      char   class,
                      int    n1, 
                      int    n2,
                      int    n3,
                      int    niter,
                      int    nprocs_compiled,
                      int    nprocs_total,
                      double t,
                      double mops,
		      char   *optype,
                      int    passed_verification,
                      char   *npbversion,
                      char   *compiletime,
                      char   *mpicc,
                      char   *clink,
                      char   *cmpi_lib,
                      char   *cmpi_inc,
                      char   *cflags,
                      char   *clinkflags )
{
  char *evalue="1000";

  printf( "\n\n %s Benchmark Completed\n", name ); 

  printf( " Class           =                        %c\n", class );

  if( n3 == 0 ) {
      long nn = n1;
      if ( n2 != 0 ) nn *= n2;
      printf( " problem_size:             %12ld\n", nn );   /* as in IS */
  } else {
    printf( " problem_size:             %3dx %3dx %3d\n", n1,n2,n3 );
  }
  printf( " iterations:                   %12d\n", niter );
  printf( " run_time:                     %12.2f\n", t );
  printf( " nproc:                        %12d\n", nprocs_total );

  if ( nprocs_compiled != 0 )
    printf( " Compiled procs  =             %12d\n", nprocs_compiled );
    printf( " mops_total:                   %12.2f\n", mops );
    printf( " mops_per_process:             %12.2f\n", mops/((float) nprocs_total) );
    printf( " Operation type  = %24s\n", optype);

    if( passed_verification ) {
      printf( " verification    =               SUCCESSFUL\n" );
      printf("verified: 1\n");
    } else {
      printf( " Verification    =             UNSUCCESSFUL\n" );
    }

    printf( " Version         =             %12s\n", npbversion );
    printf( " Compile date    =             %12s\n", compiletime );
    printf( "\n Compile options:\n" );
    printf( "    MPICC        = %s\n", mpicc );
    printf( "    CLINK        = %s\n", clink );
    printf( "    CMPI_LIB     = %s\n", cmpi_lib );
    printf( "    CMPI_INC     = %s\n", cmpi_inc );
    printf( "    CFLAGS       = %s\n", cflags );
    printf( "    CLINKFLAGS   = %s\n", clinkflags );
#ifdef SMP
    evalue = getenv("MP_SET_NUMTHREADS");
    printf( "   MULTICPUS = %s\n", evalue );
#endif

    printf( "\n\n" );
    printf( " Please send feedbacks and/or the results of this run to:\n\n" );
    printf( " NPB Development Team\n" );
    printf( " npb@nas.nasa.gov\n\n\n" );
/*    printf( " Please send the results of this run to:\n\n" );
    printf( " NPB Development Team\n" );
    printf( " Internet: npb@nas.nasa.gov\n \n" );
    printf( " If email is not available, send this to:\n\n" );
    printf( " MS T27A-1\n" );
    printf( " NASA Ames Research Center\n" );
    printf( " Moffett Field, CA  94035-1000\n\n" );
    printf( " Fax: 650-604-3957\n\n" );*/
}
 
