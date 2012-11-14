/*
 * Copyright (c) 2003, 2007-11 Matteo Frigo
 * Copyright (c) 2003, 2007-11 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "dft.h"

static const solvtab s =
{
     SOLVTAB(X(dft_indirect_register)),
     SOLVTAB(X(dft_indirect_transpose_register)),
     SOLVTAB(X(dft_rank_geq2_register)),
     SOLVTAB(X(dft_vrank_geq1_register)),
     SOLVTAB(X(dft_buffered_register)),
     SOLVTAB(X(dft_generic_register)),
     SOLVTAB(X(dft_rader_register)),
     SOLVTAB(X(dft_bluestein_register)),
     SOLVTAB(X(dft_nop_register)),
     SOLVTAB(X(ct_generic_register)),
     SOLVTAB(X(ct_genericbuf_register)),
     SOLVTAB_END
};

void X(dft_conf_standard)(planner *p)
{
     X(solvtab_exec)(s, p);
     X(solvtab_exec)(X(solvtab_dft_standard), p);
#if HAVE_SSE2
     if (X(have_simd_sse2)())
	  X(solvtab_exec)(X(solvtab_dft_sse2), p);
#endif
#if HAVE_AVX
     if (X(have_simd_avx)())
	  X(solvtab_exec)(X(solvtab_dft_avx), p);
#endif
#if HAVE_ALTIVEC
     if (X(have_simd_altivec)())
	  X(solvtab_exec)(X(solvtab_dft_altivec), p);
#endif
#if HAVE_NEON
     if (X(have_simd_neon)())
	  X(solvtab_exec)(X(solvtab_dft_neon), p);
#endif
}
