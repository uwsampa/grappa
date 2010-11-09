
#ifndef _mlfg_h
#define _mlfg_h

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

int mlfg_get_rn_int ANSI_ARGS((int *igenptr));
float mlfg_get_rn_flt ANSI_ARGS((int *igenptr));
double mlfg_get_rn_dbl ANSI_ARGS((int *igenptr));
int *mlfg_init_rng ANSI_ARGS((int rng_type,  int gennum, int total_gen,  int seed,
			  int mult));
int mlfg_spawn_rng ANSI_ARGS((int *igenptr, int nspawned, int ***newgens, int checkid) );
int mlfg_get_seed_rng ANSI_ARGS((int *genptr));
int mlfg_free_rng ANSI_ARGS((int *genptr));
int mlfg_pack_rng ANSI_ARGS(( int *genptr, char **buffer));
int *mlfg_unpack_rng ANSI_ARGS(( char *packed));
int mlfg_print_rng ANSI_ARGS(( int *igen));


#ifdef __cplusplus
}
#endif


#endif
