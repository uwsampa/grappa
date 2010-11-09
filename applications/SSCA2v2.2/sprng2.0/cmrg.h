
#ifndef _cmrg_h
#define _cmrg_h

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

int cmrg_get_rn_int ANSI_ARGS((int *igenptr));
float cmrg_get_rn_flt ANSI_ARGS((int *igenptr));
double cmrg_get_rn_dbl ANSI_ARGS((int *igenptr));
int *cmrg_init_rng ANSI_ARGS((int rng_type,  int gennum, int total_gen,  int seed,
			  int mult));
int cmrg_spawn_rng ANSI_ARGS((int *igenptr, int nspawned, int ***newgens, int checkid) );
int cmrg_get_seed_rng ANSI_ARGS((int *genptr));
int cmrg_free_rng ANSI_ARGS((int *genptr));
int cmrg_pack_rng ANSI_ARGS(( int *genptr, char **buffer));
int *cmrg_unpack_rng ANSI_ARGS(( char *packed));
int cmrg_print_rng ANSI_ARGS(( int *igen));


#ifdef __cplusplus
}
#endif


#endif
