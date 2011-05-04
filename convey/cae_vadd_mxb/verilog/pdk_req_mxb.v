/*****************************************************************************/
//
// Module          : pdk_req_mxb.vpp
// Revision        : $Revision: 1.4 $
// Last Modified On: $Date: 2010/11/02 19:45:15 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : mbarr
// Created On      : Fri Apr 30 17:01:24 CDT 2010
//
//-----------------------------------------------------------------------------
//
// Description     : PDK MC request xbar.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: pdk_req_mxb.vpp,v 1.4 2010/11/02 19:45:15 gedwards Exp $ */

`timescale 1 ns / 1 ps

module pdk_req_mxb (/*AUTOARG*/
   // Outputs
   p0_mc_req_idle, p0_mc_req_stall, p1_mc_req_idle, p1_mc_req_stall,
   p2_mc_req_idle, p2_mc_req_stall, p3_mc_req_idle, p3_mc_req_stall,
   p4_mc_req_idle, p4_mc_req_stall, p5_mc_req_idle, p5_mc_req_stall,
   p6_mc_req_idle, p6_mc_req_stall, p7_mc_req_idle, p7_mc_req_stall,
   mc0_req_ld, mc0_req_st, mc0_req_vadr, mc0_req_size,
   mc0_req_wrd_rdctl, mc1_req_ld, mc1_req_st, mc1_req_vadr,
   mc1_req_size, mc1_req_wrd_rdctl, mc2_req_ld, mc2_req_st,
   mc2_req_vadr, mc2_req_size, mc2_req_wrd_rdctl, mc3_req_ld,
   mc3_req_st, mc3_req_vadr, mc3_req_size, mc3_req_wrd_rdctl,
   mc4_req_ld, mc4_req_st, mc4_req_vadr, mc4_req_size,
   mc4_req_wrd_rdctl, mc5_req_ld, mc5_req_st, mc5_req_vadr,
   mc5_req_size, mc5_req_wrd_rdctl, mc6_req_ld, mc6_req_st,
   mc6_req_vadr, mc6_req_size, mc6_req_wrd_rdctl, mc7_req_ld,
   mc7_req_st, mc7_req_vadr, mc7_req_size, mc7_req_wrd_rdctl,
   r_ovrflow_alarm, r_undflow_alarm,
   // Inputs
   clk, clk_csr, i_reset, p0_mc_req_ld, p0_mc_req_st, p0_mc_req_last,
   p0_mc_req_vadr, p0_mc_req_size, p0_mc_req_wrd_rdctl, p1_mc_req_ld,
   p1_mc_req_st, p1_mc_req_last, p1_mc_req_vadr, p1_mc_req_size,
   p1_mc_req_wrd_rdctl, p2_mc_req_ld, p2_mc_req_st, p2_mc_req_last,
   p2_mc_req_vadr, p2_mc_req_size, p2_mc_req_wrd_rdctl, p3_mc_req_ld,
   p3_mc_req_st, p3_mc_req_last, p3_mc_req_vadr, p3_mc_req_size,
   p3_mc_req_wrd_rdctl, p4_mc_req_ld, p4_mc_req_st, p4_mc_req_last,
   p4_mc_req_vadr, p4_mc_req_size, p4_mc_req_wrd_rdctl, p5_mc_req_ld,
   p5_mc_req_st, p5_mc_req_last, p5_mc_req_vadr, p5_mc_req_size,
   p5_mc_req_wrd_rdctl, p6_mc_req_ld, p6_mc_req_st, p6_mc_req_last,
   p6_mc_req_vadr, p6_mc_req_size, p6_mc_req_wrd_rdctl, p7_mc_req_ld,
   p7_mc_req_st, p7_mc_req_last, p7_mc_req_vadr, p7_mc_req_size,
   p7_mc_req_wrd_rdctl, mc0_rd_rq_stall, mc0_wr_rq_stall,
   mc1_rd_rq_stall, mc1_wr_rq_stall, mc2_rd_rq_stall, mc2_wr_rq_stall,
   mc3_rd_rq_stall, mc3_wr_rq_stall, mc4_rd_rq_stall, mc4_wr_rq_stall,
   mc5_rd_rq_stall, mc5_wr_rq_stall, mc6_rd_rq_stall, mc6_wr_rq_stall,
   mc7_rd_rq_stall, mc7_wr_rq_stall
   ) ;

   // synthesis attribute keep_hierarchy pdk_req_xb "true";

   input                clk;
   input                clk_csr;
   input                i_reset;


   // p0 request in.
   input                p0_mc_req_ld;
   input                p0_mc_req_st;
   input                p0_mc_req_last;
   input  [47:0]        p0_mc_req_vadr;
   input  [1:0]         p0_mc_req_size;
   input  [63:0]        p0_mc_req_wrd_rdctl;

   output               p0_mc_req_idle;
   output               p0_mc_req_stall;

   // p1 request in.
   input                p1_mc_req_ld;
   input                p1_mc_req_st;
   input                p1_mc_req_last;
   input  [47:0]        p1_mc_req_vadr;
   input  [1:0]         p1_mc_req_size;
   input  [63:0]        p1_mc_req_wrd_rdctl;

   output               p1_mc_req_idle;
   output               p1_mc_req_stall;

   // p2 request in.
   input                p2_mc_req_ld;
   input                p2_mc_req_st;
   input                p2_mc_req_last;
   input  [47:0]        p2_mc_req_vadr;
   input  [1:0]         p2_mc_req_size;
   input  [63:0]        p2_mc_req_wrd_rdctl;

   output               p2_mc_req_idle;
   output               p2_mc_req_stall;

   // p3 request in.
   input                p3_mc_req_ld;
   input                p3_mc_req_st;
   input                p3_mc_req_last;
   input  [47:0]        p3_mc_req_vadr;
   input  [1:0]         p3_mc_req_size;
   input  [63:0]        p3_mc_req_wrd_rdctl;

   output               p3_mc_req_idle;
   output               p3_mc_req_stall;

   // p4 request in.
   input                p4_mc_req_ld;
   input                p4_mc_req_st;
   input                p4_mc_req_last;
   input  [47:0]        p4_mc_req_vadr;
   input  [1:0]         p4_mc_req_size;
   input  [63:0]        p4_mc_req_wrd_rdctl;

   output               p4_mc_req_idle;
   output               p4_mc_req_stall;

   // p5 request in.
   input                p5_mc_req_ld;
   input                p5_mc_req_st;
   input                p5_mc_req_last;
   input  [47:0]        p5_mc_req_vadr;
   input  [1:0]         p5_mc_req_size;
   input  [63:0]        p5_mc_req_wrd_rdctl;

   output               p5_mc_req_idle;
   output               p5_mc_req_stall;

   // p6 request in.
   input                p6_mc_req_ld;
   input                p6_mc_req_st;
   input                p6_mc_req_last;
   input  [47:0]        p6_mc_req_vadr;
   input  [1:0]         p6_mc_req_size;
   input  [63:0]        p6_mc_req_wrd_rdctl;

   output               p6_mc_req_idle;
   output               p6_mc_req_stall;

   // p7 request in.
   input                p7_mc_req_ld;
   input                p7_mc_req_st;
   input                p7_mc_req_last;
   input  [47:0]        p7_mc_req_vadr;
   input  [1:0]         p7_mc_req_size;
   input  [63:0]        p7_mc_req_wrd_rdctl;

   output               p7_mc_req_idle;
   output               p7_mc_req_stall;

   // MC0 request out.
   input                mc0_rd_rq_stall;
   input                mc0_wr_rq_stall;

   output               mc0_req_ld;
   output               mc0_req_st;
   output [47:0]        mc0_req_vadr;
   output [1:0]         mc0_req_size;
   output [63:0]        mc0_req_wrd_rdctl;

   // MC1 request out.
   input                mc1_rd_rq_stall;
   input                mc1_wr_rq_stall;

   output               mc1_req_ld;
   output               mc1_req_st;
   output [47:0]        mc1_req_vadr;
   output [1:0]         mc1_req_size;
   output [63:0]        mc1_req_wrd_rdctl;

   // MC2 request out.
   input                mc2_rd_rq_stall;
   input                mc2_wr_rq_stall;

   output               mc2_req_ld;
   output               mc2_req_st;
   output [47:0]        mc2_req_vadr;
   output [1:0]         mc2_req_size;
   output [63:0]        mc2_req_wrd_rdctl;

   // MC3 request out.
   input                mc3_rd_rq_stall;
   input                mc3_wr_rq_stall;

   output               mc3_req_ld;
   output               mc3_req_st;
   output [47:0]        mc3_req_vadr;
   output [1:0]         mc3_req_size;
   output [63:0]        mc3_req_wrd_rdctl;

   // MC4 request out.
   input                mc4_rd_rq_stall;
   input                mc4_wr_rq_stall;

   output               mc4_req_ld;
   output               mc4_req_st;
   output [47:0]        mc4_req_vadr;
   output [1:0]         mc4_req_size;
   output [63:0]        mc4_req_wrd_rdctl;

   // MC5 request out.
   input                mc5_rd_rq_stall;
   input                mc5_wr_rq_stall;

   output               mc5_req_ld;
   output               mc5_req_st;
   output [47:0]        mc5_req_vadr;
   output [1:0]         mc5_req_size;
   output [63:0]        mc5_req_wrd_rdctl;

   // MC6 request out.
   input                mc6_rd_rq_stall;
   input                mc6_wr_rq_stall;

   output               mc6_req_ld;
   output               mc6_req_st;
   output [47:0]        mc6_req_vadr;
   output [1:0]         mc6_req_size;
   output [63:0]        mc6_req_wrd_rdctl;

   // MC7 request out.
   input                mc7_rd_rq_stall;
   input                mc7_wr_rq_stall;

   output               mc7_req_ld;
   output               mc7_req_st;
   output [47:0]        mc7_req_vadr;
   output [1:0]         mc7_req_size;
   output [63:0]        mc7_req_wrd_rdctl;

   output [7:0]         r_ovrflow_alarm;
   output [7:0]         r_undflow_alarm;

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   // P0 requests to MC0-7 arb.
   wire                 r_p0_mc0_req_;
   wire                 r_p0_mc1_req_;
   wire                 r_p0_mc2_req_;
   wire                 r_p0_mc3_req_;
   wire                 r_p0_mc4_req_;
   wire                 r_p0_mc5_req_;
   wire                 r_p0_mc6_req_;
   wire                 r_p0_mc7_req_;

   // P0 grants from MC0-7 arb.
   wire                 r_mc0_p0_grant;
   wire                 r_mc1_p0_grant;
   wire                 r_mc2_p0_grant;
   wire                 r_mc3_p0_grant;
   wire                 r_mc4_p0_grant;
   wire                 r_mc5_p0_grant;
   wire                 r_mc6_p0_grant;
   wire                 r_mc7_p0_grant;

   // P0 req stall, data to MC0-7 arb muxes.
   wire                 r_p0_req_idle;
   wire                 r_p0_req_stall;
   wire                 r_p0_mc_req_ld_st;
   wire                 r_p0_mc_req_last;
   wire [47:0]          r_p0_mc_req_vadr;
   wire [1:0]           r_p0_mc_req_size;
   wire [63:0]          r_p0_mc_req_wrd_rdctl;

   // P1 requests to MC0-7 arb.
   wire                 r_p1_mc0_req_;
   wire                 r_p1_mc1_req_;
   wire                 r_p1_mc2_req_;
   wire                 r_p1_mc3_req_;
   wire                 r_p1_mc4_req_;
   wire                 r_p1_mc5_req_;
   wire                 r_p1_mc6_req_;
   wire                 r_p1_mc7_req_;

   // P1 grants from MC0-7 arb.
   wire                 r_mc0_p1_grant;
   wire                 r_mc1_p1_grant;
   wire                 r_mc2_p1_grant;
   wire                 r_mc3_p1_grant;
   wire                 r_mc4_p1_grant;
   wire                 r_mc5_p1_grant;
   wire                 r_mc6_p1_grant;
   wire                 r_mc7_p1_grant;

   // P1 req stall, data to MC0-7 arb muxes.
   wire                 r_p1_req_idle;
   wire                 r_p1_req_stall;
   wire                 r_p1_mc_req_ld_st;
   wire                 r_p1_mc_req_last;
   wire [47:0]          r_p1_mc_req_vadr;
   wire [1:0]           r_p1_mc_req_size;
   wire [63:0]          r_p1_mc_req_wrd_rdctl;

   // P2 requests to MC0-7 arb.
   wire                 r_p2_mc0_req_;
   wire                 r_p2_mc1_req_;
   wire                 r_p2_mc2_req_;
   wire                 r_p2_mc3_req_;
   wire                 r_p2_mc4_req_;
   wire                 r_p2_mc5_req_;
   wire                 r_p2_mc6_req_;
   wire                 r_p2_mc7_req_;

   // P2 grants from MC0-7 arb.
   wire                 r_mc0_p2_grant;
   wire                 r_mc1_p2_grant;
   wire                 r_mc2_p2_grant;
   wire                 r_mc3_p2_grant;
   wire                 r_mc4_p2_grant;
   wire                 r_mc5_p2_grant;
   wire                 r_mc6_p2_grant;
   wire                 r_mc7_p2_grant;

   // P2 req stall, data to MC0-7 arb muxes.
   wire                 r_p2_req_idle;
   wire                 r_p2_req_stall;
   wire                 r_p2_mc_req_ld_st;
   wire                 r_p2_mc_req_last;
   wire [47:0]          r_p2_mc_req_vadr;
   wire [1:0]           r_p2_mc_req_size;
   wire [63:0]          r_p2_mc_req_wrd_rdctl;

   // P3 requests to MC0-7 arb.
   wire                 r_p3_mc0_req_;
   wire                 r_p3_mc1_req_;
   wire                 r_p3_mc2_req_;
   wire                 r_p3_mc3_req_;
   wire                 r_p3_mc4_req_;
   wire                 r_p3_mc5_req_;
   wire                 r_p3_mc6_req_;
   wire                 r_p3_mc7_req_;

   // P3 grants from MC0-7 arb.
   wire                 r_mc0_p3_grant;
   wire                 r_mc1_p3_grant;
   wire                 r_mc2_p3_grant;
   wire                 r_mc3_p3_grant;
   wire                 r_mc4_p3_grant;
   wire                 r_mc5_p3_grant;
   wire                 r_mc6_p3_grant;
   wire                 r_mc7_p3_grant;

   // P3 req stall, data to MC0-7 arb muxes.
   wire                 r_p3_req_idle;
   wire                 r_p3_req_stall;
   wire                 r_p3_mc_req_ld_st;
   wire                 r_p3_mc_req_last;
   wire [47:0]          r_p3_mc_req_vadr;
   wire [1:0]           r_p3_mc_req_size;
   wire [63:0]          r_p3_mc_req_wrd_rdctl;

   // P4 requests to MC0-7 arb.
   wire                 r_p4_mc0_req_;
   wire                 r_p4_mc1_req_;
   wire                 r_p4_mc2_req_;
   wire                 r_p4_mc3_req_;
   wire                 r_p4_mc4_req_;
   wire                 r_p4_mc5_req_;
   wire                 r_p4_mc6_req_;
   wire                 r_p4_mc7_req_;

   // P4 grants from MC0-7 arb.
   wire                 r_mc0_p4_grant;
   wire                 r_mc1_p4_grant;
   wire                 r_mc2_p4_grant;
   wire                 r_mc3_p4_grant;
   wire                 r_mc4_p4_grant;
   wire                 r_mc5_p4_grant;
   wire                 r_mc6_p4_grant;
   wire                 r_mc7_p4_grant;

   // P4 req stall, data to MC0-7 arb muxes.
   wire                 r_p4_req_idle;
   wire                 r_p4_req_stall;
   wire                 r_p4_mc_req_ld_st;
   wire                 r_p4_mc_req_last;
   wire [47:0]          r_p4_mc_req_vadr;
   wire [1:0]           r_p4_mc_req_size;
   wire [63:0]          r_p4_mc_req_wrd_rdctl;

   // P5 requests to MC0-7 arb.
   wire                 r_p5_mc0_req_;
   wire                 r_p5_mc1_req_;
   wire                 r_p5_mc2_req_;
   wire                 r_p5_mc3_req_;
   wire                 r_p5_mc4_req_;
   wire                 r_p5_mc5_req_;
   wire                 r_p5_mc6_req_;
   wire                 r_p5_mc7_req_;

   // P5 grants from MC0-7 arb.
   wire                 r_mc0_p5_grant;
   wire                 r_mc1_p5_grant;
   wire                 r_mc2_p5_grant;
   wire                 r_mc3_p5_grant;
   wire                 r_mc4_p5_grant;
   wire                 r_mc5_p5_grant;
   wire                 r_mc6_p5_grant;
   wire                 r_mc7_p5_grant;

   // P5 req stall, data to MC0-7 arb muxes.
   wire                 r_p5_req_idle;
   wire                 r_p5_req_stall;
   wire                 r_p5_mc_req_ld_st;
   wire                 r_p5_mc_req_last;
   wire [47:0]          r_p5_mc_req_vadr;
   wire [1:0]           r_p5_mc_req_size;
   wire [63:0]          r_p5_mc_req_wrd_rdctl;

   // P6 requests to MC0-7 arb.
   wire                 r_p6_mc0_req_;
   wire                 r_p6_mc1_req_;
   wire                 r_p6_mc2_req_;
   wire                 r_p6_mc3_req_;
   wire                 r_p6_mc4_req_;
   wire                 r_p6_mc5_req_;
   wire                 r_p6_mc6_req_;
   wire                 r_p6_mc7_req_;

   // P6 grants from MC0-7 arb.
   wire                 r_mc0_p6_grant;
   wire                 r_mc1_p6_grant;
   wire                 r_mc2_p6_grant;
   wire                 r_mc3_p6_grant;
   wire                 r_mc4_p6_grant;
   wire                 r_mc5_p6_grant;
   wire                 r_mc6_p6_grant;
   wire                 r_mc7_p6_grant;

   // P6 req stall, data to MC0-7 arb muxes.
   wire                 r_p6_req_idle;
   wire                 r_p6_req_stall;
   wire                 r_p6_mc_req_ld_st;
   wire                 r_p6_mc_req_last;
   wire [47:0]          r_p6_mc_req_vadr;
   wire [1:0]           r_p6_mc_req_size;
   wire [63:0]          r_p6_mc_req_wrd_rdctl;

   // P7 requests to MC0-7 arb.
   wire                 r_p7_mc0_req_;
   wire                 r_p7_mc1_req_;
   wire                 r_p7_mc2_req_;
   wire                 r_p7_mc3_req_;
   wire                 r_p7_mc4_req_;
   wire                 r_p7_mc5_req_;
   wire                 r_p7_mc6_req_;
   wire                 r_p7_mc7_req_;

   // P7 grants from MC0-7 arb.
   wire                 r_mc0_p7_grant;
   wire                 r_mc1_p7_grant;
   wire                 r_mc2_p7_grant;
   wire                 r_mc3_p7_grant;
   wire                 r_mc4_p7_grant;
   wire                 r_mc5_p7_grant;
   wire                 r_mc6_p7_grant;
   wire                 r_mc7_p7_grant;

   // P7 req stall, data to MC0-7 arb muxes.
   wire                 r_p7_req_idle;
   wire                 r_p7_req_stall;
   wire                 r_p7_mc_req_ld_st;
   wire                 r_p7_mc_req_last;
   wire [47:0]          r_p7_mc_req_vadr;
   wire [1:0]           r_p7_mc_req_size;
   wire [63:0]          r_p7_mc_req_wrd_rdctl;

   wire [2:0]           r_mc0_gsel;
   wire                 r_mc0_push_ld;
   wire                 r_mc0_push_st;

   wire                 c_mc0_req_ld_st;
   wire [47:0]          c_mc0_req_vadr;
   wire [1:0]           c_mc0_req_size;
   wire [63:0]          c_mc0_req_wrd_rdctl;

   wire [47:0]          r_mc0_req_vadr;
   wire [1:0]           r_mc0_req_size;
   wire [63:0]          r_mc0_req_wrd_rdctl;
   wire [2:0]           r_mc1_gsel;
   wire                 r_mc1_push_ld;
   wire                 r_mc1_push_st;

   wire                 c_mc1_req_ld_st;
   wire [47:0]          c_mc1_req_vadr;
   wire [1:0]           c_mc1_req_size;
   wire [63:0]          c_mc1_req_wrd_rdctl;

   wire [47:0]          r_mc1_req_vadr;
   wire [1:0]           r_mc1_req_size;
   wire [63:0]          r_mc1_req_wrd_rdctl;
   wire [2:0]           r_mc2_gsel;
   wire                 r_mc2_push_ld;
   wire                 r_mc2_push_st;

   wire                 c_mc2_req_ld_st;
   wire [47:0]          c_mc2_req_vadr;
   wire [1:0]           c_mc2_req_size;
   wire [63:0]          c_mc2_req_wrd_rdctl;

   wire [47:0]          r_mc2_req_vadr;
   wire [1:0]           r_mc2_req_size;
   wire [63:0]          r_mc2_req_wrd_rdctl;
   wire [2:0]           r_mc3_gsel;
   wire                 r_mc3_push_ld;
   wire                 r_mc3_push_st;

   wire                 c_mc3_req_ld_st;
   wire [47:0]          c_mc3_req_vadr;
   wire [1:0]           c_mc3_req_size;
   wire [63:0]          c_mc3_req_wrd_rdctl;

   wire [47:0]          r_mc3_req_vadr;
   wire [1:0]           r_mc3_req_size;
   wire [63:0]          r_mc3_req_wrd_rdctl;
   wire [2:0]           r_mc4_gsel;
   wire                 r_mc4_push_ld;
   wire                 r_mc4_push_st;

   wire                 c_mc4_req_ld_st;
   wire [47:0]          c_mc4_req_vadr;
   wire [1:0]           c_mc4_req_size;
   wire [63:0]          c_mc4_req_wrd_rdctl;

   wire [47:0]          r_mc4_req_vadr;
   wire [1:0]           r_mc4_req_size;
   wire [63:0]          r_mc4_req_wrd_rdctl;
   wire [2:0]           r_mc5_gsel;
   wire                 r_mc5_push_ld;
   wire                 r_mc5_push_st;

   wire                 c_mc5_req_ld_st;
   wire [47:0]          c_mc5_req_vadr;
   wire [1:0]           c_mc5_req_size;
   wire [63:0]          c_mc5_req_wrd_rdctl;

   wire [47:0]          r_mc5_req_vadr;
   wire [1:0]           r_mc5_req_size;
   wire [63:0]          r_mc5_req_wrd_rdctl;
   wire [2:0]           r_mc6_gsel;
   wire                 r_mc6_push_ld;
   wire                 r_mc6_push_st;

   wire                 c_mc6_req_ld_st;
   wire [47:0]          c_mc6_req_vadr;
   wire [1:0]           c_mc6_req_size;
   wire [63:0]          c_mc6_req_wrd_rdctl;

   wire [47:0]          r_mc6_req_vadr;
   wire [1:0]           r_mc6_req_size;
   wire [63:0]          r_mc6_req_wrd_rdctl;
   wire [2:0]           r_mc7_gsel;
   wire                 r_mc7_push_ld;
   wire                 r_mc7_push_st;

   wire                 c_mc7_req_ld_st;
   wire [47:0]          c_mc7_req_vadr;
   wire [1:0]           c_mc7_req_size;
   wire [63:0]          c_mc7_req_wrd_rdctl;

   wire [47:0]          r_mc7_req_vadr;
   wire [1:0]           r_mc7_req_size;
   wire [63:0]          r_mc7_req_wrd_rdctl;

   wire                 p0_mc_req_push;
   wire                 p0_mc_req_ld_st;
   wire                 p1_mc_req_push;
   wire                 p1_mc_req_ld_st;
   wire                 p2_mc_req_push;
   wire                 p2_mc_req_ld_st;
   wire                 p3_mc_req_push;
   wire                 p3_mc_req_ld_st;
   wire                 p4_mc_req_push;
   wire                 p4_mc_req_ld_st;
   wire                 p5_mc_req_push;
   wire                 p5_mc_req_ld_st;
   wire                 p6_mc_req_push;
   wire                 p6_mc_req_ld_st;
   wire                 p7_mc_req_push;
   wire                 p7_mc_req_ld_st;

   wire [63:0]          p0_req_wrd_rdctl_xb;
   wire [63:0]          p1_req_wrd_rdctl_xb;
   wire [63:0]          p2_req_wrd_rdctl_xb;
   wire [63:0]          p3_req_wrd_rdctl_xb;
   wire [63:0]          p4_req_wrd_rdctl_xb;
   wire [63:0]          p5_req_wrd_rdctl_xb;
   wire [63:0]          p6_req_wrd_rdctl_xb;
   wire [63:0]          p7_req_wrd_rdctl_xb;
    
   /* ----------      combinatorial blocks    ---------- */

   assign p0_mc_req_idle = r_p0_req_idle;
   assign p0_mc_req_stall = r_p0_req_stall;
   assign p1_mc_req_idle = r_p1_req_idle;
   assign p1_mc_req_stall = r_p1_req_stall;
   assign p2_mc_req_idle = r_p2_req_idle;
   assign p2_mc_req_stall = r_p2_req_stall;
   assign p3_mc_req_idle = r_p3_req_idle;
   assign p3_mc_req_stall = r_p3_req_stall;
   assign p4_mc_req_idle = r_p4_req_idle;
   assign p4_mc_req_stall = r_p4_req_stall;
   assign p5_mc_req_idle = r_p5_req_idle;
   assign p5_mc_req_stall = r_p5_req_stall;
   assign p6_mc_req_idle = r_p6_req_idle;
   assign p6_mc_req_stall = r_p6_req_stall;
   assign p7_mc_req_idle = r_p7_req_idle;
   assign p7_mc_req_stall = r_p7_req_stall;

   assign mc0_req_ld = r_mc0_push_ld;
   assign mc0_req_st = r_mc0_push_st;
   assign mc0_req_vadr = r_mc0_req_vadr;
   assign mc0_req_size = r_mc0_req_size;
   assign mc0_req_wrd_rdctl = r_mc0_req_wrd_rdctl;

   assign mc1_req_ld = r_mc1_push_ld;
   assign mc1_req_st = r_mc1_push_st;
   assign mc1_req_vadr = r_mc1_req_vadr;
   assign mc1_req_size = r_mc1_req_size;
   assign mc1_req_wrd_rdctl = r_mc1_req_wrd_rdctl;

   assign mc2_req_ld = r_mc2_push_ld;
   assign mc2_req_st = r_mc2_push_st;
   assign mc2_req_vadr = r_mc2_req_vadr;
   assign mc2_req_size = r_mc2_req_size;
   assign mc2_req_wrd_rdctl = r_mc2_req_wrd_rdctl;

   assign mc3_req_ld = r_mc3_push_ld;
   assign mc3_req_st = r_mc3_push_st;
   assign mc3_req_vadr = r_mc3_req_vadr;
   assign mc3_req_size = r_mc3_req_size;
   assign mc3_req_wrd_rdctl = r_mc3_req_wrd_rdctl;

   assign mc4_req_ld = r_mc4_push_ld;
   assign mc4_req_st = r_mc4_push_st;
   assign mc4_req_vadr = r_mc4_req_vadr;
   assign mc4_req_size = r_mc4_req_size;
   assign mc4_req_wrd_rdctl = r_mc4_req_wrd_rdctl;

   assign mc5_req_ld = r_mc5_push_ld;
   assign mc5_req_st = r_mc5_push_st;
   assign mc5_req_vadr = r_mc5_req_vadr;
   assign mc5_req_size = r_mc5_req_size;
   assign mc5_req_wrd_rdctl = r_mc5_req_wrd_rdctl;

   assign mc6_req_ld = r_mc6_push_ld;
   assign mc6_req_st = r_mc6_push_st;
   assign mc6_req_vadr = r_mc6_req_vadr;
   assign mc6_req_size = r_mc6_req_size;
   assign mc6_req_wrd_rdctl = r_mc6_req_wrd_rdctl;

   assign mc7_req_ld = r_mc7_push_ld;
   assign mc7_req_st = r_mc7_push_st;
   assign mc7_req_vadr = r_mc7_req_vadr;
   assign mc7_req_size = r_mc7_req_size;
   assign mc7_req_wrd_rdctl = r_mc7_req_wrd_rdctl;

   assign p0_mc_req_push = p0_mc_req_ld | p0_mc_req_st;
   assign p0_mc_req_ld_st = p0_mc_req_ld;
   assign p1_mc_req_push = p1_mc_req_ld | p1_mc_req_st;
   assign p1_mc_req_ld_st = p1_mc_req_ld;
   assign p2_mc_req_push = p2_mc_req_ld | p2_mc_req_st;
   assign p2_mc_req_ld_st = p2_mc_req_ld;
   assign p3_mc_req_push = p3_mc_req_ld | p3_mc_req_st;
   assign p3_mc_req_ld_st = p3_mc_req_ld;
   assign p4_mc_req_push = p4_mc_req_ld | p4_mc_req_st;
   assign p4_mc_req_ld_st = p4_mc_req_ld;
   assign p5_mc_req_push = p5_mc_req_ld | p5_mc_req_st;
   assign p5_mc_req_ld_st = p5_mc_req_ld;
   assign p6_mc_req_push = p6_mc_req_ld | p6_mc_req_st;
   assign p6_mc_req_ld_st = p6_mc_req_ld;
   assign p7_mc_req_push = p7_mc_req_ld | p7_mc_req_st;
   assign p7_mc_req_ld_st = p7_mc_req_ld;

   assign p0_req_wrd_rdctl_xb = p0_mc_req_ld ? 
                                  {p0_mc_req_wrd_rdctl[63:32], 8'd0, p0_mc_req_wrd_rdctl[23:0]} :
                                   p0_mc_req_wrd_rdctl[63:0];
   assign p1_req_wrd_rdctl_xb = p1_mc_req_ld ? 
                                  {p1_mc_req_wrd_rdctl[63:32], 8'd1, p1_mc_req_wrd_rdctl[23:0]} :
                                   p1_mc_req_wrd_rdctl[63:0];
   assign p2_req_wrd_rdctl_xb = p2_mc_req_ld ? 
                                  {p2_mc_req_wrd_rdctl[63:32], 8'd2, p2_mc_req_wrd_rdctl[23:0]} :
                                   p2_mc_req_wrd_rdctl[63:0];
   assign p3_req_wrd_rdctl_xb = p3_mc_req_ld ? 
                                  {p3_mc_req_wrd_rdctl[63:32], 8'd3, p3_mc_req_wrd_rdctl[23:0]} :
                                   p3_mc_req_wrd_rdctl[63:0];
   assign p4_req_wrd_rdctl_xb = p4_mc_req_ld ? 
                                  {p4_mc_req_wrd_rdctl[63:32], 8'd4, p4_mc_req_wrd_rdctl[23:0]} :
                                   p4_mc_req_wrd_rdctl[63:0];
   assign p5_req_wrd_rdctl_xb = p5_mc_req_ld ? 
                                  {p5_mc_req_wrd_rdctl[63:32], 8'd5, p5_mc_req_wrd_rdctl[23:0]} :
                                   p5_mc_req_wrd_rdctl[63:0];
   assign p6_req_wrd_rdctl_xb = p6_mc_req_ld ? 
                                  {p6_mc_req_wrd_rdctl[63:32], 8'd6, p6_mc_req_wrd_rdctl[23:0]} :
                                   p6_mc_req_wrd_rdctl[63:0];
   assign p7_req_wrd_rdctl_xb = p7_mc_req_ld ? 
                                  {p7_mc_req_wrd_rdctl[63:32], 8'd7, p7_mc_req_wrd_rdctl[23:0]} :
                                   p7_mc_req_wrd_rdctl[63:0];

   /* ----------      external module calls   ---------- */

   // P0 request fifo
   reqfifo fifo0 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p0_mc_req_push),
     .req_ld_st              (p0_mc_req_ld_st),
     .req_last               (p0_mc_req_last),
     .req_vadr               (p0_mc_req_vadr),
     .req_size               (p0_mc_req_size),
     .req_wrd_rdctl          (p0_req_wrd_rdctl_xb),
     .req_idle               (r_p0_req_idle),
     .req_stall              (r_p0_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p0_grant),
     .mc1_grant                 (r_mc1_p0_grant),
     .mc2_grant                 (r_mc2_p0_grant),
     .mc3_grant                 (r_mc3_p0_grant),
     .mc4_grant                 (r_mc4_p0_grant),
     .mc5_grant                 (r_mc5_p0_grant),
     .mc6_grant                 (r_mc6_p0_grant),
     .mc7_grant                 (r_mc7_p0_grant),
     .mc0_req_                  (r_p0_mc0_req_),
     .mc1_req_                  (r_p0_mc1_req_),
     .mc2_req_                  (r_p0_mc2_req_),
     .mc3_req_                  (r_p0_mc3_req_),
     .mc4_req_                  (r_p0_mc4_req_),
     .mc5_req_                  (r_p0_mc5_req_),
     .mc6_req_                  (r_p0_mc6_req_),
     .mc7_req_                  (r_p0_mc7_req_),
     .mc_req_ld_st              (r_p0_mc_req_ld_st),
     .mc_req_last               (r_p0_mc_req_last),
     .mc_req_vadr               (r_p0_mc_req_vadr),
     .mc_req_size               (r_p0_mc_req_size),
     .mc_req_wrd_rdctl          (r_p0_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[0]),
     .r_undflow_alarm      (r_undflow_alarm[0])
   );

   // P1 request fifo
   reqfifo fifo1 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p1_mc_req_push),
     .req_ld_st              (p1_mc_req_ld_st),
     .req_last               (p1_mc_req_last),
     .req_vadr               (p1_mc_req_vadr),
     .req_size               (p1_mc_req_size),
     .req_wrd_rdctl          (p1_req_wrd_rdctl_xb),
     .req_idle               (r_p1_req_idle),
     .req_stall              (r_p1_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p1_grant),
     .mc1_grant                 (r_mc1_p1_grant),
     .mc2_grant                 (r_mc2_p1_grant),
     .mc3_grant                 (r_mc3_p1_grant),
     .mc4_grant                 (r_mc4_p1_grant),
     .mc5_grant                 (r_mc5_p1_grant),
     .mc6_grant                 (r_mc6_p1_grant),
     .mc7_grant                 (r_mc7_p1_grant),
     .mc0_req_                  (r_p1_mc0_req_),
     .mc1_req_                  (r_p1_mc1_req_),
     .mc2_req_                  (r_p1_mc2_req_),
     .mc3_req_                  (r_p1_mc3_req_),
     .mc4_req_                  (r_p1_mc4_req_),
     .mc5_req_                  (r_p1_mc5_req_),
     .mc6_req_                  (r_p1_mc6_req_),
     .mc7_req_                  (r_p1_mc7_req_),
     .mc_req_ld_st              (r_p1_mc_req_ld_st),
     .mc_req_last               (r_p1_mc_req_last),
     .mc_req_vadr               (r_p1_mc_req_vadr),
     .mc_req_size               (r_p1_mc_req_size),
     .mc_req_wrd_rdctl          (r_p1_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[1]),
     .r_undflow_alarm      (r_undflow_alarm[1])
   );

   // P2 request fifo
   reqfifo fifo2 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p2_mc_req_push),
     .req_ld_st              (p2_mc_req_ld_st),
     .req_last               (p2_mc_req_last),
     .req_vadr               (p2_mc_req_vadr),
     .req_size               (p2_mc_req_size),
     .req_wrd_rdctl          (p2_req_wrd_rdctl_xb),
     .req_idle               (r_p2_req_idle),
     .req_stall              (r_p2_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p2_grant),
     .mc1_grant                 (r_mc1_p2_grant),
     .mc2_grant                 (r_mc2_p2_grant),
     .mc3_grant                 (r_mc3_p2_grant),
     .mc4_grant                 (r_mc4_p2_grant),
     .mc5_grant                 (r_mc5_p2_grant),
     .mc6_grant                 (r_mc6_p2_grant),
     .mc7_grant                 (r_mc7_p2_grant),
     .mc0_req_                  (r_p2_mc0_req_),
     .mc1_req_                  (r_p2_mc1_req_),
     .mc2_req_                  (r_p2_mc2_req_),
     .mc3_req_                  (r_p2_mc3_req_),
     .mc4_req_                  (r_p2_mc4_req_),
     .mc5_req_                  (r_p2_mc5_req_),
     .mc6_req_                  (r_p2_mc6_req_),
     .mc7_req_                  (r_p2_mc7_req_),
     .mc_req_ld_st              (r_p2_mc_req_ld_st),
     .mc_req_last               (r_p2_mc_req_last),
     .mc_req_vadr               (r_p2_mc_req_vadr),
     .mc_req_size               (r_p2_mc_req_size),
     .mc_req_wrd_rdctl          (r_p2_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[2]),
     .r_undflow_alarm      (r_undflow_alarm[2])
   );

   // P3 request fifo
   reqfifo fifo3 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p3_mc_req_push),
     .req_ld_st              (p3_mc_req_ld_st),
     .req_last               (p3_mc_req_last),
     .req_vadr               (p3_mc_req_vadr),
     .req_size               (p3_mc_req_size),
     .req_wrd_rdctl          (p3_req_wrd_rdctl_xb),
     .req_idle               (r_p3_req_idle),
     .req_stall              (r_p3_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p3_grant),
     .mc1_grant                 (r_mc1_p3_grant),
     .mc2_grant                 (r_mc2_p3_grant),
     .mc3_grant                 (r_mc3_p3_grant),
     .mc4_grant                 (r_mc4_p3_grant),
     .mc5_grant                 (r_mc5_p3_grant),
     .mc6_grant                 (r_mc6_p3_grant),
     .mc7_grant                 (r_mc7_p3_grant),
     .mc0_req_                  (r_p3_mc0_req_),
     .mc1_req_                  (r_p3_mc1_req_),
     .mc2_req_                  (r_p3_mc2_req_),
     .mc3_req_                  (r_p3_mc3_req_),
     .mc4_req_                  (r_p3_mc4_req_),
     .mc5_req_                  (r_p3_mc5_req_),
     .mc6_req_                  (r_p3_mc6_req_),
     .mc7_req_                  (r_p3_mc7_req_),
     .mc_req_ld_st              (r_p3_mc_req_ld_st),
     .mc_req_last               (r_p3_mc_req_last),
     .mc_req_vadr               (r_p3_mc_req_vadr),
     .mc_req_size               (r_p3_mc_req_size),
     .mc_req_wrd_rdctl          (r_p3_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[3]),
     .r_undflow_alarm      (r_undflow_alarm[3])
   );

   // P4 request fifo
   reqfifo fifo4 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p4_mc_req_push),
     .req_ld_st              (p4_mc_req_ld_st),
     .req_last               (p4_mc_req_last),
     .req_vadr               (p4_mc_req_vadr),
     .req_size               (p4_mc_req_size),
     .req_wrd_rdctl          (p4_req_wrd_rdctl_xb),
     .req_idle               (r_p4_req_idle),
     .req_stall              (r_p4_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p4_grant),
     .mc1_grant                 (r_mc1_p4_grant),
     .mc2_grant                 (r_mc2_p4_grant),
     .mc3_grant                 (r_mc3_p4_grant),
     .mc4_grant                 (r_mc4_p4_grant),
     .mc5_grant                 (r_mc5_p4_grant),
     .mc6_grant                 (r_mc6_p4_grant),
     .mc7_grant                 (r_mc7_p4_grant),
     .mc0_req_                  (r_p4_mc0_req_),
     .mc1_req_                  (r_p4_mc1_req_),
     .mc2_req_                  (r_p4_mc2_req_),
     .mc3_req_                  (r_p4_mc3_req_),
     .mc4_req_                  (r_p4_mc4_req_),
     .mc5_req_                  (r_p4_mc5_req_),
     .mc6_req_                  (r_p4_mc6_req_),
     .mc7_req_                  (r_p4_mc7_req_),
     .mc_req_ld_st              (r_p4_mc_req_ld_st),
     .mc_req_last               (r_p4_mc_req_last),
     .mc_req_vadr               (r_p4_mc_req_vadr),
     .mc_req_size               (r_p4_mc_req_size),
     .mc_req_wrd_rdctl          (r_p4_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[4]),
     .r_undflow_alarm      (r_undflow_alarm[4])
   );

   // P5 request fifo
   reqfifo fifo5 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p5_mc_req_push),
     .req_ld_st              (p5_mc_req_ld_st),
     .req_last               (p5_mc_req_last),
     .req_vadr               (p5_mc_req_vadr),
     .req_size               (p5_mc_req_size),
     .req_wrd_rdctl          (p5_req_wrd_rdctl_xb),
     .req_idle               (r_p5_req_idle),
     .req_stall              (r_p5_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p5_grant),
     .mc1_grant                 (r_mc1_p5_grant),
     .mc2_grant                 (r_mc2_p5_grant),
     .mc3_grant                 (r_mc3_p5_grant),
     .mc4_grant                 (r_mc4_p5_grant),
     .mc5_grant                 (r_mc5_p5_grant),
     .mc6_grant                 (r_mc6_p5_grant),
     .mc7_grant                 (r_mc7_p5_grant),
     .mc0_req_                  (r_p5_mc0_req_),
     .mc1_req_                  (r_p5_mc1_req_),
     .mc2_req_                  (r_p5_mc2_req_),
     .mc3_req_                  (r_p5_mc3_req_),
     .mc4_req_                  (r_p5_mc4_req_),
     .mc5_req_                  (r_p5_mc5_req_),
     .mc6_req_                  (r_p5_mc6_req_),
     .mc7_req_                  (r_p5_mc7_req_),
     .mc_req_ld_st              (r_p5_mc_req_ld_st),
     .mc_req_last               (r_p5_mc_req_last),
     .mc_req_vadr               (r_p5_mc_req_vadr),
     .mc_req_size               (r_p5_mc_req_size),
     .mc_req_wrd_rdctl          (r_p5_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[5]),
     .r_undflow_alarm      (r_undflow_alarm[5])
   );

   // P6 request fifo
   reqfifo fifo6 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p6_mc_req_push),
     .req_ld_st              (p6_mc_req_ld_st),
     .req_last               (p6_mc_req_last),
     .req_vadr               (p6_mc_req_vadr),
     .req_size               (p6_mc_req_size),
     .req_wrd_rdctl          (p6_req_wrd_rdctl_xb),
     .req_idle               (r_p6_req_idle),
     .req_stall              (r_p6_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p6_grant),
     .mc1_grant                 (r_mc1_p6_grant),
     .mc2_grant                 (r_mc2_p6_grant),
     .mc3_grant                 (r_mc3_p6_grant),
     .mc4_grant                 (r_mc4_p6_grant),
     .mc5_grant                 (r_mc5_p6_grant),
     .mc6_grant                 (r_mc6_p6_grant),
     .mc7_grant                 (r_mc7_p6_grant),
     .mc0_req_                  (r_p6_mc0_req_),
     .mc1_req_                  (r_p6_mc1_req_),
     .mc2_req_                  (r_p6_mc2_req_),
     .mc3_req_                  (r_p6_mc3_req_),
     .mc4_req_                  (r_p6_mc4_req_),
     .mc5_req_                  (r_p6_mc5_req_),
     .mc6_req_                  (r_p6_mc6_req_),
     .mc7_req_                  (r_p6_mc7_req_),
     .mc_req_ld_st              (r_p6_mc_req_ld_st),
     .mc_req_last               (r_p6_mc_req_last),
     .mc_req_vadr               (r_p6_mc_req_vadr),
     .mc_req_size               (r_p6_mc_req_size),
     .mc_req_wrd_rdctl          (r_p6_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[6]),
     .r_undflow_alarm      (r_undflow_alarm[6])
   );

   // P7 request fifo
   reqfifo fifo7 (
     .clk167                    (clk),
     .clk_csr                    (clk_csr),
     .reset167                  (reset),
     .req_push               (p7_mc_req_push),
     .req_ld_st              (p7_mc_req_ld_st),
     .req_last               (p7_mc_req_last),
     .req_vadr               (p7_mc_req_vadr),
     .req_size               (p7_mc_req_size),
     .req_wrd_rdctl          (p7_req_wrd_rdctl_xb),
     .req_idle               (r_p7_req_idle),
     .req_stall              (r_p7_req_stall),
     .clk167_                   (clk),
     .reset167_                 (reset),
     .mc0_grant                 (r_mc0_p7_grant),
     .mc1_grant                 (r_mc1_p7_grant),
     .mc2_grant                 (r_mc2_p7_grant),
     .mc3_grant                 (r_mc3_p7_grant),
     .mc4_grant                 (r_mc4_p7_grant),
     .mc5_grant                 (r_mc5_p7_grant),
     .mc6_grant                 (r_mc6_p7_grant),
     .mc7_grant                 (r_mc7_p7_grant),
     .mc0_req_                  (r_p7_mc0_req_),
     .mc1_req_                  (r_p7_mc1_req_),
     .mc2_req_                  (r_p7_mc2_req_),
     .mc3_req_                  (r_p7_mc3_req_),
     .mc4_req_                  (r_p7_mc4_req_),
     .mc5_req_                  (r_p7_mc5_req_),
     .mc6_req_                  (r_p7_mc6_req_),
     .mc7_req_                  (r_p7_mc7_req_),
     .mc_req_ld_st              (r_p7_mc_req_ld_st),
     .mc_req_last               (r_p7_mc_req_last),
     .mc_req_vadr               (r_p7_mc_req_vadr),
     .mc_req_size               (r_p7_mc_req_size),
     .mc_req_wrd_rdctl          (r_p7_mc_req_wrd_rdctl),
     .r_ovrflow_alarm      (r_ovrflow_alarm[7]),
     .r_undflow_alarm      (r_undflow_alarm[7])
   );

   // MC0 request arbiter.
   reqarb8 arb0 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc0_req_),
     .i_empty1                  (r_p1_mc0_req_),
     .i_empty2                  (r_p2_mc0_req_),
     .i_empty3                  (r_p3_mc0_req_),
     .i_empty4                  (r_p4_mc0_req_),
     .i_empty5                  (r_p5_mc0_req_),
     .i_empty6                  (r_p6_mc0_req_),
     .i_empty7                  (r_p7_mc0_req_),
     .i_ld_stall                (mc0_rd_rq_stall),
     .i_st_stall                (mc0_wr_rq_stall),
     .i_ld_st                   (c_mc0_req_ld_st),
     .o_grant0                  (r_mc0_p0_grant),
     .o_grant1                  (r_mc0_p1_grant),
     .o_grant2                  (r_mc0_p2_grant),
     .o_grant3                  (r_mc0_p3_grant),
     .o_grant4                  (r_mc0_p4_grant),
     .o_grant5                  (r_mc0_p5_grant),
     .o_grant6                  (r_mc0_p6_grant),
     .o_grant7                  (r_mc0_p7_grant),
     .o_gsel                    (r_mc0_gsel),
     .o_push_ld                 (r_mc0_push_ld),
     .o_push_st                 (r_mc0_push_st)
   );

   mux8_115 mux0 ( .s(r_mc0_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc0_req_ld_st, c_mc0_req_vadr, c_mc0_req_size, c_mc0_req_wrd_rdctl}) );

   // MC1 request arbiter.
   reqarb8 arb1 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc1_req_),
     .i_empty1                  (r_p1_mc1_req_),
     .i_empty2                  (r_p2_mc1_req_),
     .i_empty3                  (r_p3_mc1_req_),
     .i_empty4                  (r_p4_mc1_req_),
     .i_empty5                  (r_p5_mc1_req_),
     .i_empty6                  (r_p6_mc1_req_),
     .i_empty7                  (r_p7_mc1_req_),
     .i_ld_stall                (mc1_rd_rq_stall),
     .i_st_stall                (mc1_wr_rq_stall),
     .i_ld_st                   (c_mc1_req_ld_st),
     .o_grant0                  (r_mc1_p0_grant),
     .o_grant1                  (r_mc1_p1_grant),
     .o_grant2                  (r_mc1_p2_grant),
     .o_grant3                  (r_mc1_p3_grant),
     .o_grant4                  (r_mc1_p4_grant),
     .o_grant5                  (r_mc1_p5_grant),
     .o_grant6                  (r_mc1_p6_grant),
     .o_grant7                  (r_mc1_p7_grant),
     .o_gsel                    (r_mc1_gsel),
     .o_push_ld                 (r_mc1_push_ld),
     .o_push_st                 (r_mc1_push_st)
   );

   mux8_115 mux1 ( .s(r_mc1_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc1_req_ld_st, c_mc1_req_vadr, c_mc1_req_size, c_mc1_req_wrd_rdctl}) );

   // MC2 request arbiter.
   reqarb8 arb2 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc2_req_),
     .i_empty1                  (r_p1_mc2_req_),
     .i_empty2                  (r_p2_mc2_req_),
     .i_empty3                  (r_p3_mc2_req_),
     .i_empty4                  (r_p4_mc2_req_),
     .i_empty5                  (r_p5_mc2_req_),
     .i_empty6                  (r_p6_mc2_req_),
     .i_empty7                  (r_p7_mc2_req_),
     .i_ld_stall                (mc2_rd_rq_stall),
     .i_st_stall                (mc2_wr_rq_stall),
     .i_ld_st                   (c_mc2_req_ld_st),
     .o_grant0                  (r_mc2_p0_grant),
     .o_grant1                  (r_mc2_p1_grant),
     .o_grant2                  (r_mc2_p2_grant),
     .o_grant3                  (r_mc2_p3_grant),
     .o_grant4                  (r_mc2_p4_grant),
     .o_grant5                  (r_mc2_p5_grant),
     .o_grant6                  (r_mc2_p6_grant),
     .o_grant7                  (r_mc2_p7_grant),
     .o_gsel                    (r_mc2_gsel),
     .o_push_ld                 (r_mc2_push_ld),
     .o_push_st                 (r_mc2_push_st)
   );

   mux8_115 mux2 ( .s(r_mc2_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc2_req_ld_st, c_mc2_req_vadr, c_mc2_req_size, c_mc2_req_wrd_rdctl}) );

   // MC3 request arbiter.
   reqarb8 arb3 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc3_req_),
     .i_empty1                  (r_p1_mc3_req_),
     .i_empty2                  (r_p2_mc3_req_),
     .i_empty3                  (r_p3_mc3_req_),
     .i_empty4                  (r_p4_mc3_req_),
     .i_empty5                  (r_p5_mc3_req_),
     .i_empty6                  (r_p6_mc3_req_),
     .i_empty7                  (r_p7_mc3_req_),
     .i_ld_stall                (mc3_rd_rq_stall),
     .i_st_stall                (mc3_wr_rq_stall),
     .i_ld_st                   (c_mc3_req_ld_st),
     .o_grant0                  (r_mc3_p0_grant),
     .o_grant1                  (r_mc3_p1_grant),
     .o_grant2                  (r_mc3_p2_grant),
     .o_grant3                  (r_mc3_p3_grant),
     .o_grant4                  (r_mc3_p4_grant),
     .o_grant5                  (r_mc3_p5_grant),
     .o_grant6                  (r_mc3_p6_grant),
     .o_grant7                  (r_mc3_p7_grant),
     .o_gsel                    (r_mc3_gsel),
     .o_push_ld                 (r_mc3_push_ld),
     .o_push_st                 (r_mc3_push_st)
   );

   mux8_115 mux3 ( .s(r_mc3_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc3_req_ld_st, c_mc3_req_vadr, c_mc3_req_size, c_mc3_req_wrd_rdctl}) );

   // MC4 request arbiter.
   reqarb8 arb4 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc4_req_),
     .i_empty1                  (r_p1_mc4_req_),
     .i_empty2                  (r_p2_mc4_req_),
     .i_empty3                  (r_p3_mc4_req_),
     .i_empty4                  (r_p4_mc4_req_),
     .i_empty5                  (r_p5_mc4_req_),
     .i_empty6                  (r_p6_mc4_req_),
     .i_empty7                  (r_p7_mc4_req_),
     .i_ld_stall                (mc4_rd_rq_stall),
     .i_st_stall                (mc4_wr_rq_stall),
     .i_ld_st                   (c_mc4_req_ld_st),
     .o_grant0                  (r_mc4_p0_grant),
     .o_grant1                  (r_mc4_p1_grant),
     .o_grant2                  (r_mc4_p2_grant),
     .o_grant3                  (r_mc4_p3_grant),
     .o_grant4                  (r_mc4_p4_grant),
     .o_grant5                  (r_mc4_p5_grant),
     .o_grant6                  (r_mc4_p6_grant),
     .o_grant7                  (r_mc4_p7_grant),
     .o_gsel                    (r_mc4_gsel),
     .o_push_ld                 (r_mc4_push_ld),
     .o_push_st                 (r_mc4_push_st)
   );

   mux8_115 mux4 ( .s(r_mc4_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc4_req_ld_st, c_mc4_req_vadr, c_mc4_req_size, c_mc4_req_wrd_rdctl}) );

   // MC5 request arbiter.
   reqarb8 arb5 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc5_req_),
     .i_empty1                  (r_p1_mc5_req_),
     .i_empty2                  (r_p2_mc5_req_),
     .i_empty3                  (r_p3_mc5_req_),
     .i_empty4                  (r_p4_mc5_req_),
     .i_empty5                  (r_p5_mc5_req_),
     .i_empty6                  (r_p6_mc5_req_),
     .i_empty7                  (r_p7_mc5_req_),
     .i_ld_stall                (mc5_rd_rq_stall),
     .i_st_stall                (mc5_wr_rq_stall),
     .i_ld_st                   (c_mc5_req_ld_st),
     .o_grant0                  (r_mc5_p0_grant),
     .o_grant1                  (r_mc5_p1_grant),
     .o_grant2                  (r_mc5_p2_grant),
     .o_grant3                  (r_mc5_p3_grant),
     .o_grant4                  (r_mc5_p4_grant),
     .o_grant5                  (r_mc5_p5_grant),
     .o_grant6                  (r_mc5_p6_grant),
     .o_grant7                  (r_mc5_p7_grant),
     .o_gsel                    (r_mc5_gsel),
     .o_push_ld                 (r_mc5_push_ld),
     .o_push_st                 (r_mc5_push_st)
   );

   mux8_115 mux5 ( .s(r_mc5_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc5_req_ld_st, c_mc5_req_vadr, c_mc5_req_size, c_mc5_req_wrd_rdctl}) );

   // MC6 request arbiter.
   reqarb8 arb6 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc6_req_),
     .i_empty1                  (r_p1_mc6_req_),
     .i_empty2                  (r_p2_mc6_req_),
     .i_empty3                  (r_p3_mc6_req_),
     .i_empty4                  (r_p4_mc6_req_),
     .i_empty5                  (r_p5_mc6_req_),
     .i_empty6                  (r_p6_mc6_req_),
     .i_empty7                  (r_p7_mc6_req_),
     .i_ld_stall                (mc6_rd_rq_stall),
     .i_st_stall                (mc6_wr_rq_stall),
     .i_ld_st                   (c_mc6_req_ld_st),
     .o_grant0                  (r_mc6_p0_grant),
     .o_grant1                  (r_mc6_p1_grant),
     .o_grant2                  (r_mc6_p2_grant),
     .o_grant3                  (r_mc6_p3_grant),
     .o_grant4                  (r_mc6_p4_grant),
     .o_grant5                  (r_mc6_p5_grant),
     .o_grant6                  (r_mc6_p6_grant),
     .o_grant7                  (r_mc6_p7_grant),
     .o_gsel                    (r_mc6_gsel),
     .o_push_ld                 (r_mc6_push_ld),
     .o_push_st                 (r_mc6_push_st)
   );

   mux8_115 mux6 ( .s(r_mc6_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc6_req_ld_st, c_mc6_req_vadr, c_mc6_req_size, c_mc6_req_wrd_rdctl}) );

   // MC7 request arbiter.
   reqarb8 arb7 (
     .clk                       (clk),
     .reset                     (reset),
     .i_lastreq0                (r_p0_mc_req_last),
     .i_lastreq1                (r_p1_mc_req_last),
     .i_lastreq2                (r_p2_mc_req_last),
     .i_lastreq3                (r_p3_mc_req_last),
     .i_lastreq4                (r_p4_mc_req_last),
     .i_lastreq5                (r_p5_mc_req_last),
     .i_lastreq6                (r_p6_mc_req_last),
     .i_lastreq7                (r_p7_mc_req_last),
     .i_empty0                  (r_p0_mc7_req_),
     .i_empty1                  (r_p1_mc7_req_),
     .i_empty2                  (r_p2_mc7_req_),
     .i_empty3                  (r_p3_mc7_req_),
     .i_empty4                  (r_p4_mc7_req_),
     .i_empty5                  (r_p5_mc7_req_),
     .i_empty6                  (r_p6_mc7_req_),
     .i_empty7                  (r_p7_mc7_req_),
     .i_ld_stall                (mc7_rd_rq_stall),
     .i_st_stall                (mc7_wr_rq_stall),
     .i_ld_st                   (c_mc7_req_ld_st),
     .o_grant0                  (r_mc7_p0_grant),
     .o_grant1                  (r_mc7_p1_grant),
     .o_grant2                  (r_mc7_p2_grant),
     .o_grant3                  (r_mc7_p3_grant),
     .o_grant4                  (r_mc7_p4_grant),
     .o_grant5                  (r_mc7_p5_grant),
     .o_grant6                  (r_mc7_p6_grant),
     .o_grant7                  (r_mc7_p7_grant),
     .o_gsel                    (r_mc7_gsel),
     .o_push_ld                 (r_mc7_push_ld),
     .o_push_st                 (r_mc7_push_st)
   );

   mux8_115 mux7 ( .s(r_mc7_gsel), .i0({r_p0_mc_req_ld_st, r_p0_mc_req_vadr, r_p0_mc_req_size, r_p0_mc_req_wrd_rdctl}), .i1({r_p1_mc_req_ld_st, r_p1_mc_req_vadr, r_p1_mc_req_size, r_p1_mc_req_wrd_rdctl}), .i2({r_p2_mc_req_ld_st, r_p2_mc_req_vadr, r_p2_mc_req_size, r_p2_mc_req_wrd_rdctl}), .i3({r_p3_mc_req_ld_st, r_p3_mc_req_vadr, r_p3_mc_req_size, r_p3_mc_req_wrd_rdctl}), .i4({r_p4_mc_req_ld_st, r_p4_mc_req_vadr, r_p4_mc_req_size, r_p4_mc_req_wrd_rdctl}), .i5({r_p5_mc_req_ld_st, r_p5_mc_req_vadr, r_p5_mc_req_size, r_p5_mc_req_wrd_rdctl}), .i6({r_p6_mc_req_ld_st, r_p6_mc_req_vadr, r_p6_mc_req_size, r_p6_mc_req_wrd_rdctl}), .i7({r_p7_mc_req_ld_st, r_p7_mc_req_vadr, r_p7_mc_req_size, r_p7_mc_req_wrd_rdctl}), .o({c_mc7_req_ld_st, c_mc7_req_vadr, c_mc7_req_size, c_mc7_req_wrd_rdctl}) );


   /* ----------            registers         ---------- */

   dffs_keep_1 rsk0 ( .clk(clk), .set(i_reset), .d(1'b0), .q(reset) );


   dff_48 reg_vadr0 ( .clk(clk), .d(c_mc0_req_vadr), .q(r_mc0_req_vadr) );
   dff_2 reg_size0 ( .clk(clk), .d(c_mc0_req_size), .q(r_mc0_req_size) );
   dff_64 reg_wdrc0 ( .clk(clk), .d(c_mc0_req_wrd_rdctl), .q(r_mc0_req_wrd_rdctl) );

   dff_48 reg_vadr1 ( .clk(clk), .d(c_mc1_req_vadr), .q(r_mc1_req_vadr) );
   dff_2 reg_size1 ( .clk(clk), .d(c_mc1_req_size), .q(r_mc1_req_size) );
   dff_64 reg_wdrc1 ( .clk(clk), .d(c_mc1_req_wrd_rdctl), .q(r_mc1_req_wrd_rdctl) );

   dff_48 reg_vadr2 ( .clk(clk), .d(c_mc2_req_vadr), .q(r_mc2_req_vadr) );
   dff_2 reg_size2 ( .clk(clk), .d(c_mc2_req_size), .q(r_mc2_req_size) );
   dff_64 reg_wdrc2 ( .clk(clk), .d(c_mc2_req_wrd_rdctl), .q(r_mc2_req_wrd_rdctl) );

   dff_48 reg_vadr3 ( .clk(clk), .d(c_mc3_req_vadr), .q(r_mc3_req_vadr) );
   dff_2 reg_size3 ( .clk(clk), .d(c_mc3_req_size), .q(r_mc3_req_size) );
   dff_64 reg_wdrc3 ( .clk(clk), .d(c_mc3_req_wrd_rdctl), .q(r_mc3_req_wrd_rdctl) );

   dff_48 reg_vadr4 ( .clk(clk), .d(c_mc4_req_vadr), .q(r_mc4_req_vadr) );
   dff_2 reg_size4 ( .clk(clk), .d(c_mc4_req_size), .q(r_mc4_req_size) );
   dff_64 reg_wdrc4 ( .clk(clk), .d(c_mc4_req_wrd_rdctl), .q(r_mc4_req_wrd_rdctl) );

   dff_48 reg_vadr5 ( .clk(clk), .d(c_mc5_req_vadr), .q(r_mc5_req_vadr) );
   dff_2 reg_size5 ( .clk(clk), .d(c_mc5_req_size), .q(r_mc5_req_size) );
   dff_64 reg_wdrc5 ( .clk(clk), .d(c_mc5_req_wrd_rdctl), .q(r_mc5_req_wrd_rdctl) );

   dff_48 reg_vadr6 ( .clk(clk), .d(c_mc6_req_vadr), .q(r_mc6_req_vadr) );
   dff_2 reg_size6 ( .clk(clk), .d(c_mc6_req_size), .q(r_mc6_req_size) );
   dff_64 reg_wdrc6 ( .clk(clk), .d(c_mc6_req_wrd_rdctl), .q(r_mc6_req_wrd_rdctl) );

   dff_48 reg_vadr7 ( .clk(clk), .d(c_mc7_req_vadr), .q(r_mc7_req_vadr) );
   dff_2 reg_size7 ( .clk(clk), .d(c_mc7_req_size), .q(r_mc7_req_size) );
   dff_64 reg_wdrc7 ( .clk(clk), .d(c_mc7_req_wrd_rdctl), .q(r_mc7_req_wrd_rdctl) );


   /* ---------- debug & synopsys off blocks  ---------- */

`ifdef DEBUG
   //always @(posedge clk) begin
   //  if (r_t4_valid)
   //    $display("%m i=%d j=%d c=%d ci=%d cqd=%d", r_t4_ix,r_t4_jx,r_t4_c,r_t4_ci,r_t4_cqd);
   //end
`endif
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk167), .reset_n(~reset), .test_expr(r_unimplemented_inst));

   // TODO Assertion checks:

   // synopsys translate_on

endmodule // pdk_req_mxb
