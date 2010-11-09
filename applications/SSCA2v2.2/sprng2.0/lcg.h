
#ifndef _lcg_h
#define _lcg_h

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

int lcg_get_rn_int ANSI_ARGS((int *igenptr));
float lcg_get_rn_flt ANSI_ARGS((int *igenptr));
double lcg_get_rn_dbl ANSI_ARGS((int *igenptr));
int *lcg_init_rng ANSI_ARGS((int rng_type,  
			int gennum, int total_gen,  int seed, int mult));
int lcg_spawn_rng ANSI_ARGS((int *igenptr, int nspawned, 
			int ***newgens, int checkid) );
int lcg_get_seed_rng ANSI_ARGS((int *genptr));
int lcg_free_rng ANSI_ARGS((int *genptr));
int lcg_pack_rng ANSI_ARGS(( int *genptr, char **buffer));
int *lcg_unpack_rng ANSI_ARGS(( char *packed));
int lcg_print_rng ANSI_ARGS(( int *igen));


#ifdef __cplusplus
}
#endif


#endif
