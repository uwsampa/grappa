/*****************************************************************************/
//
// Module          : mux8to1x2.vpp
// Revision        : $Revision: 1.1.1.1 $
// Last Modified On: $Date: 2010/09/14 20:23:30 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : Dean Walker
// Created On      : Thu Nov 29 16:25:48 2007
//
//-----------------------------------------------------------------------------
//
// Description     : optimized 8 to 1 muxes for Xilinx
//
//			Up thru at least ISE 9.2 sp4, Xilinx will not pack 
//			2 8:1 muxes in the same slice. 
//			Furthermore the unused LUTs in a slice with an 8:1 mux 
//			will not be used. This module works around that by 
//			forcing 2 8:1 muxes into the same slice using the 
//			RLOC constraint.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: mux8to1x2.v,v 1.1.1.1 2010/09/14 20:23:30 gedwards Exp $ */

`timescale 1 ns / 1 ps

module mux8to1x2 (/*AUTOARG*/
   // Outputs
   out,
   // Inputs
   in0, sel0, in1, sel1
   ) ;
   
   // synthesis attribute keep_hierarchy mux8to1x2 "true";

   /* ----------        port declarations     ---------- */

   input  [7:0]         in0;
   input  [2:0]         sel0;
   input  [7:0]         in1;
   input  [2:0]         sel1;

   output [1:0]         out;

   /* ----------         include files        ---------- */
   /* ----------          wires & regs        ---------- */

   wire   [3:0]         m0i0;
   wire   [3:0]         m1i0;
   wire                 m0o0;
   wire                 m1o0;
   wire   [3:0]         m0i1;
   wire   [3:0]         m1i1;
   wire                 m0o1;
   wire                 m1o1;

   /* ----------      combinatorial blocks    ---------- */
   /* ----------      external module calls   ---------- */

   assign m0i0 = in0[3:0];
   assign m1i0 = in0[7:4];
   assign m0o0 = m0i0[sel0[1:0]];
   assign m1o0 = m1i0[sel0[1:0]];

   // synthesis attribute RLOC m0 "X0Y0";
   MUXF7 m0 (.S(sel0[2]), .I0(m0o0), .I1(m1o0), .O(out[0]));

   assign m0i1 = in1[3:0];
   assign m1i1 = in1[7:4];
   assign m0o1 = m0i1[sel1[1:0]];
   assign m1o1 = m1i1[sel1[1:0]];

   // synthesis attribute RLOC m1 "X0Y0";
   MUXF7 m1 (.S(sel1[2]), .I0(m0o1), .I1(m1o1), .O(out[1]));

   /* ----------            registers         ---------- */
   /* ---------- debug & synopsys off blocks  ---------- */
   
endmodule // mux8to1x2

