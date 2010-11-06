

#ifndef _sprngf_h_

#define SPRNG_LFG   0
#define SPRNG_LCG   1
#define SPRNG_LCG64 2
#define SPRNG_CMRG  3
#define SPRNG_MLFG  4
#define SPRNG_PMLCG 5
#define DEFAULT_RNG_TYPE SPRNG_LFG

#define SPRNG_DEFAULT 0
#define CRAYLCG 0
#define DRAND48 1
#define FISH1   2
#define FISH2   3
#define FISH3   4
#define FISH4   5
#define FISH5   6
#define LECU1   0
#define LECU2   1
#define LECU3   2
#define LAG1279    0
#define LAG17    1
#define LAG31    2
#define LAG55    3
#define LAG63    4
#define LAG127   5
#define LAG521   6
#define LAG521B  7
#define LAG607   8
#define LAG607B  9
#define LAG1279B 10

#ifdef CHECK_POINTERS
#define CHECK 1
#else
#define CHECK 0
#endif /* ifdef CHECK_POINTERS */

#define MAX_PACKED_LENGTH 24000

#ifdef POINTER_SIZE
#if POINTER_SIZE == 8
#define SPRNG_POINTER integer*8
#else
#define SPRNG_POINTER integer*4
#endif
#else
#define SPRNG_POINTER integer*4
#endif /* ifdef POINTER_SIZE */

#ifdef USE_MPI
#define make_sprng_seed fseed_mpi
#else
#define make_sprng_seed fmake_new_seed
#endif

#endif /* ifdef _sprng_h */

#ifdef USE_MPI
          external fseed_mpi
          integer fseed_mpi
#else
          external fmake_new_seed
          integer fmake_new_seed
#endif

#ifndef DEFAULTINT
#define DEFAULTINT
#endif
#ifndef FLOAT_GEN
#define DBLGEN
#endif

#if defined(SIMPLE_SPRNG)
#undef DEFAULTINT

#ifndef  _sprngf_h_
#define pack_sprng fpack_rng_simple
#define unpack_sprng funpack_rng_simple
#ifdef USE_MPI
#define isprng  fget_rn_int_simmpi
#define init_sprng finit_rng_simmpi
#else
#define isprng  fget_rn_int_sim
#define init_sprng finit_rng_sim
#endif /* ifdef USE_MPI */
#define print_sprng fprint_rng_simple

#if defined(FLOAT_GEN) && defined(USE_MPI)
#define sprng  fget_rn_flt_simmpi
#endif
#if defined(FLOAT_GEN) && !defined(USE_MPI)
#define sprng  fget_rn_flt_sim
#endif
#if defined(DBLGEN) && defined(USE_MPI)
#define sprng  fget_rn_dbl_simmpi
#endif
#if defined(DBLGEN) && !defined(USE_MPI)
#define sprng  fget_rn_dbl_sim
#endif 

#endif /* ifdef _sprng_h */
          external isprng
          external fget_rn_dbl_sim, fget_rn_flt_sim
          external init_sprng, fpack_rng_simple 
          external funpack_rng_simple, fprint_rng_simple 
#ifdef USE_MPI
          external fget_rn_flt_simmpi, fget_rn_dbl_simmpi
          real*4 fget_rn_flt_simmpi
          real*8 fget_rn_dbl_simmpi
#endif
          integer isprng,fpack_rng_simple,fprint_rng_simple 
          SPRNG_POINTER init_sprng, funpack_rng_simple
          real*4 fget_rn_flt_sim
          real*8 fget_rn_dbl_sim
#endif

#if defined(CHECK_POINTERS)
#undef DEFAULTINT
          external fget_rn_int_ptr, fget_rn_flt_ptr, fget_rn_dbl_ptr
          external fspawn_rng_ptr, ffree_rng_ptr, finit_rng_ptr
          external fpack_rng_ptr, funpack_rng_ptr, fprint_rng_ptr

          integer fget_rn_int_ptr, ffree_rng_ptr, fpack_rng_ptr 
          SPRNG_POINTER finit_rng_ptr, funpack_rng_ptr
          integer fspawn_rng_ptr, fprint_rng_ptr
          real*4 fget_rn_flt_ptr
          real*8 fget_rn_dbl_ptr

#ifndef  _sprngf_h_
#define isprng  fget_rn_int_ptr
#define free_sprng ffree_rng_ptr
#define spawn_sprng(A,B,C) fspawn_rng_ptr(A,B,C,CHECK)
#define pack_sprng  fpack_rng_ptr
#define unpack_sprng funpack_rng_ptr
#define init_sprng finit_rng_ptr
#define print_sprng fprint_rng_ptr
#ifdef FLOAT_GEN
#define sprng  fget_rn_flt_ptr
#else
#define sprng  fget_rn_dbl_ptr
#endif
#endif
#endif

#if defined(DEFAULTINT)
          external fget_rn_int, fget_rn_flt, fget_rn_dbl
          external fspawn_rng, ffree_rng, finit_rng
          external fpack_rng, funpack_rng, fprint_rng 

          integer fget_rn_int, ffree_rng, fpack_rng 
          SPRNG_POINTER finit_rng, funpack_rng
          integer fspawn_rng, fprint_rng
          real*4 fget_rn_flt
          real*8 fget_rn_dbl

#ifndef  _sprngf_h_
#define isprng fget_rn_int
#define free_sprng ffree_rng
#define spawn_sprng(A,B,C) fspawn_rng(A,B,C,CHECK)
#define pack_sprng fpack_rng
#define unpack_sprng funpack_rng
#define init_sprng finit_rng
#define print_sprng fprint_rng
#ifdef FLOAT_GEN
#define sprng fget_rn_flt
#else
#define sprng fget_rn_dbl
#endif
#endif

#endif



#ifndef  _sprngf_h_
#define _sprngf_h_
#endif
