
#ifndef _lcg64_h
#define _lcg64_h

#ifndef ANSI_ARGS
#ifdef __STDC__
#define ANSI_ARGS(args) args
#else
#define ANSI_ARGS(args) ()
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

int lcg64_get_rn_int ANSI_ARGS((int *igenptr));
float lcg64_get_rn_flt ANSI_ARGS((int *igenptr));
double lcg64_get_rn_dbl ANSI_ARGS((int *igenptr));
int *lcg64_init_rng ANSI_ARGS((int rng_type,  int gennum, int total_gen,  int seed,
			  int mult));
int lcg64_spawn_rng ANSI_ARGS((int *igenptr, int nspawned, int ***newgens, int checkid) );
int lcg64_get_seed_rng ANSI_ARGS((int *genptr));
int lcg64_free_rng ANSI_ARGS((int *genptr));
int lcg64_pack_rng ANSI_ARGS(( int *genptr, char **buffer));
int *lcg64_unpack_rng ANSI_ARGS(( char *packed));
int lcg64_print_rng ANSI_ARGS(( int *igen));


#ifdef __cplusplus
}
#endif


#endif
