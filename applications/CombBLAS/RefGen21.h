/****************************************************************/
/* Parallel Combinatorial BLAS Library (for Graph Computations) */
/* version 1.1 -------------------------------------------------*/
/* date: 12/25/2010 --------------------------------------------*/
/* authors: Aydin Buluc (abuluc@lbl.gov), Adam Lugowski --------*/
/****************************************************************/


/**
 * Deterministic vertex scrambling functions from V2.1 of the reference implementation
 **/

#ifndef _REF_GEN_2_1_H_
#define _REF_GEN_2_1_H_

#ifdef _STDINT_H
	#undef _STDINT_H
#endif
#ifdef _GCC_STDINT_H 	// for cray
	#undef _GCC_STDINT_H // original stdint does #include_next<"/opt/gcc/4.5.2/snos/lib/gcc/x86_64-suse-linux/4.5.2/include/stdint-gcc.h">
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>

#include <vector>
#include <limits>
#include "SpDefs.h"
#include "StackEntry.h"
#include "promote.h"
#include "Isect.h"
#include "HeapEntry.h"
#include "SpImpl.h"
#include "graph500-1.2/generator/graph_generator.h"
#include "graph500-1.2/generator/utils.h"


/* Initiator settings: for faster random number generation, the initiator
 * probabilities are defined as fractions (a = INITIATOR_A_NUMERATOR /
 * INITIATOR_DENOMINATOR, b = c = INITIATOR_BC_NUMERATOR /
 * INITIATOR_DENOMINATOR, d = 1 - a - b - c. */
#define INITIATOR_A_NUMERATOR 5700
#define INITIATOR_BC_NUMERATOR 1900
#define INITIATOR_DENOMINATOR 10000

/* If this macro is defined to a non-zero value, use SPK_NOISE_LEVEL /
 * INITIATOR_DENOMINATOR as the noise parameter to use in introducing noise
 * into the graph parameters.  The approach used is from "A Hitchhiker's Guide
 * to Choosing Parameters of Stochastic Kronecker Graphs" by C. Seshadhri, Ali
 * Pinar, and Tamara G. Kolda (http://arxiv.org/abs/1102.5046v1), except that
 * the adjustment here is chosen based on the current level being processed
 * rather than being chosen randomly. */
#define SPK_NOISE_LEVEL 0
/* #define SPK_NOISE_LEVEL 1000 -- in INITIATOR_DENOMINATOR units */


class RefGen21
{
public:

	/* Spread the two 64-bit numbers into five nonzero values in the correct range (2 parameter version) */
	static void make_mrg_seed_short(uint64_t userseed, uint_fast32_t* seed) 
	{
  		seed[0] = (userseed & 0x3FFFFFFF) + 1;
  		seed[1] = ((userseed >> 30) & 0x3FFFFFFF) + 1;
  		seed[2] = (userseed & 0x3FFFFFFF) + 1;
  		seed[3] = ((userseed >> 30) & 0x3FFFFFFF) + 1;
  		seed[4] = ((userseed >> 60) << 4) + (userseed >> 60) + 1;
	}

	static int generate_4way_bernoulli(mrg_state* st, int level, int nlevels) 
	{
  		/* Generator a pseudorandom number in the range [0, INITIATOR_DENOMINATOR) without modulo bias. */
  		static const uint32_t limit = (UINT32_C(0xFFFFFFFF) % INITIATOR_DENOMINATOR);
  		uint32_t val = mrg_get_uint_orig(st);
  		if (/* Unlikely */ val < limit) {
    			do 
			{
      				val = mrg_get_uint_orig(st);
    			} 
			while (val < limit);
  		}
		#if SPK_NOISE_LEVEL == 0
  		int spk_noise_factor = 0;
		#else
  		int spk_noise_factor = 2 * SPK_NOISE_LEVEL * level / nlevels - SPK_NOISE_LEVEL;
		#endif
  		int adjusted_bc_numerator = INITIATOR_BC_NUMERATOR + spk_noise_factor;
  		val %= INITIATOR_DENOMINATOR;
  		if ((signed)val < adjusted_bc_numerator) return 1;
  		val -= adjusted_bc_numerator;
  		if ((signed)val < adjusted_bc_numerator) return 2;
  		val -= adjusted_bc_numerator;
		#if SPK_NOISE_LEVEL == 0
  		if (val < INITIATOR_A_NUMERATOR) return 0;
		#else
  		if (val < INITIATOR_A_NUMERATOR * (INITIATOR_DENOMINATOR - 2 * INITIATOR_BC_NUMERATOR) / (INITIATOR_DENOMINATOR - 2 * adjusted_bc_numerator)) return 0;
		#endif
		return 3;
	}

	/* Reverse bits in a number; this should be optimized for performance
 	* (including using bit- or byte-reverse intrinsics if your platform has them).
 	* */
	static inline uint64_t bitreverse(uint64_t x) 
	{
		#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
		#define USE_GCC_BYTESWAP /* __builtin_bswap* are in 4.3 but not 4.2 */
		#endif

		#ifdef FAST_64BIT_ARITHMETIC

 		 /* 64-bit code */
		#ifdef USE_GCC_BYTESWAP
  		 x = __builtin_bswap64(x);
		#else
  		 x = (x >> 32) | (x << 32);
  		 x = ((x >> 16) & UINT64_C(0x0000FFFF0000FFFF)) | ((x & UINT64_C(0x0000FFFF0000FFFF)) << 16);
  		 x = ((x >>  8) & UINT64_C(0x00FF00FF00FF00FF)) | ((x & UINT64_C(0x00FF00FF00FF00FF)) <<  8);
		#endif
  		x = ((x >>  4) & UINT64_C(0x0F0F0F0F0F0F0F0F)) | ((x & UINT64_C(0x0F0F0F0F0F0F0F0F)) <<  4);
  		x = ((x >>  2) & UINT64_C(0x3333333333333333)) | ((x & UINT64_C(0x3333333333333333)) <<  2);
 		x = ((x >>  1) & UINT64_C(0x5555555555555555)) | ((x & UINT64_C(0x5555555555555555)) <<  1);
  		return x;

		#else

  		/* 32-bit code */
 		uint32_t h = (uint32_t)(x >> 32);
  		uint32_t l = (uint32_t)(x & UINT32_MAX);
		#ifdef USE_GCC_BYTESWAP
  		 h = __builtin_bswap32(h);
  		 l = __builtin_bswap32(l);
		#else
 		 h = (h >> 16) | (h << 16);
 		 l = (l >> 16) | (l << 16);
		 h = ((h >> 8) & UINT32_C(0x00FF00FF)) | ((h & UINT32_C(0x00FF00FF)) << 8);
 		 l = ((l >> 8) & UINT32_C(0x00FF00FF)) | ((l & UINT32_C(0x00FF00FF)) << 8);
		#endif
  		h = ((h >> 4) & UINT32_C(0x0F0F0F0F)) | ((h & UINT32_C(0x0F0F0F0F)) << 4);
  		l = ((l >> 4) & UINT32_C(0x0F0F0F0F)) | ((l & UINT32_C(0x0F0F0F0F)) << 4);
  		h = ((h >> 2) & UINT32_C(0x33333333)) | ((h & UINT32_C(0x33333333)) << 2);
  		l = ((l >> 2) & UINT32_C(0x33333333)) | ((l & UINT32_C(0x33333333)) << 2);
  		h = ((h >> 1) & UINT32_C(0x55555555)) | ((h & UINT32_C(0x55555555)) << 1);
  		l = ((l >> 1) & UINT32_C(0x55555555)) | ((l & UINT32_C(0x55555555)) << 1);
  		return ((uint64_t)l << 32) | h; /* Swap halves */

		#endif
	}


	/* Apply a permutation to scramble vertex numbers; a randomly generated
 	* permutation is not used because applying it at scale is too expensive. */
	static inline int64_t scramble(int64_t v0, int lgN, uint64_t val0, uint64_t val1) 
	{
  		uint64_t v = (uint64_t)v0;
  		v += val0 + val1;
  		v *= (val0 | UINT64_C(0x4519840211493211));
 	 	v = (RefGen21::bitreverse(v) >> (64 - lgN));
  		assert ((v >> lgN) == 0);
  		v *= (val1 | UINT64_C(0x3050852102C843A5));
  		v = (RefGen21::bitreverse(v) >> (64 - lgN));
  		assert ((v >> lgN) == 0);
  		return (int64_t)v;
	}

	/* Make a single graph edge using a pre-set MRG state. */
	static void make_one_edge(int64_t nverts, int level, int lgN, mrg_state* st, packed_edge* result, uint64_t val0, uint64_t val1) 
	{
  		int64_t base_src = 0, base_tgt = 0;
  		while (nverts > 1) 
		{
    			int square = generate_4way_bernoulli(st, level, lgN);
    			int src_offset = square / 2;
    			int tgt_offset = square % 2;
    			assert (base_src <= base_tgt);
    			if (base_src == base_tgt) 
			{
      				/* Clip-and-flip for undirected graph */
      				if (src_offset > tgt_offset) {
        				int temp = src_offset;
        				src_offset = tgt_offset;
        				tgt_offset = temp;
      				}
    			}
    			nverts /= 2;
    			++level;
    			base_src += nverts * src_offset;
    			base_tgt += nverts * tgt_offset;
  		}
  		write_edge(result,
             		scramble(base_src, lgN, val0, val1),
             		scramble(base_tgt, lgN, val0, val1));
	}

	static inline mrg_state MakeScrambleValues(uint64_t & val0, uint64_t & val1, const uint_fast32_t seed[])
	{
		mrg_state state;
		mrg_seed(&state, seed);
    		mrg_state new_state = state;
    		mrg_skip(&new_state, 50, 7, 0);
    		val0 = mrg_get_uint_orig(&new_state);
    		val0 *= UINT64_C(0xFFFFFFFF);
    		val0 += mrg_get_uint_orig(&new_state);
    		val1 = mrg_get_uint_orig(&new_state);
    		val1 *= UINT64_C(0xFFFFFFFF);
    		val1 += mrg_get_uint_orig(&new_state);
		return state;
	}

	/* Generate a range of edges (from start_edge to end_edge of the total graph),
 	 * writing into elements [0, end_edge - start_edge) of the edges array.  This
 	 * code is parallel on OpenMP, it must be used with separately-implemented SPMD parallelism for MPI. 
	 */
	static void generate_kronecker_range(	const uint_fast32_t seed[5] /* All values in [0, 2^31 - 1), not all zero */,
       					int logN /* In base 2 */, int64_t start_edge, int64_t end_edge, packed_edge* edges) 
	{
  		int64_t nverts = (int64_t)1 << logN;
  		uint64_t val0, val1; /* Values for scrambling */
		mrg_state state = MakeScrambleValues(val0, val1, seed);

		#ifdef _OPENMP
		#pragma omp parallel for
		#endif
  		for (int64_t ei = start_edge; ei < end_edge; ++ei) 
		{
    			mrg_state new_state = state;
 		   	mrg_skip(&new_state, 0, ei, 0);
    			make_one_edge(nverts, 0, logN, &new_state, edges + (ei - start_edge), val0, val1);
  		}
	}
	static inline void compute_edge_range(int rank, int size, int64_t M, int64_t* start_idx, int64_t* end_idx) 
	{
  		int64_t rankc = (int64_t)(rank);
  		int64_t sizec = (int64_t)(size);
  		*start_idx = rankc * (M / sizec) + (rankc < (M % sizec) ? rankc : (M % sizec));
  		*end_idx = (rankc + 1) * (M / sizec) + (rankc + 1 < (M % sizec) ? rankc + 1 : (M % sizec));
	}

	static inline void make_graph(int log_numverts, int64_t M, int64_t* nedges_ptr, packed_edge** result_ptr) 
	{
  		int rank, size;
		uint64_t userseed1 = (uint64_t) init_random();

  		/* Spread the two 64-bit numbers into five nonzero values in the correct range. */
  		uint_fast32_t seed[5];
  		make_mrg_seed(userseed1, userseed1, seed);

  		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  		MPI_Comm_size(MPI_COMM_WORLD, &size);

  		int64_t start_idx, end_idx;
  		compute_edge_range(rank, size, M, &start_idx, &end_idx);
  		int64_t nedges = end_idx - start_idx;

  		packed_edge* local_edges = new packed_edge[nedges];

  		double start = MPI_Wtime();
  		generate_kronecker_range(seed, log_numverts, start_idx, end_idx, local_edges);
 		double gen_time = MPI_Wtime() - start;

  		*result_ptr = local_edges;
  		*nedges_ptr = nedges;

  		if (rank == 0) 
		{
    			fprintf(stdout, "graph_generation:               %f s\n", gen_time);
  		}
	}

	static inline long init_random ()
	{
  		long seed = -1;
  		if (getenv ("SEED")) 
		{
    			errno = 0;
    			seed = strtol (getenv ("SEED"), NULL, 10);
    			if (errno) seed = -1;
  		}

  		if (seed < 0) seed = 0xDECAFBAD;
		return seed;
	}
};

#endif
