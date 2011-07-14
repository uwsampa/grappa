/***************************************************************************

                  COPYRIGHT

The following is a notice of limited availability of the code, and disclaimer
which must be included in the prologue of the code and in all source listings
of the code.

Copyright Notice
 + 2009 University of Chicago

Permission is hereby granted to use, reproduce, prepare derivative works, and
to redistribute to others.  This software was authored by:

Jeff R. Hammond
Leadership Computing Facility
Argonne National Laboratory
Argonne IL 60439 USA
phone: (630) 252-5381
e-mail: jhammond@anl.gov

                  GOVERNMENT LICENSE

Portions of this material resulted from work developed under a U.S.
Government Contract and are subject to the following license: the Government
is granted for itself and others acting on its behalf a paid-up, nonexclusive,
irrevocable worldwide license in this computer software to reproduce, prepare
derivative works, and perform publicly and display publicly.

                  DISCLAIMER

This computer code material was prepared, in part, as an account of work
sponsored by an agency of the United States Government.  Neither the United
States, nor the University of Chicago, nor any of their employees, makes any
warranty express or implied, or assumes any legal liability or responsibility
for the accuracy, completeness, or usefulness of any information, apparatus,
product, or process disclosed, or represents that its use would not infringe
privately owned rights.

 ***************************************************************************/
#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "copy.h"
#include "timer.h"

#define DCOPY2D_N_ F77_FUNC_(dcopy2d_n,DCOPY2D_N)
#define DCOPY2D_U_ F77_FUNC_(dcopy2d_u,DCOPY2D_U)
#define DCOPY1D_N_ F77_FUNC_(dcopy1d_n,DCOPY1D_N)
#define DCOPY1D_U_ F77_FUNC_(dcopy1d_u,DCOPY1D_U)


int main(int argc, char** argv)
{
    unsigned long long timer;
    int dim1  = ( argc>1 ? atoi(argv[1]) : 353 );
    int dim2  = ( argc>2 ? atoi(argv[2]) : 419 );
    int dim3  = ( argc>3 ? atoi(argv[3]) : 467 );
    timer_init();

    printf("\ntesting ARMCI copy routines\n");
#if __STDC_VERSION__ >= 199901L
    printf("\nrestrict keyword is used for C routines\n");
#endif
    printf("\ntimer name '%s'\n", timer_name());

    /*********************************************************/

    double* in1 = malloc( (dim1)           * sizeof(double) );
    double* in2 = malloc( (dim1*dim2)      * sizeof(double) );
    double* in3 = malloc( (dim1*dim2*dim3) * sizeof(double) );

    double* cout1 = malloc( (dim1)           * sizeof(double) );
    double* cout2 = malloc( (dim1*dim2)      * sizeof(double) );
    double* cout3 = malloc( (dim1*dim2*dim3) * sizeof(double) );

    double* fout1 = malloc( (dim1)           * sizeof(double) );
    double* fout2 = malloc( (dim1*dim2)      * sizeof(double) );
    double* fout3 = malloc( (dim1*dim2*dim3) * sizeof(double) );

    int i;

    for ( i=0 ; i<(dim1)           ; i++ ) in1[i] = (double)i;
    for ( i=0 ; i<(dim1*dim2)      ; i++ ) in2[i] = (double)i;
    for ( i=0 ; i<(dim1*dim2*dim3) ; i++ ) in3[i] = (double)i;

    int ccur, fcur;

    /*********************************************************/

    printf("\n");

    for (i=0;i<dim1;i++) cout1[i] = -1.0f;
    for (i=0;i<dim1;i++) fout1[i] = -1.0f;

    timer = timer_start();
    DCOPY1D_N_(in1, fout1, &dim1);
    timer = timer_end(timer);
    printf("  DCOPY1D_N_ = %15llu\n", timer);
    timer = timer_start();
    c_dcopy1d_n_(in1, cout1, &dim1);
    timer = timer_end(timer);
    printf("c_dcopy1d_n_ = %15llu\n", timer);
    for (i=0 ;i<dim1;i++) assert(cout1[i]==fout1[i]);

    printf("\n");

    for (i=0;i<dim1;i++) cout1[i] = -1.0f;
    for (i=0;i<dim1;i++) fout1[i] = -1.0f;

    timer = timer_start();
    DCOPY1D_U_(in1, fout1, &dim1);
    timer = timer_end(timer);
    printf("  DCOPY1D_U_ = %15llu\n", timer);
    timer = timer_start();
    c_dcopy1d_u_(in1, cout1, &dim1);
    timer = timer_end(timer);
    printf("c_dcopy1d_u_ = %15llu\n", timer);
    for (i=0 ;i<dim1;i++) assert(cout1[i]==fout1[i]);

    /*printf("all 1d tests have passed!\n");*/

    /*********************************************************/

    printf("\n");

    for (i=0;i<(dim1*dim2);i++) cout2[i] = -1.0f;
    for (i=0;i<(dim1*dim2);i++) fout2[i] = -1.0f;

    timer = timer_start();
    DCOPY2D_N_(&dim1,&dim2,in2,&dim1,fout2,&dim1);
    timer = timer_end(timer);
    printf("  DCOPY2D_N_ = %15llu\n", timer);
    timer = timer_start();
    c_dcopy2d_n_(&dim1,&dim2,in2,&dim1,cout2,&dim1);
    timer = timer_end(timer);
    printf("c_dcopy2d_n_ = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2);i++) assert(cout2[i]==fout2[i]);

    printf("\n");

    for (i=0;i<(dim1*dim2);i++) cout2[i] = -1.0f;
    for (i=0;i<(dim1*dim2);i++) fout2[i] = -1.0f;

    timer = timer_start();
    DCOPY2D_U_(&dim1,&dim2,in2,&dim1,fout2,&dim1);
    timer = timer_end(timer);
    printf("  DCOPY2D_U_ = %15llu\n", timer);
    timer = timer_start();
    c_dcopy2d_u_(&dim1,&dim2,in2,&dim1,cout2,&dim1);
    timer = timer_end(timer);
    printf("c_dcopy2d_u_ = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2);i++) assert(cout2[i]==fout2[i]);

    printf("\n");

    for (i=0;i<(dim1*dim2);i++) cout2[i] = -1.0f;
    for (i=0;i<(dim1*dim2);i++) fout2[i] = -1.0f;

    timer = timer_start();
    DCOPY21 (&dim1,&dim2,in2,&dim1,fout2,&fcur);
    timer = timer_end(timer);
    printf("   DCOPY21   = %15llu\n", timer);
    timer = timer_start();
    c_dcopy21_(&dim1,&dim2,in2,&dim1,cout2,&ccur);
    timer = timer_end(timer);
    printf("c_dcopy21_   = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2);i++) assert(cout2[i]==fout2[i]);
    assert(ccur==fcur);

    printf("\n");

    for (i=0;i<(dim1*dim2);i++) cout2[i] = -1.0f;
    for (i=0;i<(dim1*dim2);i++) fout2[i] = -1.0f;

    timer = timer_start();
    DCOPY12 (&dim1,&dim2,fout2,&dim1,in2,&fcur);
    timer = timer_end(timer);
    printf("   DCOPY12   = %15llu\n", timer);
    timer = timer_start();
    c_dcopy12_(&dim1,&dim2,cout2,&dim1,in2,&ccur);
    timer = timer_end(timer);
    printf("c_dcopy12_   = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2);i++) assert(cout2[i]==fout2[i]);
    assert(ccur==fcur);

    /*printf("all 2d tests have passed!\n");*/

    /*********************************************************/

    printf("\n");

    for (i=0;i<(dim1*dim2*dim3);i++) cout3[i] = -1.0f;
    for (i=0;i<(dim1*dim2*dim3);i++) fout3[i] = -1.0f;

    timer = timer_start();
    DCOPY31 (&dim1,&dim2,&dim3,in3,&dim1,&dim2,fout3,&fcur);
    timer = timer_end(timer);
    printf("   DCOPY31   = %15llu\n", timer);
    timer = timer_start();
    c_dcopy31_(&dim1,&dim2,&dim3,in3,&dim1,&dim2,cout3,&ccur);
    timer = timer_end(timer);
    printf("c_dcopy31_   = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2*dim3);i++) assert(cout3[i]==fout3[i]);
    assert(ccur==fcur);

    printf("\n");

    for (i=0;i<(dim1*dim2*dim3);i++) cout3[i] = -1.0f;
    for (i=0;i<(dim1*dim2*dim3);i++) fout3[i] = -1.0f;

    timer = timer_start();
    DCOPY13 (&dim1,&dim2,&dim3,fout3,&dim1,&dim2,in3,&fcur);
    timer = timer_end(timer);
    printf("   DCOPY13   = %15llu\n", timer);
    timer = timer_start();
    c_dcopy13_(&dim1,&dim2,&dim3,cout3,&dim1,&dim2,in3,&ccur);
    timer = timer_end(timer);
    printf("c_dcopy13_   = %15llu\n", timer);
    for (i=0 ;i<(dim1*dim2*dim3);i++) assert(cout3[i]==fout3[i]);
    assert(ccur==fcur);

    /*printf("all 3d tests have passed!\n");*/

    /*********************************************************/

    free(in1);
    free(in2);
    free(in3);
    free(cout1);
    free(cout2);
    free(cout3);
    free(fout1);
    free(fout2);
    free(fout3);

    return(0);
}
