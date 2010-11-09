/*************************************************************************/
/*               SPRNG single library version                            */
/*               sprng.c, Wrapper file for rngs                          */
/*                                                                       */ 
/* Author: Mike H. Zhou,                                                 */
/*             University of Southern Mississippi                        */
/* E-Mail: Mike.Zhou@usm.edu                                             */
/* Date: April, 1999                                                     */
/*                                                                       */ 
/* Disclaimer: We expressly disclaim any and all warranties, expressed   */
/* or implied, concerning the enclosed software.  The intent in sharing  */
/* this software is to promote the productive interchange of ideas       */
/* throughout the research community. All software is furnished on an    */
/* "as is" basis. No further updates to this software should be          */
/* expected. Although this may occur, no commitment exists. The authors  */
/* certainly invite your comments as well as the reporting of any bugs.  */
/* We cannot commit that any or all bugs will be fixed.                  */
/*************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "sprng.h"
#include "sprng_interface.h"
#include "store.h"
#include "lfg.h"
#include "lcg.h"
#include "lcg64.h"
#include "cmrg.h"
#include "mlfg.h"
#ifdef USE_PMLCG
#include "pmlcg.h"
#endif

#define NDEBUG
#include <assert.h>

#define VERSION "00"
#define GENTYPE  VERSION "SPRNG Wrapper"

/* 
 *	This struct is used to retrieve "rng_type" from the rng specific 
 *	"struct rngen". RNGs have different definations for "struct rngen",
 *  however, its first field must be the integer "rng_type"
 */
struct rngen
{
	int rng_type;
};

/*
 *	The function tables, the order of the RNG functions in each table 
 *  must conform to that of the macro definations in sprng.h and
 *  sprng_f.h,
 *  #define SPRNG_LFG   0
 *  #define SPRNG_LCG   1
 *  #define SPRNG_LCG64 2
 *  #define SPRNG_CMRG  3
 *  #define SPRNG_MLFG  4
 *  #define SPRNG_PMLCG 5
 */

int *(*init_rng_tbl[])(int rng_type,int gennum,int total_gen,int seed,int mult)\
	= {	lfg_init_rng, \
		lcg_init_rng, \
		lcg64_init_rng, \
		cmrg_init_rng,\
		mlfg_init_rng 
#ifdef USE_PMLCG
		, pmlcg_init_rng
#endif
	};

double (*get_rn_dbl_tbl[])(int *igenptr)\
	={	lfg_get_rn_dbl, \
		lcg_get_rn_dbl, \
		lcg64_get_rn_dbl, \
		cmrg_get_rn_dbl,\
      		mlfg_get_rn_dbl 
#ifdef USE_PMLCG
	  	, pmlcg_get_rn_dbl
#endif
	};

int (*get_rn_int_tbl[])(int *igenptr)\
	={	lfg_get_rn_int, \
		lcg_get_rn_int, \
		lcg64_get_rn_int, \
		cmrg_get_rn_int,\
	  	mlfg_get_rn_int 
#ifdef USE_PMLCG
	  	, pmlcg_get_rn_int
#endif
	};

float (*get_rn_flt_tbl[])(int *igenptr)\
	={	lfg_get_rn_flt, \
		lcg_get_rn_flt, \
		lcg64_get_rn_flt, \
		cmrg_get_rn_flt,\
	  	mlfg_get_rn_flt 
#ifdef USE_PMLCG
	  	, pmlcg_get_rn_flt
#endif
	};

int (*spawn_rng_tbl[])(int *igenptr, int nspawned, int ***newgens, int checkid)\
	={	lfg_spawn_rng, \
		lcg_spawn_rng, \
		lcg64_spawn_rng, \
		cmrg_spawn_rng,\
	  	mlfg_spawn_rng 
#ifdef USE_PMLCG
	  	, pmlcg_spawn_rng
#endif
	};

int (*free_rng_tbl[])(int *genptr)\
	={	lfg_free_rng, \
		lcg_free_rng, \
		lcg64_free_rng, \
		cmrg_free_rng,\
	  	mlfg_free_rng 
#ifdef USE_PMLCG
	  	, pmlcg_free_rng
#endif
	};

int (*pack_rng_tbl[])( int *genptr, char **buffer)\
	={	lfg_pack_rng, \
		lcg_pack_rng, \
		lcg64_pack_rng, \
		cmrg_pack_rng,\
	  	mlfg_pack_rng 
#ifdef USE_PMLCG
	  	, pmlcg_pack_rng
#endif
	};

int *(*unpack_rng_tbl[])( char *packed)\
	={	lfg_unpack_rng, \
		lcg_unpack_rng, \
		lcg64_unpack_rng, \
		cmrg_unpack_rng,\
	  	mlfg_unpack_rng 
#ifdef USE_PMLCG
	  	, pmlcg_unpack_rng
#endif
	};

int (*get_seed_rng_tbl[])(int *gen)\
	={	lfg_get_seed_rng, \
		lcg_get_seed_rng, \
		lcg64_get_seed_rng, \
		cmrg_get_seed_rng,\
	  	mlfg_get_seed_rng 
#ifdef USE_PMLCG
	  	, pmlcg_get_seed_rng
#endif
	};

int (*print_rng_tbl[])( int *igen)\
	={	lfg_print_rng, \
		lcg_print_rng, \
		lcg64_print_rng, \
		cmrg_print_rng,\
		mlfg_print_rng 
#ifdef USE_PMLCG
		, pmlcg_print_rng
#endif
	};


#ifdef __STDC__
int *init_rng(int rng_type, int gennum,  int total_gen,  int seed, int mult)
#else
int *init_rng(rng_type,gennum,total_gen,seed,mult)
int rng_type,gennum,mult,seed,total_gen;
#endif
{
	if (rng_type==SPRNG_LFG 	|| \
		rng_type==SPRNG_LCG 	|| \
		rng_type==SPRNG_LCG64 	||\
		rng_type==SPRNG_CMRG 	|| \
		rng_type==SPRNG_MLFG 	 
#ifdef USE_PMLCG
		|| rng_type==SPRNG_PMLCG
#endif
		)
	{
		return init_rng_tbl[rng_type](rng_type,gennum,total_gen,seed,mult);
	}else{
		fprintf(stderr, \
		"Error: in init_rng, invalid generator type: %d.\n", rng_type);
		return NULL;
	}
}



#ifdef __STDC__
int get_rn_int(int *igenptr)
#else
int get_rn_int(igenptr)
int *igenptr;
#endif
{
	struct rngen * tmpgen = (struct rngen *)igenptr;
 return get_rn_int_tbl[tmpgen->rng_type](igenptr); 
} 

#ifdef __STDC__
float get_rn_flt(int *igenptr)
#else
float get_rn_flt(igenptr)
int *igenptr;
#endif
{
    return get_rn_flt_tbl[((struct rngen *)igenptr)->rng_type](igenptr);
} /* get_rn_float */


#ifdef __STDC__
double get_rn_dbl(int *igenptr)
#else
double get_rn_dbl(igenptr)
int *igenptr;
#endif
{
	 return get_rn_dbl_tbl[((struct rngen *)igenptr)->rng_type](igenptr);
} /* get_rn_dbl */



#ifdef __STDC__
int spawn_rng(int *igenptr,  int nspawned, int ***newgens, int checkid)
#else
int spawn_rng(igenptr,nspawned, newgens, checkid)
int *igenptr,nspawned, ***newgens, checkid;
#endif
{
	 return spawn_rng_tbl[((struct rngen *)igenptr)->rng_type]\
		 (igenptr,nspawned,newgens,checkid);
}



#ifdef __STDC__
int free_rng(int *genptr)
#else
int free_rng(genptr)
int *genptr;
#endif
{
	return free_rng_tbl[((struct rngen *)genptr)->rng_type](genptr);
}


#ifdef __STDC__
int pack_rng( int *genptr, char **buffer)
#else
int pack_rng(genptr,buffer)
int *genptr;
char **buffer;
#endif
{
	return pack_rng_tbl[((struct rngen *)genptr)->rng_type](genptr,buffer);
}



#ifdef __STDC__
int get_seed_rng(int *gen)
#else
int get_seed_rng(gen)
int *gen;
#endif
{
	return get_seed_rng_tbl[((struct rngen *)gen)->rng_type](gen);
}


#ifdef __STDC__
int *unpack_rng(char *packed)
#else
int *unpack_rng(packed)
unsigned char *packed;
#endif
{
	int rng_type;

	load_int((unsigned char *)packed,4,(unsigned int *)&rng_type);
	if (rng_type==SPRNG_LFG 	|| \
		rng_type==SPRNG_LCG 	|| \
		rng_type==SPRNG_LCG64 	||\
		rng_type==SPRNG_CMRG 	|| \
		rng_type==SPRNG_MLFG 	 
#ifdef USE_PMLCG
		|| rng_type==SPRNG_PMLCG
#endif
		){
		
			return unpack_rng_tbl[rng_type](packed);
		} else {
			return NULL;
		}
}

      

#ifdef __STDC__
int print_rng( int *igen)
#else
int print_rng(igen)
int *igen;
#endif
{
	return print_rng_tbl[((struct rngen *)igen)->rng_type](igen);
}


#include "simple_.h"
#include "fwrap_.h"

