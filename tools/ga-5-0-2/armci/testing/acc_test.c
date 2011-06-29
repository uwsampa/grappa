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

#include "acc.h"
#include "timer.h"

#undef C_ACCUMULATE_1D
#undef C_ACCUMULATE_2D
#undef D_ACCUMULATE_1D
#undef D_ACCUMULATE_2D
#undef F_ACCUMULATE_1D
#undef F_ACCUMULATE_2D
#undef I_ACCUMULATE_1D
#undef I_ACCUMULATE_2D
#undef Z_ACCUMULATE_1D
#undef Z_ACCUMULATE_2D

#define C_ACCUMULATE_1D   F77_FUNC_(c_accumulate_1d,C_ACCUMULATE_1D)
#define C_ACCUMULATE_2D   F77_FUNC_(c_accumulate_2d,C_ACCUMULATE_2D)
#define C_ACCUMULATE_2D_U F77_FUNC_(c_accumulate_2d_u,C_ACCUMULATE_2D_U)
#define D_ACCUMULATE_1D   F77_FUNC_(d_accumulate_1d,D_ACCUMULATE_1D)
#define D_ACCUMULATE_2D   F77_FUNC_(d_accumulate_2d,D_ACCUMULATE_2D)
#define D_ACCUMULATE_2D_U F77_FUNC_(d_accumulate_2d_u,D_ACCUMULATE_2D_U)
#define F_ACCUMULATE_1D   F77_FUNC_(f_accumulate_1d,F_ACCUMULATE_1D)
#define F_ACCUMULATE_2D   F77_FUNC_(f_accumulate_2d,F_ACCUMULATE_2D)
#define F_ACCUMULATE_2D_U F77_FUNC_(f_accumulate_2d_u,F_ACCUMULATE_2D_U)
#define I_ACCUMULATE_1D   F77_FUNC_(i_accumulate_1d,I_ACCUMULATE_1D)
#define I_ACCUMULATE_2D   F77_FUNC_(i_accumulate_2d,I_ACCUMULATE_2D)
#define I_ACCUMULATE_2D_U F77_FUNC_(i_accumulate_2d_u,I_ACCUMULATE_2D_U)
#define Z_ACCUMULATE_1D   F77_FUNC_(z_accumulate_1d,Z_ACCUMULATE_1D)
#define Z_ACCUMULATE_2D   F77_FUNC_(z_accumulate_2d,Z_ACCUMULATE_2D)
#define Z_ACCUMULATE_2D_U F77_FUNC_(z_accumulate_2d_u,Z_ACCUMULATE_2D_U)

void d_assert(double *x, double *y, int n);
void f_assert(float *x, float *y, int n);
void i_assert(int *x, int *y, int n);
void z_assert(dcomplex_t *x, dcomplex_t *y, int n);
void c_assert(complex_t *x, complex_t *y, int n);
void d_enum(double *x, int n);
void f_enum(float *x, int n);
void i_enum(int *x, int n);
void z_enum(dcomplex_t *x, int n);
void c_enum(complex_t *x, int n);
void d_fill(double *x, double val, int n);
void f_fill(float *x, float val, int n);
void i_fill(int *x, int val, int n);
void z_fill(dcomplex_t *x, dcomplex_t val, int n);
void c_fill(complex_t *x, complex_t val, int n);


int main(int argc, char** argv)
{
    unsigned long long timer;
    int dim1  = ( argc>1 ? atoi(argv[1]) : 1019 );
    int dim2  = ( argc>2 ? atoi(argv[2]) : 1087 );
    timer_init();

    printf("\ntesting ARMCI accumulate routines\n");
#if __STDC_VERSION__ >= 199901L
    printf("\nrestrict keyword is used for C routines\n");
#endif
    printf("\ntimer name '%s'\n", timer_name());

    /*********************************************************/

    double*     d_in1 = malloc( (dim1)        * sizeof(double) );
    double*     d_in2 = malloc( (dim1*dim2)   * sizeof(double) );
    float*      f_in1 = malloc( (dim1)        * sizeof(float) );
    float*      f_in2 = malloc( (dim1*dim2)   * sizeof(float) );
    int*        i_in1 = malloc( (dim1)        * sizeof(int) );
    int*        i_in2 = malloc( (dim1*dim2)   * sizeof(int) );
    dcomplex_t* z_in1 = malloc( (dim1)        * sizeof(dcomplex_t) );
    dcomplex_t* z_in2 = malloc( (dim1*dim2)   * sizeof(dcomplex_t) );
    complex_t*  c_in1 = malloc( (dim1)        * sizeof(complex_t) );
    complex_t*  c_in2 = malloc( (dim1*dim2)   * sizeof(complex_t) );

    double*     c_d_out1 = malloc( (dim1)      * sizeof(double) );
    double*     c_d_out2 = malloc( (dim1*dim2) * sizeof(double) );
    float*      c_f_out1 = malloc( (dim1)      * sizeof(float) );
    float*      c_f_out2 = malloc( (dim1*dim2) * sizeof(float) );
    int*        c_i_out1 = malloc( (dim1)      * sizeof(int) );
    int*        c_i_out2 = malloc( (dim1*dim2) * sizeof(int) );
    dcomplex_t* c_z_out1 = malloc( (dim1)      * sizeof(dcomplex_t) );
    dcomplex_t* c_z_out2 = malloc( (dim1*dim2) * sizeof(dcomplex_t) );
    complex_t*  c_c_out1 = malloc( (dim1)      * sizeof(complex_t) );
    complex_t*  c_c_out2 = malloc( (dim1*dim2) * sizeof(complex_t) );

    double*     f_d_out1 = malloc( (dim1)      * sizeof(double) );
    double*     f_d_out2 = malloc( (dim1*dim2) * sizeof(double) );
    float*      f_f_out1 = malloc( (dim1)      * sizeof(float) );
    float*      f_f_out2 = malloc( (dim1*dim2) * sizeof(float) );
    int*        f_i_out1 = malloc( (dim1)      * sizeof(int) );
    int*        f_i_out2 = malloc( (dim1*dim2) * sizeof(int) );
    dcomplex_t* f_z_out1 = malloc( (dim1)      * sizeof(dcomplex_t) );
    dcomplex_t* f_z_out2 = malloc( (dim1*dim2) * sizeof(dcomplex_t) );
    complex_t*  f_c_out1 = malloc( (dim1)      * sizeof(complex_t) );
    complex_t*  f_c_out2 = malloc( (dim1*dim2) * sizeof(complex_t) );

    double     d_alpha = 2;
    float      f_alpha = 2;
    int        i_alpha = 2;
    dcomplex_t z_alpha = {2,2};
    complex_t  c_alpha = {2,2};

    double     d_fill_value = -1;
    float      f_fill_value = -1;
    int        i_fill_value = -1;
    dcomplex_t z_fill_value = {-1,-1};
    complex_t  c_fill_value = {-1,-1};

    d_enum(d_in1, dim1);
    f_enum(f_in1, dim1);
    i_enum(i_in1, dim1);
    z_enum(z_in1, dim1);
    c_enum(c_in1, dim1);

    d_enum(d_in2, dim1*dim2);
    f_enum(f_in2, dim1*dim2);
    i_enum(i_in2, dim1*dim2);
    z_enum(z_in2, dim1*dim2);
    c_enum(c_in2, dim1*dim2);

    /*********************************************************/

    printf("\n");
    d_fill(c_d_out1, d_fill_value, dim1);
    d_fill(f_d_out1, d_fill_value, dim1);
    timer = timer_start();
    D_ACCUMULATE_1D(&d_alpha, f_d_out1, d_in1, &dim1);
    timer = timer_end(timer);
    printf("D_ACCUMULATE_1D      =%15llu\n", timer);
    timer = timer_start();
    c_d_accumulate_1d_(&d_alpha, c_d_out1, d_in1, &dim1);
    timer = timer_end(timer);
    printf("c_d_accumulate_1d_   =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    f_fill(c_f_out1, f_fill_value, dim1);
    f_fill(f_f_out1, f_fill_value, dim1);
    timer = timer_start();
    F_ACCUMULATE_1D(&f_alpha, f_f_out1, f_in1, &dim1);
    timer = timer_end(timer);
    printf("F_ACCUMULATE_1D      =%15llu\n", timer);
    timer = timer_start();
    c_f_accumulate_1d_(&f_alpha, c_f_out1, f_in1, &dim1);
    timer = timer_end(timer);
    printf("c_f_accumulate_1d_   =%15llu\n", timer);
    f_assert(c_f_out1, f_f_out1, dim1);

    printf("\n");
    i_fill(c_i_out1, i_fill_value, dim1);
    i_fill(f_i_out1, i_fill_value, dim1);
    timer = timer_start();
    I_ACCUMULATE_1D(&i_alpha, f_i_out1, i_in1, &dim1);
    timer = timer_end(timer);
    printf("I_ACCUMULATE_1D      =%15llu\n", timer);
    timer = timer_start();
    c_i_accumulate_1d_(&i_alpha, c_i_out1, i_in1, &dim1);
    timer = timer_end(timer);
    printf("c_i_accumulate_1d_   =%15llu\n", timer);
    i_assert(c_i_out1, f_i_out1, dim1);

    printf("\n");
    z_fill(c_z_out1, z_fill_value, dim1);
    z_fill(f_z_out1, z_fill_value, dim1);
    timer = timer_start();
    Z_ACCUMULATE_1D(&z_alpha, f_z_out1, z_in1, &dim1);
    timer = timer_end(timer);
    printf("Z_ACCUMULATE_1D      =%15llu\n", timer);
    timer = timer_start();
    c_z_accumulate_1d_(&z_alpha, c_z_out1, z_in1, &dim1);
    timer = timer_end(timer);
    printf("c_z_accumulate_1d_   =%15llu\n", timer);
    z_assert(c_z_out1, f_z_out1, dim1);

    printf("\n");
    c_fill(c_c_out1, c_fill_value, dim1);
    c_fill(f_c_out1, c_fill_value, dim1);
    timer = timer_start();
    C_ACCUMULATE_1D(&c_alpha, f_c_out1, c_in1, &dim1);
    timer = timer_end(timer);
    printf("C_ACCUMULATE_1D      =%15llu\n", timer);
    timer = timer_start();
    c_c_accumulate_1d_(&c_alpha, c_c_out1, c_in1, &dim1);
    timer = timer_end(timer);
    printf("c_c_accumulate_1d_   =%15llu\n", timer);
    c_assert(c_c_out1, f_c_out1, dim1);

    printf("\n");
    d_fill(c_d_out1, d_fill_value, dim1);
    d_fill(f_d_out1, d_fill_value, dim1);
    timer = timer_start();
    FORT_DADD(&dim1, f_d_out1, d_in1);
    timer = timer_end(timer);
    printf("FORT_DADD            =%15llu\n", timer);
    timer = timer_start();
    c_dadd_(&dim1, c_d_out1, d_in1);
    timer = timer_end(timer);
    printf("c_dadd_              =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    d_fill(c_d_out1, d_fill_value, dim1);
    d_fill(f_d_out1, d_fill_value, dim1);
    timer = timer_start();
    FORT_DADD2(&dim1, f_d_out1, d_in1, d_in1);
    timer = timer_end(timer);
    printf("FORT_DADD2           =%15llu\n", timer);
    timer = timer_start();
    c_dadd2_(&dim1, c_d_out1, d_in1, d_in1);
    timer = timer_end(timer);
    printf("c_dadd2_             =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    d_fill(c_d_out1, d_fill_value, dim1);
    d_fill(f_d_out1, d_fill_value, dim1);
    timer = timer_start();
    FORT_DMULT(&dim1, f_d_out1, d_in1);
    timer = timer_end(timer);
    printf("FORT_DMULT           =%15llu\n", timer);
    timer = timer_start();
    c_dmult_(&dim1, c_d_out1, d_in1);
    timer = timer_end(timer);
    printf("c_dmult_             =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    d_fill(c_d_out1, d_fill_value, dim1);
    d_fill(f_d_out1, d_fill_value, dim1);
    timer = timer_start();
    FORT_DMULT2(&dim1, f_d_out1, d_in1, d_in1);
    timer = timer_end(timer);
    printf("FORT_DMULT2          =%15llu\n", timer);
    timer = timer_start();
    c_dmult2_(&dim1, c_d_out1, d_in1, d_in1);
    timer = timer_end(timer);
    printf("c_dmult2_            =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    /*printf("all 1d tests have passed!\n");*/

    /*********************************************************/

    printf("\n");
    d_fill(c_d_out2, d_fill_value, dim1*dim2);
    d_fill(f_d_out2, d_fill_value, dim1*dim2);
    timer = timer_start();
    D_ACCUMULATE_2D(&d_alpha, &dim1, &dim2, f_d_out2, &dim1, d_in2, &dim1);
    timer = timer_end(timer);
    printf("D_ACCUMULATE_2D      =%15llu\n", timer);
    timer = timer_start();
    c_d_accumulate_2d_(&d_alpha, &dim1, &dim2, c_d_out2, &dim1, d_in2, &dim1);
    timer = timer_end(timer);
    printf("c_d_accumulate_2d_   =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    f_fill(c_f_out2, f_fill_value, dim1*dim2);
    f_fill(f_f_out2, f_fill_value, dim1*dim2);
    timer = timer_start();
    F_ACCUMULATE_2D(&f_alpha, &dim1, &dim2, f_f_out2, &dim1, f_in2, &dim1);
    timer = timer_end(timer);
    printf("F_ACCUMULATE_2D      =%15llu\n", timer);
    timer = timer_start();
    c_f_accumulate_2d_(&f_alpha, &dim1, &dim2, c_f_out2, &dim1, f_in2, &dim1);
    timer = timer_end(timer);
    printf("c_f_accumulate_2d_   =%15llu\n", timer);
    f_assert(c_f_out1, f_f_out1, dim1);

    printf("\n");
    i_fill(c_i_out2, i_fill_value, dim1*dim2);
    i_fill(f_i_out2, i_fill_value, dim1*dim2);
    timer = timer_start();
    I_ACCUMULATE_2D(&i_alpha, &dim1, &dim2, f_i_out2, &dim1, i_in2, &dim1);
    timer = timer_end(timer);
    printf("I_ACCUMULATE_2D      =%15llu\n", timer);
    timer = timer_start();
    c_i_accumulate_2d_(&i_alpha, &dim1, &dim2, c_i_out2, &dim1, i_in2, &dim1);
    timer = timer_end(timer);
    printf("c_i_accumulate_2d_   =%15llu\n", timer);
    i_assert(c_i_out1, f_i_out1, dim1);

    printf("\n");
    z_fill(c_z_out2, z_fill_value, dim1*dim2);
    z_fill(f_z_out2, z_fill_value, dim1*dim2);
    timer = timer_start();
    Z_ACCUMULATE_2D(&z_alpha, &dim1, &dim2, f_z_out2, &dim1, z_in2, &dim1);
    timer = timer_end(timer);
    printf("Z_ACCUMULATE_2D      =%15llu\n", timer);
    timer = timer_start();
    c_z_accumulate_2d_(&z_alpha, &dim1, &dim2, c_z_out2, &dim1, z_in2, &dim1);
    timer = timer_end(timer);
    printf("c_z_accumulate_2d_   =%15llu\n", timer);
    z_assert(c_z_out1, f_z_out1, dim1);

    printf("\n");
    c_fill(c_c_out2, c_fill_value, dim1*dim2);
    c_fill(f_c_out2, c_fill_value, dim1*dim2);
    timer = timer_start();
    C_ACCUMULATE_2D(&c_alpha, &dim1, &dim2, f_c_out2, &dim1, c_in2, &dim1);
    timer = timer_end(timer);
    printf("C_ACCUMULATE_2D      =%15llu\n", timer);
    timer = timer_start();
    c_c_accumulate_2d_(&c_alpha, &dim1, &dim2, c_c_out2, &dim1, c_in2, &dim1);
    timer = timer_end(timer);
    printf("c_c_accumulate_2d_   =%15llu\n", timer);
    c_assert(c_c_out1, f_c_out1, dim1);

    printf("\n");
    d_fill(c_d_out2, d_fill_value, dim1*dim2);
    d_fill(f_d_out2, d_fill_value, dim1*dim2);
    timer = timer_start();
    D_ACCUMULATE_2D_U(&d_alpha, &dim1, &dim2, f_d_out2, &dim1, d_in2, &dim1);
    timer = timer_end(timer);
    printf("D_ACCUMULATE_2D_U    =%15llu\n", timer);
    timer = timer_start();
    c_d_accumulate_2d_u_(&d_alpha, &dim1, &dim2, c_d_out2, &dim1, d_in2, &dim1);
    timer = timer_end(timer);
    printf("c_d_accumulate_2d_u_ =%15llu\n", timer);
    d_assert(c_d_out1, f_d_out1, dim1);

    printf("\n");
    f_fill(c_f_out2, f_fill_value, dim1*dim2);
    f_fill(f_f_out2, f_fill_value, dim1*dim2);
    timer = timer_start();
    F_ACCUMULATE_2D_U(&f_alpha, &dim1, &dim2, f_f_out2, &dim1, f_in2, &dim1);
    timer = timer_end(timer);
    printf("F_ACCUMULATE_2D_U    =%15llu\n", timer);
    timer = timer_start();
    c_f_accumulate_2d_u_(&f_alpha, &dim1, &dim2, c_f_out2, &dim1, f_in2, &dim1);
    timer = timer_end(timer);
    printf("c_f_accumulate_2d_u_ =%15llu\n", timer);
    f_assert(c_f_out1, f_f_out1, dim1);

    printf("\n");
    i_fill(c_i_out2, i_fill_value, dim1*dim2);
    i_fill(f_i_out2, i_fill_value, dim1*dim2);
    timer = timer_start();
    I_ACCUMULATE_2D_U(&i_alpha, &dim1, &dim2, f_i_out2, &dim1, i_in2, &dim1);
    timer = timer_end(timer);
    printf("I_ACCUMULATE_2D_U    =%15llu\n", timer);
    timer = timer_start();
    c_i_accumulate_2d_u_(&i_alpha, &dim1, &dim2, c_i_out2, &dim1, i_in2, &dim1);
    timer = timer_end(timer);
    printf("c_i_accumulate_2d_u_ =%15llu\n", timer);
    i_assert(c_i_out1, f_i_out1, dim1);

    printf("\n");
    z_fill(c_z_out2, z_fill_value, dim1*dim2);
    z_fill(f_z_out2, z_fill_value, dim1*dim2);
    timer = timer_start();
    Z_ACCUMULATE_2D_U(&z_alpha, &dim1, &dim2, f_z_out2, &dim1, z_in2, &dim1);
    timer = timer_end(timer);
    printf("Z_ACCUMULATE_2D_U    =%15llu\n", timer);
    timer = timer_start();
    c_z_accumulate_2d_u_(&z_alpha, &dim1, &dim2, c_z_out2, &dim1, z_in2, &dim1);
    timer = timer_end(timer);
    printf("c_z_accumulate_2d_u_ =%15llu\n", timer);
    z_assert(c_z_out1, f_z_out1, dim1);

    printf("\n");
    c_fill(c_c_out2, c_fill_value, dim1*dim2);
    c_fill(f_c_out2, c_fill_value, dim1*dim2);
    timer = timer_start();
    C_ACCUMULATE_2D_U(&c_alpha, &dim1, &dim2, f_c_out2, &dim1, c_in2, &dim1);
    timer = timer_end(timer);
    printf("C_ACCUMULATE_2D_U    =%15llu\n", timer);
    timer = timer_start();
    c_c_accumulate_2d_u_(&c_alpha, &dim1, &dim2, c_c_out2, &dim1, c_in2, &dim1);
    timer = timer_end(timer);
    printf("c_c_accumulate_2d_u_ =%15llu\n", timer);
    c_assert(c_c_out1, f_c_out1, dim1);

    /*printf("all 2d tests have passed!\n");*/

    /*********************************************************/

    free(d_in1);
    free(d_in2);
    free(f_in1);
    free(f_in2);
    free(z_in1);
    free(z_in2);
    free(c_in1);
    free(c_in2);
    free(i_in1);
    free(i_in2);

    free(c_d_out1);
    free(c_d_out2);
    free(c_f_out1);
    free(c_f_out2);
    free(c_z_out1);
    free(c_z_out2);
    free(c_c_out1);
    free(c_c_out2);
    free(c_i_out1);
    free(c_i_out2);

    free(f_d_out1);
    free(f_d_out2);
    free(f_f_out1);
    free(f_f_out2);
    free(f_z_out1);
    free(f_z_out2);
    free(f_c_out1);
    free(f_c_out2);
    free(f_i_out1);
    free(f_i_out2);

    return(0);
}


void d_assert(double *x, double *y, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        assert(x[i]==y[i]);
    }
}

void f_assert(float *x, float *y, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        assert(x[i]==y[i]);
    }
}

void i_assert(int *x, int *y, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        assert(x[i]==y[i]);
    }
}

void z_assert(dcomplex_t *x, dcomplex_t *y, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        assert(x[i].real==y[i].real);
        assert(x[i].imag==y[i].imag);
    }
}

void c_assert(complex_t *x, complex_t *y, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        assert(x[i].real==y[i].real);
        assert(x[i].imag==y[i].imag);
    }
}

void d_enum(double *x, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = i;
    }
}

void f_enum(float *x, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = i;
    }
}

void i_enum(int *x, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = i;
    }
}

void z_enum(dcomplex_t *x, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i].real = i;
        x[i].imag = i;
    }
}

void c_enum(complex_t *x, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i].real = i;
        x[i].imag = i;
    }
}


void d_fill(double *x, double val, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = val;
    }
}

void f_fill(float *x, float val, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = val;
    }
}

void i_fill(int *x, int val, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i] = val;
    }
}

void z_fill(dcomplex_t *x, dcomplex_t val, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i].real = val.real;
        x[i].imag = val.imag;
    }
}

void c_fill(complex_t *x, complex_t val, int n)
{
    int i;
    for ( i=0 ; i<n ; i++ ) {
        x[i].real = val.real;
        x[i].imag = val.imag;
    }
}
