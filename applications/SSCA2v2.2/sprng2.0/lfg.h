
#ifndef _lfg_h
#define _lfg_h

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

int lfg_get_rn_int ANSI_ARGS((int *igenptr));
float lfg_get_rn_flt ANSI_ARGS((int *igenptr));
double lfg_get_rn_dbl ANSI_ARGS((int *igenptr));
int *lfg_init_rng ANSI_ARGS((int rng_type,  int gennum, int total_gen,  int seed,
			  int mult));
int lfg_spawn_rng ANSI_ARGS((int *igenptr, int nspawned, int ***newgens, int checkid) );
int lfg_get_seed_rng ANSI_ARGS((int *genptr));
int lfg_free_rng ANSI_ARGS((int *genptr));
int lfg_pack_rng ANSI_ARGS(( int *genptr, char **buffer));
int *lfg_unpack_rng ANSI_ARGS(( char *packed));
int lfg_print_rng ANSI_ARGS(( int *igen));


#ifdef __cplusplus
}
#endif


#endif
