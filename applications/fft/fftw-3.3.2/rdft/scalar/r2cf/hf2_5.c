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

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sat Apr 28 11:02:58 EDT 2012 */

#include "codelet-rdft.h"

#ifdef HAVE_FMA

/* Generated by: ../../../genfft/gen_hc2hc.native -fma -reorder-insns -schedule-for-pipeline -compact -variables 4 -pipeline-latency 4 -twiddle-log3 -precompute-twiddles -n 5 -dit -name hf2_5 -include hf.h */

/*
 * This function contains 44 FP additions, 40 FP multiplications,
 * (or, 14 additions, 10 multiplications, 30 fused multiply/add),
 * 47 stack variables, 4 constants, and 20 memory accesses
 */
#include "hf.h"

static void hf2_5(R *cr, R *ci, const R *W, stride rs, INT mb, INT me, INT ms)
{
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP618033988, +0.618033988749894848204586834365638117720309180);
     {
	  INT m;
	  for (m = mb, W = W + ((mb - 1) * 4); m < me; m = m + 1, cr = cr + ms, ci = ci - ms, W = W + 4, MAKE_VOLATILE_STRIDE(rs)) {
	       E Ta, T1, TL, Tp, TT, Ti, TM, TC, To, TE, Ts, TF, T2, T8, T5;
	       E TS, Tt, TG;
	       T2 = W[0];
	       Ta = W[3];
	       T8 = W[2];
	       T5 = W[1];
	       {
		    E Tq, Tr, Te, T9;
		    T1 = cr[0];
		    Te = T2 * Ta;
		    T9 = T2 * T8;
		    TL = ci[0];
		    {
			 E T3, Tf, Tm, Tj, Tb, T4, T6, Tc, Tg;
			 T3 = cr[WS(rs, 1)];
			 Tf = FMA(T5, T8, Te);
			 Tm = FNMS(T5, T8, Te);
			 Tj = FMA(T5, Ta, T9);
			 Tb = FNMS(T5, Ta, T9);
			 T4 = T2 * T3;
			 T6 = ci[WS(rs, 1)];
			 Tc = cr[WS(rs, 4)];
			 Tg = ci[WS(rs, 4)];
			 {
			      E Tk, Tl, Tn, TD;
			      {
				   E T7, Tz, Th, TB, Ty, Td, TA;
				   Tk = cr[WS(rs, 2)];
				   T7 = FMA(T5, T6, T4);
				   Ty = T2 * T6;
				   Td = Tb * Tc;
				   TA = Tb * Tg;
				   Tl = Tj * Tk;
				   Tz = FNMS(T5, T3, Ty);
				   Th = FMA(Tf, Tg, Td);
				   TB = FNMS(Tf, Tc, TA);
				   Tn = ci[WS(rs, 2)];
				   Tp = cr[WS(rs, 3)];
				   TT = Th - T7;
				   Ti = T7 + Th;
				   TM = Tz + TB;
				   TC = Tz - TB;
				   TD = Tj * Tn;
				   Tq = T8 * Tp;
				   Tr = ci[WS(rs, 3)];
			      }
			      To = FMA(Tm, Tn, Tl);
			      TE = FNMS(Tm, Tk, TD);
			 }
		    }
		    Ts = FMA(Ta, Tr, Tq);
		    TF = T8 * Tr;
	       }
	       TS = To - Ts;
	       Tt = To + Ts;
	       TG = FNMS(Ta, Tp, TF);
	       {
		    E TU, TW, TV, TR, Tw, Tu;
		    TU = FMA(KP618033988, TT, TS);
		    TW = FNMS(KP618033988, TS, TT);
		    Tw = Ti - Tt;
		    Tu = Ti + Tt;
		    {
			 E TN, TH, Tv, TI, TK;
			 TN = TE + TG;
			 TH = TE - TG;
			 cr[0] = T1 + Tu;
			 Tv = FNMS(KP250000000, Tu, T1);
			 TI = FMA(KP618033988, TH, TC);
			 TK = FNMS(KP618033988, TC, TH);
			 {
			      E TQ, TO, Tx, TJ, TP;
			      TQ = TM - TN;
			      TO = TM + TN;
			      Tx = FMA(KP559016994, Tw, Tv);
			      TJ = FNMS(KP559016994, Tw, Tv);
			      ci[WS(rs, 4)] = TO + TL;
			      TP = FNMS(KP250000000, TO, TL);
			      ci[WS(rs, 1)] = FMA(KP951056516, TK, TJ);
			      cr[WS(rs, 2)] = FNMS(KP951056516, TK, TJ);
			      cr[WS(rs, 1)] = FMA(KP951056516, TI, Tx);
			      ci[0] = FNMS(KP951056516, TI, Tx);
			      TV = FMA(KP559016994, TQ, TP);
			      TR = FNMS(KP559016994, TQ, TP);
			 }
		    }
		    ci[WS(rs, 2)] = FMA(KP951056516, TU, TR);
		    cr[WS(rs, 3)] = FMS(KP951056516, TU, TR);
		    ci[WS(rs, 3)] = FMA(KP951056516, TW, TV);
		    cr[WS(rs, 4)] = FMS(KP951056516, TW, TV);
	       }
	  }
     }
}

static const tw_instr twinstr[] = {
     {TW_CEXP, 1, 1},
     {TW_CEXP, 1, 3},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 5, "hf2_5", twinstr, &GENUS, {14, 10, 30, 0} };

void X(codelet_hf2_5) (planner *p) {
     X(khc2hc_register) (p, hf2_5, &desc);
}
#else				/* HAVE_FMA */

/* Generated by: ../../../genfft/gen_hc2hc.native -compact -variables 4 -pipeline-latency 4 -twiddle-log3 -precompute-twiddles -n 5 -dit -name hf2_5 -include hf.h */

/*
 * This function contains 44 FP additions, 32 FP multiplications,
 * (or, 30 additions, 18 multiplications, 14 fused multiply/add),
 * 37 stack variables, 4 constants, and 20 memory accesses
 */
#include "hf.h"

static void hf2_5(R *cr, R *ci, const R *W, stride rs, INT mb, INT me, INT ms)
{
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     {
	  INT m;
	  for (m = mb, W = W + ((mb - 1) * 4); m < me; m = m + 1, cr = cr + ms, ci = ci - ms, W = W + 4, MAKE_VOLATILE_STRIDE(rs)) {
	       E T2, T4, T7, T9, Tb, Tl, Tf, Tj;
	       {
		    E T8, Te, Ta, Td;
		    T2 = W[0];
		    T4 = W[1];
		    T7 = W[2];
		    T9 = W[3];
		    T8 = T2 * T7;
		    Te = T4 * T7;
		    Ta = T4 * T9;
		    Td = T2 * T9;
		    Tb = T8 - Ta;
		    Tl = Td - Te;
		    Tf = Td + Te;
		    Tj = T8 + Ta;
	       }
	       {
		    E T1, TI, Ty, TB, TG, TF, TJ, TK, TL, Ti, Tr, Ts;
		    T1 = cr[0];
		    TI = ci[0];
		    {
			 E T6, Tw, Tq, TA, Th, Tx, Tn, Tz;
			 {
			      E T3, T5, To, Tp;
			      T3 = cr[WS(rs, 1)];
			      T5 = ci[WS(rs, 1)];
			      T6 = FMA(T2, T3, T4 * T5);
			      Tw = FNMS(T4, T3, T2 * T5);
			      To = cr[WS(rs, 3)];
			      Tp = ci[WS(rs, 3)];
			      Tq = FMA(T7, To, T9 * Tp);
			      TA = FNMS(T9, To, T7 * Tp);
			 }
			 {
			      E Tc, Tg, Tk, Tm;
			      Tc = cr[WS(rs, 4)];
			      Tg = ci[WS(rs, 4)];
			      Th = FMA(Tb, Tc, Tf * Tg);
			      Tx = FNMS(Tf, Tc, Tb * Tg);
			      Tk = cr[WS(rs, 2)];
			      Tm = ci[WS(rs, 2)];
			      Tn = FMA(Tj, Tk, Tl * Tm);
			      Tz = FNMS(Tl, Tk, Tj * Tm);
			 }
			 Ty = Tw - Tx;
			 TB = Tz - TA;
			 TG = Tn - Tq;
			 TF = Th - T6;
			 TJ = Tw + Tx;
			 TK = Tz + TA;
			 TL = TJ + TK;
			 Ti = T6 + Th;
			 Tr = Tn + Tq;
			 Ts = Ti + Tr;
		    }
		    cr[0] = T1 + Ts;
		    {
			 E TC, TE, Tv, TD, Tt, Tu;
			 TC = FMA(KP951056516, Ty, KP587785252 * TB);
			 TE = FNMS(KP587785252, Ty, KP951056516 * TB);
			 Tt = KP559016994 * (Ti - Tr);
			 Tu = FNMS(KP250000000, Ts, T1);
			 Tv = Tt + Tu;
			 TD = Tu - Tt;
			 ci[0] = Tv - TC;
			 ci[WS(rs, 1)] = TD + TE;
			 cr[WS(rs, 1)] = Tv + TC;
			 cr[WS(rs, 2)] = TD - TE;
		    }
		    ci[WS(rs, 4)] = TL + TI;
		    {
			 E TH, TP, TO, TQ, TM, TN;
			 TH = FMA(KP587785252, TF, KP951056516 * TG);
			 TP = FNMS(KP587785252, TG, KP951056516 * TF);
			 TM = FNMS(KP250000000, TL, TI);
			 TN = KP559016994 * (TJ - TK);
			 TO = TM - TN;
			 TQ = TN + TM;
			 cr[WS(rs, 3)] = TH - TO;
			 ci[WS(rs, 3)] = TP + TQ;
			 ci[WS(rs, 2)] = TH + TO;
			 cr[WS(rs, 4)] = TP - TQ;
		    }
	       }
	  }
     }
}

static const tw_instr twinstr[] = {
     {TW_CEXP, 1, 1},
     {TW_CEXP, 1, 3},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 5, "hf2_5", twinstr, &GENUS, {30, 18, 14, 0} };

void X(codelet_hf2_5) (planner *p) {
     X(khc2hc_register) (p, hf2_5, &desc);
}
#endif				/* HAVE_FMA */
