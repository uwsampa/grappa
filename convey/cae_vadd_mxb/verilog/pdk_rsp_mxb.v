/*****************************************************************************/
//
// Module          : pdk_rsp_mxb.vpp
// Revision        : $Revision: 1.2 $
// Last Modified On: $Date: 2010/10/22 19:39:51 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : mbarr
// Created On      : Fri Apr 30 17:01:24 CDT 2010
//
//-----------------------------------------------------------------------------
//
// Description     : PDK MC response xbar.
//
// This is really more mux than xbar, there is a single arbiter for one
// reponse destination.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: pdk_rsp_mxb.vpp,v 1.2 2010/10/22 19:39:51 gedwards Exp $ */

`timescale 1 ns / 1 ps

module pdk_rsp_mxb (/*AUTOARG*/
   // Outputs
   mc0_rsp_stall, mc1_rsp_stall, mc2_rsp_stall, mc3_rsp_stall,
   mc4_rsp_stall, mc5_rsp_stall, mc6_rsp_stall, mc7_rsp_stall,
   p0_rsp_push, p0_rsp_rdctl, p0_rsp_data, p1_rsp_push, p1_rsp_rdctl,
   p1_rsp_data, p2_rsp_push, p2_rsp_rdctl, p2_rsp_data, p3_rsp_push,
   p3_rsp_rdctl, p3_rsp_data, p4_rsp_push, p4_rsp_rdctl, p4_rsp_data,
   p5_rsp_push, p5_rsp_rdctl, p5_rsp_data, p6_rsp_push, p6_rsp_rdctl,
   p6_rsp_data, p7_rsp_push, p7_rsp_rdctl, p7_rsp_data,
   r_ovrflow_alarm, r_undflow_alarm,
   // Inputs
   clk, clk_csr, i_reset, mc0_rsp_push, mc0_rsp_rdctl, mc0_rsp_data,
   mc1_rsp_push, mc1_rsp_rdctl, mc1_rsp_data, mc2_rsp_push,
   mc2_rsp_rdctl, mc2_rsp_data, mc3_rsp_push, mc3_rsp_rdctl,
   mc3_rsp_data, mc4_rsp_push, mc4_rsp_rdctl, mc4_rsp_data,
   mc5_rsp_push, mc5_rsp_rdctl, mc5_rsp_data, mc6_rsp_push,
   mc6_rsp_rdctl, mc6_rsp_data, mc7_rsp_push, mc7_rsp_rdctl,
   mc7_rsp_data
   ) ;

   // synthesis attribute keep_hierarchy pdk_mc_mxb "true";

   input                clk;
   input                clk_csr;
   input                i_reset;


   // MC0 response in.
   input                mc0_rsp_push;
   input  [31:0]        mc0_rsp_rdctl;
   input  [63:0]        mc0_rsp_data;

   output               mc0_rsp_stall;

   // MC1 response in.
   input                mc1_rsp_push;
   input  [31:0]        mc1_rsp_rdctl;
   input  [63:0]        mc1_rsp_data;

   output               mc1_rsp_stall;

   // MC2 response in.
   input                mc2_rsp_push;
   input  [31:0]        mc2_rsp_rdctl;
   input  [63:0]        mc2_rsp_data;

   output               mc2_rsp_stall;

   // MC3 response in.
   input                mc3_rsp_push;
   input  [31:0]        mc3_rsp_rdctl;
   input  [63:0]        mc3_rsp_data;

   output               mc3_rsp_stall;

   // MC4 response in.
   input                mc4_rsp_push;
   input  [31:0]        mc4_rsp_rdctl;
   input  [63:0]        mc4_rsp_data;

   output               mc4_rsp_stall;

   // MC5 response in.
   input                mc5_rsp_push;
   input  [31:0]        mc5_rsp_rdctl;
   input  [63:0]        mc5_rsp_data;

   output               mc5_rsp_stall;

   // MC6 response in.
   input                mc6_rsp_push;
   input  [31:0]        mc6_rsp_rdctl;
   input  [63:0]        mc6_rsp_data;

   output               mc6_rsp_stall;

   // MC7 response in.
   input                mc7_rsp_push;
   input  [31:0]        mc7_rsp_rdctl;
   input  [63:0]        mc7_rsp_data;

   output               mc7_rsp_stall;

   // P0 response out.
   output               p0_rsp_push;

   output [31:0]        p0_rsp_rdctl;
   output [63:0]        p0_rsp_data;

   // P1 response out.
   output               p1_rsp_push;

   output [31:0]        p1_rsp_rdctl;
   output [63:0]        p1_rsp_data;

   // P2 response out.
   output               p2_rsp_push;

   output [31:0]        p2_rsp_rdctl;
   output [63:0]        p2_rsp_data;

   // P3 response out.
   output               p3_rsp_push;

   output [31:0]        p3_rsp_rdctl;
   output [63:0]        p3_rsp_data;

   // P4 response out.
   output               p4_rsp_push;

   output [31:0]        p4_rsp_rdctl;
   output [63:0]        p4_rsp_data;

   // P5 response out.
   output               p5_rsp_push;

   output [31:0]        p5_rsp_rdctl;
   output [63:0]        p5_rsp_data;

   // P6 response out.
   output               p6_rsp_push;

   output [31:0]        p6_rsp_rdctl;
   output [63:0]        p6_rsp_data;

   // P7 response out.
   output               p7_rsp_push;

   output [31:0]        p7_rsp_rdctl;
   output [63:0]        p7_rsp_data;

   output [7:0]         r_ovrflow_alarm;
   output [7:0]         r_undflow_alarm;

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   // MC0 responses to P0-7 arb.
   wire                 r_mc0_p0_rsp_;
   wire                 r_mc0_p1_rsp_;
   wire                 r_mc0_p2_rsp_;
   wire                 r_mc0_p3_rsp_;
   wire                 r_mc0_p4_rsp_;
   wire                 r_mc0_p5_rsp_;
   wire                 r_mc0_p6_rsp_;
   wire                 r_mc0_p7_rsp_;

   // MC0 grants from P0-7 arb.
   wire                 r_p0_mc0_grant;
   wire                 r_p1_mc0_grant;
   wire                 r_p2_mc0_grant;
   wire                 r_p3_mc0_grant;
   wire                 r_p4_mc0_grant;
   wire                 r_p5_mc0_grant;
   wire                 r_p6_mc0_grant;
   wire                 r_p7_mc0_grant;

   // MC0 req stall, data to P0-7 arb muxes.
   wire                 r_mc0_rsp_stall;
   wire                 r_mc0_rsp_last;
   wire [31:0]          r_mc0_rsp_rdctl;
   wire [63:0]          r_mc0_rsp_data;

   // MC1 responses to P0-7 arb.
   wire                 r_mc1_p0_rsp_;
   wire                 r_mc1_p1_rsp_;
   wire                 r_mc1_p2_rsp_;
   wire                 r_mc1_p3_rsp_;
   wire                 r_mc1_p4_rsp_;
   wire                 r_mc1_p5_rsp_;
   wire                 r_mc1_p6_rsp_;
   wire                 r_mc1_p7_rsp_;

   // MC1 grants from P0-7 arb.
   wire                 r_p0_mc1_grant;
   wire                 r_p1_mc1_grant;
   wire                 r_p2_mc1_grant;
   wire                 r_p3_mc1_grant;
   wire                 r_p4_mc1_grant;
   wire                 r_p5_mc1_grant;
   wire                 r_p6_mc1_grant;
   wire                 r_p7_mc1_grant;

   // MC1 req stall, data to P0-7 arb muxes.
   wire                 r_mc1_rsp_stall;
   wire                 r_mc1_rsp_last;
   wire [31:0]          r_mc1_rsp_rdctl;
   wire [63:0]          r_mc1_rsp_data;

   // MC2 responses to P0-7 arb.
   wire                 r_mc2_p0_rsp_;
   wire                 r_mc2_p1_rsp_;
   wire                 r_mc2_p2_rsp_;
   wire                 r_mc2_p3_rsp_;
   wire                 r_mc2_p4_rsp_;
   wire                 r_mc2_p5_rsp_;
   wire                 r_mc2_p6_rsp_;
   wire                 r_mc2_p7_rsp_;

   // MC2 grants from P0-7 arb.
   wire                 r_p0_mc2_grant;
   wire                 r_p1_mc2_grant;
   wire                 r_p2_mc2_grant;
   wire                 r_p3_mc2_grant;
   wire                 r_p4_mc2_grant;
   wire                 r_p5_mc2_grant;
   wire                 r_p6_mc2_grant;
   wire                 r_p7_mc2_grant;

   // MC2 req stall, data to P0-7 arb muxes.
   wire                 r_mc2_rsp_stall;
   wire                 r_mc2_rsp_last;
   wire [31:0]          r_mc2_rsp_rdctl;
   wire [63:0]          r_mc2_rsp_data;

   // MC3 responses to P0-7 arb.
   wire                 r_mc3_p0_rsp_;
   wire                 r_mc3_p1_rsp_;
   wire                 r_mc3_p2_rsp_;
   wire                 r_mc3_p3_rsp_;
   wire                 r_mc3_p4_rsp_;
   wire                 r_mc3_p5_rsp_;
   wire                 r_mc3_p6_rsp_;
   wire                 r_mc3_p7_rsp_;

   // MC3 grants from P0-7 arb.
   wire                 r_p0_mc3_grant;
   wire                 r_p1_mc3_grant;
   wire                 r_p2_mc3_grant;
   wire                 r_p3_mc3_grant;
   wire                 r_p4_mc3_grant;
   wire                 r_p5_mc3_grant;
   wire                 r_p6_mc3_grant;
   wire                 r_p7_mc3_grant;

   // MC3 req stall, data to P0-7 arb muxes.
   wire                 r_mc3_rsp_stall;
   wire                 r_mc3_rsp_last;
   wire [31:0]          r_mc3_rsp_rdctl;
   wire [63:0]          r_mc3_rsp_data;

   // MC4 responses to P0-7 arb.
   wire                 r_mc4_p0_rsp_;
   wire                 r_mc4_p1_rsp_;
   wire                 r_mc4_p2_rsp_;
   wire                 r_mc4_p3_rsp_;
   wire                 r_mc4_p4_rsp_;
   wire                 r_mc4_p5_rsp_;
   wire                 r_mc4_p6_rsp_;
   wire                 r_mc4_p7_rsp_;

   // MC4 grants from P0-7 arb.
   wire                 r_p0_mc4_grant;
   wire                 r_p1_mc4_grant;
   wire                 r_p2_mc4_grant;
   wire                 r_p3_mc4_grant;
   wire                 r_p4_mc4_grant;
   wire                 r_p5_mc4_grant;
   wire                 r_p6_mc4_grant;
   wire                 r_p7_mc4_grant;

   // MC4 req stall, data to P0-7 arb muxes.
   wire                 r_mc4_rsp_stall;
   wire                 r_mc4_rsp_last;
   wire [31:0]          r_mc4_rsp_rdctl;
   wire [63:0]          r_mc4_rsp_data;

   // MC5 responses to P0-7 arb.
   wire                 r_mc5_p0_rsp_;
   wire                 r_mc5_p1_rsp_;
   wire                 r_mc5_p2_rsp_;
   wire                 r_mc5_p3_rsp_;
   wire                 r_mc5_p4_rsp_;
   wire                 r_mc5_p5_rsp_;
   wire                 r_mc5_p6_rsp_;
   wire                 r_mc5_p7_rsp_;

   // MC5 grants from P0-7 arb.
   wire                 r_p0_mc5_grant;
   wire                 r_p1_mc5_grant;
   wire                 r_p2_mc5_grant;
   wire                 r_p3_mc5_grant;
   wire                 r_p4_mc5_grant;
   wire                 r_p5_mc5_grant;
   wire                 r_p6_mc5_grant;
   wire                 r_p7_mc5_grant;

   // MC5 req stall, data to P0-7 arb muxes.
   wire                 r_mc5_rsp_stall;
   wire                 r_mc5_rsp_last;
   wire [31:0]          r_mc5_rsp_rdctl;
   wire [63:0]          r_mc5_rsp_data;

   // MC6 responses to P0-7 arb.
   wire                 r_mc6_p0_rsp_;
   wire                 r_mc6_p1_rsp_;
   wire                 r_mc6_p2_rsp_;
   wire                 r_mc6_p3_rsp_;
   wire                 r_mc6_p4_rsp_;
   wire                 r_mc6_p5_rsp_;
   wire                 r_mc6_p6_rsp_;
   wire                 r_mc6_p7_rsp_;

   // MC6 grants from P0-7 arb.
   wire                 r_p0_mc6_grant;
   wire                 r_p1_mc6_grant;
   wire                 r_p2_mc6_grant;
   wire                 r_p3_mc6_grant;
   wire                 r_p4_mc6_grant;
   wire                 r_p5_mc6_grant;
   wire                 r_p6_mc6_grant;
   wire                 r_p7_mc6_grant;

   // MC6 req stall, data to P0-7 arb muxes.
   wire                 r_mc6_rsp_stall;
   wire                 r_mc6_rsp_last;
   wire [31:0]          r_mc6_rsp_rdctl;
   wire [63:0]          r_mc6_rsp_data;

   // MC7 responses to P0-7 arb.
   wire                 r_mc7_p0_rsp_;
   wire                 r_mc7_p1_rsp_;
   wire                 r_mc7_p2_rsp_;
   wire                 r_mc7_p3_rsp_;
   wire                 r_mc7_p4_rsp_;
   wire                 r_mc7_p5_rsp_;
   wire                 r_mc7_p6_rsp_;
   wire                 r_mc7_p7_rsp_;

   // MC7 grants from P0-7 arb.
   wire                 r_p0_mc7_grant;
   wire                 r_p1_mc7_grant;
   wire                 r_p2_mc7_grant;
   wire                 r_p3_mc7_grant;
   wire                 r_p4_mc7_grant;
   wire                 r_p5_mc7_grant;
   wire                 r_p6_mc7_grant;
   wire                 r_p7_mc7_grant;

   // MC7 req stall, data to P0-7 arb muxes.
   wire                 r_mc7_rsp_stall;
   wire                 r_mc7_rsp_last;
   wire [31:0]          r_mc7_rsp_rdctl;
   wire [63:0]          r_mc7_rsp_data;

   wire [2:0]           r_p0_gsel;
   wire                 r_p0_push;

   wire [31:0]          c_p0_rsp_rdctl;
   wire [63:0]          c_p0_rsp_data;

   wire [31:0]          r_p0_rsp_rdctl;
   wire [63:0]          r_p0_rsp_data;
   wire [2:0]           r_p1_gsel;
   wire                 r_p1_push;

   wire [31:0]          c_p1_rsp_rdctl;
   wire [63:0]          c_p1_rsp_data;

   wire [31:0]          r_p1_rsp_rdctl;
   wire [63:0]          r_p1_rsp_data;
   wire [2:0]           r_p2_gsel;
   wire                 r_p2_push;

   wire [31:0]          c_p2_rsp_rdctl;
   wire [63:0]          c_p2_rsp_data;

   wire [31:0]          r_p2_rsp_rdctl;
   wire [63:0]          r_p2_rsp_data;
   wire [2:0]           r_p3_gsel;
   wire                 r_p3_push;

   wire [31:0]          c_p3_rsp_rdctl;
   wire [63:0]          c_p3_rsp_data;

   wire [31:0]          r_p3_rsp_rdctl;
   wire [63:0]          r_p3_rsp_data;
   wire [2:0]           r_p4_gsel;
   wire                 r_p4_push;

   wire [31:0]          c_p4_rsp_rdctl;
   wire [63:0]          c_p4_rsp_data;

   wire [31:0]          r_p4_rsp_rdctl;
   wire [63:0]          r_p4_rsp_data;
   wire [2:0]           r_p5_gsel;
   wire                 r_p5_push;

   wire [31:0]          c_p5_rsp_rdctl;
   wire [63:0]          c_p5_rsp_data;

   wire [31:0]          r_p5_rsp_rdctl;
   wire [63:0]          r_p5_rsp_data;
   wire [2:0]           r_p6_gsel;
   wire                 r_p6_push;

   wire [31:0]          c_p6_rsp_rdctl;
   wire [63:0]          c_p6_rsp_data;

   wire [31:0]          r_p6_rsp_rdctl;
   wire [63:0]          r_p6_rsp_data;
   wire [2:0]           r_p7_gsel;
   wire                 r_p7_push;

   wire [31:0]          c_p7_rsp_rdctl;
   wire [63:0]          c_p7_rsp_data;

   wire [31:0]          r_p7_rsp_rdctl;
   wire [63:0]          r_p7_rsp_data;


   /* ----------      combinatorial blocks    ---------- */

   assign mc0_rsp_stall = r_mc0_rsp_stall;
   assign mc1_rsp_stall = r_mc1_rsp_stall;
   assign mc2_rsp_stall = r_mc2_rsp_stall;
   assign mc3_rsp_stall = r_mc3_rsp_stall;
   assign mc4_rsp_stall = r_mc4_rsp_stall;
   assign mc5_rsp_stall = r_mc5_rsp_stall;
   assign mc6_rsp_stall = r_mc6_rsp_stall;
   assign mc7_rsp_stall = r_mc7_rsp_stall;

   assign p0_rsp_push = r_p0_push;
   assign p0_rsp_rdctl = r_p0_rsp_rdctl;
   assign p0_rsp_data = r_p0_rsp_data;

   assign p1_rsp_push = r_p1_push;
   assign p1_rsp_rdctl = r_p1_rsp_rdctl;
   assign p1_rsp_data = r_p1_rsp_data;

   assign p2_rsp_push = r_p2_push;
   assign p2_rsp_rdctl = r_p2_rsp_rdctl;
   assign p2_rsp_data = r_p2_rsp_data;

   assign p3_rsp_push = r_p3_push;
   assign p3_rsp_rdctl = r_p3_rsp_rdctl;
   assign p3_rsp_data = r_p3_rsp_data;

   assign p4_rsp_push = r_p4_push;
   assign p4_rsp_rdctl = r_p4_rsp_rdctl;
   assign p4_rsp_data = r_p4_rsp_data;

   assign p5_rsp_push = r_p5_push;
   assign p5_rsp_rdctl = r_p5_rsp_rdctl;
   assign p5_rsp_data = r_p5_rsp_data;

   assign p6_rsp_push = r_p6_push;
   assign p6_rsp_rdctl = r_p6_rsp_rdctl;
   assign p6_rsp_data = r_p6_rsp_data;

   assign p7_rsp_push = r_p7_push;
   assign p7_rsp_rdctl = r_p7_rsp_rdctl;
   assign p7_rsp_data = r_p7_rsp_data;

   /* ----------      external module calls   ---------- */

   // MC0 response fifo
   rspfifo fifo0 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc0_rsp_push),
     .mc_rsp_rdctl         (mc0_rsp_rdctl),
     .mc_rsp_data          (mc0_rsp_data),
     .mc_rsp_stall         (r_mc0_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc0_grant),
     .p1_grant          (r_p1_mc0_grant),
     .p2_grant          (r_p2_mc0_grant),
     .p3_grant          (r_p3_mc0_grant),
     .p4_grant          (r_p4_mc0_grant),
     .p5_grant          (r_p5_mc0_grant),
     .p6_grant          (r_p6_mc0_grant),
     .p7_grant          (r_p7_mc0_grant),
     .p0_rsp_           (r_mc0_p0_rsp_),
     .p1_rsp_           (r_mc0_p1_rsp_),
     .p2_rsp_           (r_mc0_p2_rsp_),
     .p3_rsp_           (r_mc0_p3_rsp_),
     .p4_rsp_           (r_mc0_p4_rsp_),
     .p5_rsp_           (r_mc0_p5_rsp_),
     .p6_rsp_           (r_mc0_p6_rsp_),
     .p7_rsp_           (r_mc0_p7_rsp_),
     .rsp_last             (r_mc0_rsp_last),
     .rsp_rdctl            (r_mc0_rsp_rdctl),
     .rsp_data             (r_mc0_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[0]),
     .r_undflow_alarm     (r_undflow_alarm[0])
   );

   // MC1 response fifo
   rspfifo fifo1 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc1_rsp_push),
     .mc_rsp_rdctl         (mc1_rsp_rdctl),
     .mc_rsp_data          (mc1_rsp_data),
     .mc_rsp_stall         (r_mc1_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc1_grant),
     .p1_grant          (r_p1_mc1_grant),
     .p2_grant          (r_p2_mc1_grant),
     .p3_grant          (r_p3_mc1_grant),
     .p4_grant          (r_p4_mc1_grant),
     .p5_grant          (r_p5_mc1_grant),
     .p6_grant          (r_p6_mc1_grant),
     .p7_grant          (r_p7_mc1_grant),
     .p0_rsp_           (r_mc1_p0_rsp_),
     .p1_rsp_           (r_mc1_p1_rsp_),
     .p2_rsp_           (r_mc1_p2_rsp_),
     .p3_rsp_           (r_mc1_p3_rsp_),
     .p4_rsp_           (r_mc1_p4_rsp_),
     .p5_rsp_           (r_mc1_p5_rsp_),
     .p6_rsp_           (r_mc1_p6_rsp_),
     .p7_rsp_           (r_mc1_p7_rsp_),
     .rsp_last             (r_mc1_rsp_last),
     .rsp_rdctl            (r_mc1_rsp_rdctl),
     .rsp_data             (r_mc1_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[1]),
     .r_undflow_alarm     (r_undflow_alarm[1])
   );

   // MC2 response fifo
   rspfifo fifo2 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc2_rsp_push),
     .mc_rsp_rdctl         (mc2_rsp_rdctl),
     .mc_rsp_data          (mc2_rsp_data),
     .mc_rsp_stall         (r_mc2_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc2_grant),
     .p1_grant          (r_p1_mc2_grant),
     .p2_grant          (r_p2_mc2_grant),
     .p3_grant          (r_p3_mc2_grant),
     .p4_grant          (r_p4_mc2_grant),
     .p5_grant          (r_p5_mc2_grant),
     .p6_grant          (r_p6_mc2_grant),
     .p7_grant          (r_p7_mc2_grant),
     .p0_rsp_           (r_mc2_p0_rsp_),
     .p1_rsp_           (r_mc2_p1_rsp_),
     .p2_rsp_           (r_mc2_p2_rsp_),
     .p3_rsp_           (r_mc2_p3_rsp_),
     .p4_rsp_           (r_mc2_p4_rsp_),
     .p5_rsp_           (r_mc2_p5_rsp_),
     .p6_rsp_           (r_mc2_p6_rsp_),
     .p7_rsp_           (r_mc2_p7_rsp_),
     .rsp_last             (r_mc2_rsp_last),
     .rsp_rdctl            (r_mc2_rsp_rdctl),
     .rsp_data             (r_mc2_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[2]),
     .r_undflow_alarm     (r_undflow_alarm[2])
   );

   // MC3 response fifo
   rspfifo fifo3 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc3_rsp_push),
     .mc_rsp_rdctl         (mc3_rsp_rdctl),
     .mc_rsp_data          (mc3_rsp_data),
     .mc_rsp_stall         (r_mc3_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc3_grant),
     .p1_grant          (r_p1_mc3_grant),
     .p2_grant          (r_p2_mc3_grant),
     .p3_grant          (r_p3_mc3_grant),
     .p4_grant          (r_p4_mc3_grant),
     .p5_grant          (r_p5_mc3_grant),
     .p6_grant          (r_p6_mc3_grant),
     .p7_grant          (r_p7_mc3_grant),
     .p0_rsp_           (r_mc3_p0_rsp_),
     .p1_rsp_           (r_mc3_p1_rsp_),
     .p2_rsp_           (r_mc3_p2_rsp_),
     .p3_rsp_           (r_mc3_p3_rsp_),
     .p4_rsp_           (r_mc3_p4_rsp_),
     .p5_rsp_           (r_mc3_p5_rsp_),
     .p6_rsp_           (r_mc3_p6_rsp_),
     .p7_rsp_           (r_mc3_p7_rsp_),
     .rsp_last             (r_mc3_rsp_last),
     .rsp_rdctl            (r_mc3_rsp_rdctl),
     .rsp_data             (r_mc3_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[3]),
     .r_undflow_alarm     (r_undflow_alarm[3])
   );

   // MC4 response fifo
   rspfifo fifo4 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc4_rsp_push),
     .mc_rsp_rdctl         (mc4_rsp_rdctl),
     .mc_rsp_data          (mc4_rsp_data),
     .mc_rsp_stall         (r_mc4_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc4_grant),
     .p1_grant          (r_p1_mc4_grant),
     .p2_grant          (r_p2_mc4_grant),
     .p3_grant          (r_p3_mc4_grant),
     .p4_grant          (r_p4_mc4_grant),
     .p5_grant          (r_p5_mc4_grant),
     .p6_grant          (r_p6_mc4_grant),
     .p7_grant          (r_p7_mc4_grant),
     .p0_rsp_           (r_mc4_p0_rsp_),
     .p1_rsp_           (r_mc4_p1_rsp_),
     .p2_rsp_           (r_mc4_p2_rsp_),
     .p3_rsp_           (r_mc4_p3_rsp_),
     .p4_rsp_           (r_mc4_p4_rsp_),
     .p5_rsp_           (r_mc4_p5_rsp_),
     .p6_rsp_           (r_mc4_p6_rsp_),
     .p7_rsp_           (r_mc4_p7_rsp_),
     .rsp_last             (r_mc4_rsp_last),
     .rsp_rdctl            (r_mc4_rsp_rdctl),
     .rsp_data             (r_mc4_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[4]),
     .r_undflow_alarm     (r_undflow_alarm[4])
   );

   // MC5 response fifo
   rspfifo fifo5 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc5_rsp_push),
     .mc_rsp_rdctl         (mc5_rsp_rdctl),
     .mc_rsp_data          (mc5_rsp_data),
     .mc_rsp_stall         (r_mc5_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc5_grant),
     .p1_grant          (r_p1_mc5_grant),
     .p2_grant          (r_p2_mc5_grant),
     .p3_grant          (r_p3_mc5_grant),
     .p4_grant          (r_p4_mc5_grant),
     .p5_grant          (r_p5_mc5_grant),
     .p6_grant          (r_p6_mc5_grant),
     .p7_grant          (r_p7_mc5_grant),
     .p0_rsp_           (r_mc5_p0_rsp_),
     .p1_rsp_           (r_mc5_p1_rsp_),
     .p2_rsp_           (r_mc5_p2_rsp_),
     .p3_rsp_           (r_mc5_p3_rsp_),
     .p4_rsp_           (r_mc5_p4_rsp_),
     .p5_rsp_           (r_mc5_p5_rsp_),
     .p6_rsp_           (r_mc5_p6_rsp_),
     .p7_rsp_           (r_mc5_p7_rsp_),
     .rsp_last             (r_mc5_rsp_last),
     .rsp_rdctl            (r_mc5_rsp_rdctl),
     .rsp_data             (r_mc5_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[5]),
     .r_undflow_alarm     (r_undflow_alarm[5])
   );

   // MC6 response fifo
   rspfifo fifo6 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc6_rsp_push),
     .mc_rsp_rdctl         (mc6_rsp_rdctl),
     .mc_rsp_data          (mc6_rsp_data),
     .mc_rsp_stall         (r_mc6_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc6_grant),
     .p1_grant          (r_p1_mc6_grant),
     .p2_grant          (r_p2_mc6_grant),
     .p3_grant          (r_p3_mc6_grant),
     .p4_grant          (r_p4_mc6_grant),
     .p5_grant          (r_p5_mc6_grant),
     .p6_grant          (r_p6_mc6_grant),
     .p7_grant          (r_p7_mc6_grant),
     .p0_rsp_           (r_mc6_p0_rsp_),
     .p1_rsp_           (r_mc6_p1_rsp_),
     .p2_rsp_           (r_mc6_p2_rsp_),
     .p3_rsp_           (r_mc6_p3_rsp_),
     .p4_rsp_           (r_mc6_p4_rsp_),
     .p5_rsp_           (r_mc6_p5_rsp_),
     .p6_rsp_           (r_mc6_p6_rsp_),
     .p7_rsp_           (r_mc6_p7_rsp_),
     .rsp_last             (r_mc6_rsp_last),
     .rsp_rdctl            (r_mc6_rsp_rdctl),
     .rsp_data             (r_mc6_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[6]),
     .r_undflow_alarm     (r_undflow_alarm[6])
   );

   // MC7 response fifo
   rspfifo fifo7 (
     .clk167_              (clk),
     .clk_csr              (clk_csr),
     .reset167_            (reset),
     .mc_rsp_push          (mc7_rsp_push),
     .mc_rsp_rdctl         (mc7_rsp_rdctl),
     .mc_rsp_data          (mc7_rsp_data),
     .mc_rsp_stall         (r_mc7_rsp_stall),
     .clk167               (clk),
     .reset167             (reset),
     .p0_grant          (r_p0_mc7_grant),
     .p1_grant          (r_p1_mc7_grant),
     .p2_grant          (r_p2_mc7_grant),
     .p3_grant          (r_p3_mc7_grant),
     .p4_grant          (r_p4_mc7_grant),
     .p5_grant          (r_p5_mc7_grant),
     .p6_grant          (r_p6_mc7_grant),
     .p7_grant          (r_p7_mc7_grant),
     .p0_rsp_           (r_mc7_p0_rsp_),
     .p1_rsp_           (r_mc7_p1_rsp_),
     .p2_rsp_           (r_mc7_p2_rsp_),
     .p3_rsp_           (r_mc7_p3_rsp_),
     .p4_rsp_           (r_mc7_p4_rsp_),
     .p5_rsp_           (r_mc7_p5_rsp_),
     .p6_rsp_           (r_mc7_p6_rsp_),
     .p7_rsp_           (r_mc7_p7_rsp_),
     .rsp_last             (r_mc7_rsp_last),
     .rsp_rdctl            (r_mc7_rsp_rdctl),
     .rsp_data             (r_mc7_rsp_data),
     .r_ovrflow_alarm      (r_ovrflow_alarm[7]),
     .r_undflow_alarm     (r_undflow_alarm[7])
   );

   // P0 response arbiter.
   rsparb8 arb0 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p0_rsp_),
     .i_empty1            (r_mc1_p0_rsp_),
     .i_empty2            (r_mc2_p0_rsp_),
     .i_empty3            (r_mc3_p0_rsp_),
     .i_empty4            (r_mc4_p0_rsp_),
     .i_empty5            (r_mc5_p0_rsp_),
     .i_empty6            (r_mc6_p0_rsp_),
     .i_empty7            (r_mc7_p0_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p0_mc0_grant),
     .o_grant1            (r_p0_mc1_grant),
     .o_grant2            (r_p0_mc2_grant),
     .o_grant3            (r_p0_mc3_grant),
     .o_grant4            (r_p0_mc4_grant),
     .o_grant5            (r_p0_mc5_grant),
     .o_grant6            (r_p0_mc6_grant),
     .o_grant7            (r_p0_mc7_grant),
     .o_gsel               (r_p0_gsel),
     .o_push               (r_p0_push)
   );

   mux8_96 mux0 ( .s(r_p0_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p0_rsp_rdctl, c_p0_rsp_data}) );

   // P1 response arbiter.
   rsparb8 arb1 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p1_rsp_),
     .i_empty1            (r_mc1_p1_rsp_),
     .i_empty2            (r_mc2_p1_rsp_),
     .i_empty3            (r_mc3_p1_rsp_),
     .i_empty4            (r_mc4_p1_rsp_),
     .i_empty5            (r_mc5_p1_rsp_),
     .i_empty6            (r_mc6_p1_rsp_),
     .i_empty7            (r_mc7_p1_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p1_mc0_grant),
     .o_grant1            (r_p1_mc1_grant),
     .o_grant2            (r_p1_mc2_grant),
     .o_grant3            (r_p1_mc3_grant),
     .o_grant4            (r_p1_mc4_grant),
     .o_grant5            (r_p1_mc5_grant),
     .o_grant6            (r_p1_mc6_grant),
     .o_grant7            (r_p1_mc7_grant),
     .o_gsel               (r_p1_gsel),
     .o_push               (r_p1_push)
   );

   mux8_96 mux1 ( .s(r_p1_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p1_rsp_rdctl, c_p1_rsp_data}) );

   // P2 response arbiter.
   rsparb8 arb2 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p2_rsp_),
     .i_empty1            (r_mc1_p2_rsp_),
     .i_empty2            (r_mc2_p2_rsp_),
     .i_empty3            (r_mc3_p2_rsp_),
     .i_empty4            (r_mc4_p2_rsp_),
     .i_empty5            (r_mc5_p2_rsp_),
     .i_empty6            (r_mc6_p2_rsp_),
     .i_empty7            (r_mc7_p2_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p2_mc0_grant),
     .o_grant1            (r_p2_mc1_grant),
     .o_grant2            (r_p2_mc2_grant),
     .o_grant3            (r_p2_mc3_grant),
     .o_grant4            (r_p2_mc4_grant),
     .o_grant5            (r_p2_mc5_grant),
     .o_grant6            (r_p2_mc6_grant),
     .o_grant7            (r_p2_mc7_grant),
     .o_gsel               (r_p2_gsel),
     .o_push               (r_p2_push)
   );

   mux8_96 mux2 ( .s(r_p2_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p2_rsp_rdctl, c_p2_rsp_data}) );

   // P3 response arbiter.
   rsparb8 arb3 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p3_rsp_),
     .i_empty1            (r_mc1_p3_rsp_),
     .i_empty2            (r_mc2_p3_rsp_),
     .i_empty3            (r_mc3_p3_rsp_),
     .i_empty4            (r_mc4_p3_rsp_),
     .i_empty5            (r_mc5_p3_rsp_),
     .i_empty6            (r_mc6_p3_rsp_),
     .i_empty7            (r_mc7_p3_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p3_mc0_grant),
     .o_grant1            (r_p3_mc1_grant),
     .o_grant2            (r_p3_mc2_grant),
     .o_grant3            (r_p3_mc3_grant),
     .o_grant4            (r_p3_mc4_grant),
     .o_grant5            (r_p3_mc5_grant),
     .o_grant6            (r_p3_mc6_grant),
     .o_grant7            (r_p3_mc7_grant),
     .o_gsel               (r_p3_gsel),
     .o_push               (r_p3_push)
   );

   mux8_96 mux3 ( .s(r_p3_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p3_rsp_rdctl, c_p3_rsp_data}) );

   // P4 response arbiter.
   rsparb8 arb4 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p4_rsp_),
     .i_empty1            (r_mc1_p4_rsp_),
     .i_empty2            (r_mc2_p4_rsp_),
     .i_empty3            (r_mc3_p4_rsp_),
     .i_empty4            (r_mc4_p4_rsp_),
     .i_empty5            (r_mc5_p4_rsp_),
     .i_empty6            (r_mc6_p4_rsp_),
     .i_empty7            (r_mc7_p4_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p4_mc0_grant),
     .o_grant1            (r_p4_mc1_grant),
     .o_grant2            (r_p4_mc2_grant),
     .o_grant3            (r_p4_mc3_grant),
     .o_grant4            (r_p4_mc4_grant),
     .o_grant5            (r_p4_mc5_grant),
     .o_grant6            (r_p4_mc6_grant),
     .o_grant7            (r_p4_mc7_grant),
     .o_gsel               (r_p4_gsel),
     .o_push               (r_p4_push)
   );

   mux8_96 mux4 ( .s(r_p4_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p4_rsp_rdctl, c_p4_rsp_data}) );

   // P5 response arbiter.
   rsparb8 arb5 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p5_rsp_),
     .i_empty1            (r_mc1_p5_rsp_),
     .i_empty2            (r_mc2_p5_rsp_),
     .i_empty3            (r_mc3_p5_rsp_),
     .i_empty4            (r_mc4_p5_rsp_),
     .i_empty5            (r_mc5_p5_rsp_),
     .i_empty6            (r_mc6_p5_rsp_),
     .i_empty7            (r_mc7_p5_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p5_mc0_grant),
     .o_grant1            (r_p5_mc1_grant),
     .o_grant2            (r_p5_mc2_grant),
     .o_grant3            (r_p5_mc3_grant),
     .o_grant4            (r_p5_mc4_grant),
     .o_grant5            (r_p5_mc5_grant),
     .o_grant6            (r_p5_mc6_grant),
     .o_grant7            (r_p5_mc7_grant),
     .o_gsel               (r_p5_gsel),
     .o_push               (r_p5_push)
   );

   mux8_96 mux5 ( .s(r_p5_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p5_rsp_rdctl, c_p5_rsp_data}) );

   // P6 response arbiter.
   rsparb8 arb6 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p6_rsp_),
     .i_empty1            (r_mc1_p6_rsp_),
     .i_empty2            (r_mc2_p6_rsp_),
     .i_empty3            (r_mc3_p6_rsp_),
     .i_empty4            (r_mc4_p6_rsp_),
     .i_empty5            (r_mc5_p6_rsp_),
     .i_empty6            (r_mc6_p6_rsp_),
     .i_empty7            (r_mc7_p6_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p6_mc0_grant),
     .o_grant1            (r_p6_mc1_grant),
     .o_grant2            (r_p6_mc2_grant),
     .o_grant3            (r_p6_mc3_grant),
     .o_grant4            (r_p6_mc4_grant),
     .o_grant5            (r_p6_mc5_grant),
     .o_grant6            (r_p6_mc6_grant),
     .o_grant7            (r_p6_mc7_grant),
     .o_gsel               (r_p6_gsel),
     .o_push               (r_p6_push)
   );

   mux8_96 mux6 ( .s(r_p6_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p6_rsp_rdctl, c_p6_rsp_data}) );

   // P7 response arbiter.
   rsparb8 arb7 (
     .clk                  (clk),
     .reset                (reset),
     .i_lastreq0          (r_mc0_rsp_last),
     .i_lastreq1          (r_mc1_rsp_last),
     .i_lastreq2          (r_mc2_rsp_last),
     .i_lastreq3          (r_mc3_rsp_last),
     .i_lastreq4          (r_mc4_rsp_last),
     .i_lastreq5          (r_mc5_rsp_last),
     .i_lastreq6          (r_mc6_rsp_last),
     .i_lastreq7          (r_mc7_rsp_last),
     .i_empty0            (r_mc0_p7_rsp_),
     .i_empty1            (r_mc1_p7_rsp_),
     .i_empty2            (r_mc2_p7_rsp_),
     .i_empty3            (r_mc3_p7_rsp_),
     .i_empty4            (r_mc4_p7_rsp_),
     .i_empty5            (r_mc5_p7_rsp_),
     .i_empty6            (r_mc6_p7_rsp_),
     .i_empty7            (r_mc7_p7_rsp_),
     .i_stall              (1'b0),
     .o_grant0            (r_p7_mc0_grant),
     .o_grant1            (r_p7_mc1_grant),
     .o_grant2            (r_p7_mc2_grant),
     .o_grant3            (r_p7_mc3_grant),
     .o_grant4            (r_p7_mc4_grant),
     .o_grant5            (r_p7_mc5_grant),
     .o_grant6            (r_p7_mc6_grant),
     .o_grant7            (r_p7_mc7_grant),
     .o_gsel               (r_p7_gsel),
     .o_push               (r_p7_push)
   );

   mux8_96 mux7 ( .s(r_p7_gsel), .i0({r_mc0_rsp_rdctl, r_mc0_rsp_data}), .i1({r_mc1_rsp_rdctl, r_mc1_rsp_data}), .i2({r_mc2_rsp_rdctl, r_mc2_rsp_data}), .i3({r_mc3_rsp_rdctl, r_mc3_rsp_data}), .i4({r_mc4_rsp_rdctl, r_mc4_rsp_data}), .i5({r_mc5_rsp_rdctl, r_mc5_rsp_data}), .i6({r_mc6_rsp_rdctl, r_mc6_rsp_data}), .i7({r_mc7_rsp_rdctl, r_mc7_rsp_data}), .o({c_p7_rsp_rdctl, c_p7_rsp_data}) );


   /* ----------            registers         ---------- */

   dffs_keep_1 rsk0 ( .clk(clk), .set(i_reset), .d(1'b0), .q(reset) );

   dff_32 reg_rdctl0 ( .clk(clk), .d(c_p0_rsp_rdctl), .q(r_p0_rsp_rdctl) );
   dff_64 reg_data0 ( .clk(clk), .d(c_p0_rsp_data), .q(r_p0_rsp_data) );
   dff_32 reg_rdctl1 ( .clk(clk), .d(c_p1_rsp_rdctl), .q(r_p1_rsp_rdctl) );
   dff_64 reg_data1 ( .clk(clk), .d(c_p1_rsp_data), .q(r_p1_rsp_data) );
   dff_32 reg_rdctl2 ( .clk(clk), .d(c_p2_rsp_rdctl), .q(r_p2_rsp_rdctl) );
   dff_64 reg_data2 ( .clk(clk), .d(c_p2_rsp_data), .q(r_p2_rsp_data) );
   dff_32 reg_rdctl3 ( .clk(clk), .d(c_p3_rsp_rdctl), .q(r_p3_rsp_rdctl) );
   dff_64 reg_data3 ( .clk(clk), .d(c_p3_rsp_data), .q(r_p3_rsp_data) );
   dff_32 reg_rdctl4 ( .clk(clk), .d(c_p4_rsp_rdctl), .q(r_p4_rsp_rdctl) );
   dff_64 reg_data4 ( .clk(clk), .d(c_p4_rsp_data), .q(r_p4_rsp_data) );
   dff_32 reg_rdctl5 ( .clk(clk), .d(c_p5_rsp_rdctl), .q(r_p5_rsp_rdctl) );
   dff_64 reg_data5 ( .clk(clk), .d(c_p5_rsp_data), .q(r_p5_rsp_data) );
   dff_32 reg_rdctl6 ( .clk(clk), .d(c_p6_rsp_rdctl), .q(r_p6_rsp_rdctl) );
   dff_64 reg_data6 ( .clk(clk), .d(c_p6_rsp_data), .q(r_p6_rsp_data) );
   dff_32 reg_rdctl7 ( .clk(clk), .d(c_p7_rsp_rdctl), .q(r_p7_rsp_rdctl) );
   dff_64 reg_data7 ( .clk(clk), .d(c_p7_rsp_data), .q(r_p7_rsp_data) );


   /* ---------- debug & synopsys off blocks  ---------- */

`ifdef DEBUG
   //always @(posedge clk) begin
   //  if (r_t4_valid)
   //    $display("%m i=%d j=%d c=%d ci=%d cqd=%d", r_t4_ix,r_t4_jx,r_t4_c,r_t4_ci,r_t4_cqd);
   //end
`endif
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

   // TODO Assertion checks:

   // synopsys translate_on

endmodule // pdk_rsp_mxb
