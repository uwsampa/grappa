#ifndef _ACC_H_
#define _ACC_H_

/* since all of this code replaces Fortran, it should never improperly alias
 * vectors */
#if __STDC_VERSION__ >= 199901L
    #define MAYBE_RESTRICT restrict
#else
    #define MAYBE_RESTRICT
#endif

typedef struct {
    float real;
    float imag;
} complex_t;

typedef struct {
    double real;
    double imag;
} dcomplex_t;

void c_d_accumulate_1d_(const double* const alpha,
                        double* MAYBE_RESTRICT A,
                        double* const B,
                        const int* const rows);
void c_f_accumulate_1d_(const float* const alpha,
                        float* MAYBE_RESTRICT A,
                        float* const B,
                        const int* const rows);
void c_c_accumulate_1d_(const complex_t* const alpha,
                        complex_t* MAYBE_RESTRICT A,
                        complex_t* const B,
                        const int* const rows);
void c_z_accumulate_1d_(const dcomplex_t* const alpha,
                        dcomplex_t* MAYBE_RESTRICT A,
                        dcomplex_t* const B,
                        const int* const rows);
void c_i_accumulate_1d_(const int* const alpha,
                        int* MAYBE_RESTRICT A,
                        int* const B,
                        const int* const rows);
void c_l_accumulate_1d_(const long* const alpha,
                        long* MAYBE_RESTRICT A,
                        long* const B,
                        const int* const rows);
void c_ll_accumulate_1d_(const long long* const alpha,
                        long long* MAYBE_RESTRICT A,
                        long long* const B,
                        const int* const rows);

void c_d_accumulate_2d_(const double* const alpha,
                        const int* const rows,
                        const int* const cols,
                        double* MAYBE_RESTRICT A,
                        const int* const ald,
                        const double* const B,
                        const int* const bld);
void c_f_accumulate_2d_(const float* const alpha,
                        const int* const rows,
                        const int* const cols,
                        float* MAYBE_RESTRICT A,
                        const int* const ald,
                        const float* const B,
                        const int* const bld);
void c_c_accumulate_2d_(const complex_t* const alpha,
                        const int* const rows,
                        const int* const cols,
                        complex_t* MAYBE_RESTRICT A,
                        const int* const ald,
                        const complex_t* const B,
                        const int* const bld);
void c_z_accumulate_2d_(const dcomplex_t* const alpha,
                        const int* const rows,
                        const int* const cols,
                        dcomplex_t* MAYBE_RESTRICT A,
                        const int* const ald,
                        const dcomplex_t* const B,
                        const int* const bld);
void c_i_accumulate_2d_(const int* const alpha,
                        const int* const rows,
                        const int* const cols,
                        int* MAYBE_RESTRICT A,
                        const int* const ald,
                        const int* const B,
                        const int* const bld);
void c_l_accumulate_2d_(const long* const alpha,
                        const int* const rows,
                        const int* const cols,
                        long* MAYBE_RESTRICT A,
                        const int* const ald,
                        const long* const B,
                        const int* const bld);
void c_ll_accumulate_2d_(const long long* const alpha,
                        const int* const rows,
                        const int* const cols,
                        long long* MAYBE_RESTRICT A,
                        const int* const ald,
                        const long long* const B,
                        const int* const bld);

void c_d_accumulate_2d_u_(const double* const alpha,
                        const int* const rows,
                        const int* const cols,
                        double* MAYBE_RESTRICT A,
                        const int* const ald,
                        const double* const B,
                        const int* const bld);
void c_f_accumulate_2d_u_(const float* const alpha,
                        const int* const rows,
                        const int* const cols,
                        float* MAYBE_RESTRICT A,
                        const int* const ald,
                        const float* const B,
                        const int* const bld);
void c_c_accumulate_2d_u_(const complex_t* const alpha,
                        const int* const rows,
                        const int* const cols,
                        complex_t* MAYBE_RESTRICT A,
                        const int* const ald,
                        const complex_t* const B,
                        const int* const bld);
void c_z_accumulate_2d_u_(const dcomplex_t* const alpha,
                        const int* const rows,
                        const int* const cols,
                        dcomplex_t* MAYBE_RESTRICT A,
                        const int* const ald,
                        const dcomplex_t* const B,
                        const int* const bld);
void c_i_accumulate_2d_u_(const int* const alpha,
                        const int* const rows,
                        const int* const cols,
                        int* MAYBE_RESTRICT A,
                        const int* const ald,
                        const int* const B,
                        const int* const bld);
void c_l_accumulate_2d_u_(const long* const alpha,
                        const int* const rows,
                        const int* const cols,
                        long* MAYBE_RESTRICT A,
                        const int* const ald,
                        const long* const B,
                        const int* const bld);
void c_ll_accumulate_2d_u_(const long long* const alpha,
                        const int* const rows,
                        const int* const cols,
                        long long* MAYBE_RESTRICT A,
                        const int* const ald,
                        const long long* const B,
                        const int* const bld);

void c_dadd_(const int* const n,
            double* MAYBE_RESTRICT x,
            double* const work);
void c_dadd2_(const int* const n,
            double* MAYBE_RESTRICT x,
            double* const work,
            double* const work2);
void c_dmult_(const int* const n,
            double* MAYBE_RESTRICT x,
            double* const work);
void c_dmult2_(const int* const n,
            double* MAYBE_RESTRICT x,
            double* const work,
            double* const work2);

#if NOFORT
#   define ATR
#   define I_ACCUMULATE_1D c_i_accumulate_1d_
#   define L_ACCUMULATE_1D c_l_accumulate_1d_
#   define D_ACCUMULATE_1D c_d_accumulate_1d_
#   define C_ACCUMULATE_1D c_c_accumulate_1d_
#   define Z_ACCUMULATE_1D c_z_accumulate_1d_
#   define F_ACCUMULATE_1D c_f_accumulate_1d_
#   define I_ACCUMULATE_2D c_i_accumulate_2d_
#   define L_ACCUMULATE_2D c_l_accumulate_2d_
#   define D_ACCUMULATE_2D c_d_accumulate_2d_
#   define C_ACCUMULATE_2D c_c_accumulate_2d_
#   define Z_ACCUMULATE_2D c_z_accumulate_2d_
#   define F_ACCUMULATE_2D c_f_accumulate_2d_
#   define FORT_DADD   c_dadd_
#   define FORT_DADD2  c_dadd2_
#   define FORT_DMULT  c_dmult_
#   define FORT_DMULT2 c_dmult2_
#else /* !NOFORT */
#   ifdef WIN32
#       define ATR __stdcall
#   else
#       define ATR
#   endif
#   if defined(AIX) || defined(BGML) || defined(SGI_)
#       define I_ACCUMULATE_2D F77_FUNC_(i_accumulate_2d_u,I_ACCUMULATE_2D_U)
#       define L_ACCUMULATE_2D         c_l_accumulate_2d_u_
#       define D_ACCUMULATE_2D F77_FUNC_(d_accumulate_2d_u,D_ACCUMULATE_2D_U)
#       define C_ACCUMULATE_2D F77_FUNC_(c_accumulate_2d_u,C_ACCUMULATE_2D_U)
#       define Z_ACCUMULATE_2D F77_FUNC_(z_accumulate_2d_u,Z_ACCUMULATE_2D_U)
#       define F_ACCUMULATE_2D F77_FUNC_(f_accumulate_2d_u,F_ACCUMULATE_2D_U)
#   else
#       define I_ACCUMULATE_2D F77_FUNC_(i_accumulate_2d,I_ACCUMULATE_2D)
#       define L_ACCUMULATE_2D         c_l_accumulate_2d_
#       define D_ACCUMULATE_2D F77_FUNC_(d_accumulate_2d,D_ACCUMULATE_2D)
#       define C_ACCUMULATE_2D F77_FUNC_(c_accumulate_2d,C_ACCUMULATE_2D)
#       define Z_ACCUMULATE_2D F77_FUNC_(z_accumulate_2d,Z_ACCUMULATE_2D)
#       define F_ACCUMULATE_2D F77_FUNC_(f_accumulate_2d,F_ACCUMULATE_2D)
#   endif
#   if defined(CRAY) && !defined(__crayx1)
#       undef  D_ACCUMULATE_2D 
#       define D_ACCUMULATE_2D F77_FUNC_(daxpy_2d,DAXPY_2D)
#   endif
#   define I_ACCUMULATE_1D F77_FUNC_(i_accumulate_1d,I_ACCUMULATE_1D)
#   define L_ACCUMULATE_1D         c_l_accumulate_1d_
#   define D_ACCUMULATE_1D F77_FUNC_(d_accumulate_1d,D_ACCUMULATE_1D)
#   define C_ACCUMULATE_1D F77_FUNC_(c_accumulate_1d,C_ACCUMULATE_1D)
#   define Z_ACCUMULATE_1D F77_FUNC_(z_accumulate_1d,Z_ACCUMULATE_1D)
#   define F_ACCUMULATE_1D F77_FUNC_(f_accumulate_1d,F_ACCUMULATE_1D)
#   define FORT_DADD   F77_FUNC_(fort_dadd,FORT_DADD)
#   define FORT_DADD2  F77_FUNC_(fort_dadd2,FORT_DADD2)
#   define FORT_DMULT  F77_FUNC_(fort_dmult,FORT_DMULT)
#   define FORT_DMULT2 F77_FUNC_(fort_dmult2,FORT_DMULT2)

void ATR I_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*);
/* void ATR L_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*); */
void ATR D_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*);
void ATR C_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*);
void ATR Z_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*);
void ATR F_ACCUMULATE_2D(void*, int*, int*, void*, int*, void*, int*);
void ATR I_ACCUMULATE_1D(void*, void*, void*, int*);
/* void ATR L_ACCUMULATE_1D(void*, void*, void*, int*); */
void ATR D_ACCUMULATE_1D(void*, void*, void*, int*);
void ATR C_ACCUMULATE_1D(void*, void*, void*, int*);
void ATR Z_ACCUMULATE_1D(void*, void*, void*, int*);
void ATR F_ACCUMULATE_1D(void*, void*, void*, int*);
void ATR FORT_DADD(int*, void*, void*);
void ATR FORT_DADD2(int*, void*, void*, void*);
void ATR FORT_DMULT(int*, void*, void*);
void ATR FORT_DMULT2(int*, void*, void*, void*);

#endif /* !NOFORT */

// specific to src-portals
#if defined(AIX) || defined(NOUNDERSCORE)
#   define RA_ACCUMULATE_2D ra_accumulate_2d_u
#elif defined(BGML)
#   define RA_ACCUMULATE_2D ra_accumulate_2d_u__
#elif defined(SGI_)
#   define RA_ACCUMULATE_2D RA_ACCUMULATE_2D_
#elif !defined(CRAY) && !defined(WIN32) && !defined(HITACHI) ||defined(__crayx1)
#   define RA_ACCUMULATE_2D RA_ACCUMULATE_2D_
#endif

#ifndef CRAY_T3E
void ATR RA_ACCUMULATE_2D(long*, int*, int*, long*, int*, long*, int*);
#else
#define RA_ACCUMULATE_2D RA_ACCUMULATE_2D_
void RA_ACCUMULATE_2D_(long*, int*, int*, long*, int*, long*, int*);
#endif

#endif /* _ACC_H_ */
