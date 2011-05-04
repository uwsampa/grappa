/*****************************************************************************/
//
// Module          : rspfifo.vpp
// Revision        : $Revision: 1.3 $
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
// Description     : MC response XBAR FIFO.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: rspfifo.vpp,v 1.3 2010/10/22 19:39:51 gedwards Exp $ */

// leda FM_2_23 off Non driven output ports or signals
module rspfifo (/*AUTOARG*/
   // Outputs
   mc_rsp_stall, p0_rsp_, p1_rsp_, p2_rsp_, p3_rsp_, p4_rsp_, p5_rsp_,
   p6_rsp_, p7_rsp_, rsp_last, rsp_rdctl, rsp_data, r_ovrflow_alarm,
   r_undflow_alarm,
   // Inputs
   clk167_, reset167_, mc_rsp_push, mc_rsp_rdctl, mc_rsp_data, clk167,
   reset167, clk_csr, p0_grant, p1_grant, p2_grant, p3_grant,
   p4_grant, p5_grant, p6_grant, p7_grant
   ) ;

   // synthesis attribute keep_hierarchy rspfifo "true";

   /* ----------        port declarations     ---------- */

   // MC response interface.

   input                   clk167_;
   input                   reset167_;

   input                   mc_rsp_push;
   input  [31:0]           mc_rsp_rdctl;
   input  [63:0]           mc_rsp_data;

   output                  mc_rsp_stall;

   // response arbiter interface.

   input                   clk167;
   input                   reset167;
   input                   clk_csr;

   input                   p0_grant;
   input                   p1_grant;
   input                   p2_grant;
   input                   p3_grant;
   input                   p4_grant;
   input                   p5_grant;
   input                   p6_grant;
   input                   p7_grant;

   output                  p0_rsp_;
   output                  p1_rsp_;
   output                  p2_rsp_;
   output                  p3_rsp_;
   output                  p4_rsp_;
   output                  p5_rsp_;
   output                  p6_rsp_;
   output                  p7_rsp_;

   output                  rsp_last;
   output [31:0]           rsp_rdctl;
   output [63:0]           rsp_data;

   output                  r_ovrflow_alarm;
   output                  r_undflow_alarm;

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   wire                    r_mc_rsp_push;
   wire                    c_mc_rsp_last;
   wire [31:0]             r_mc_rsp_rdctl;
   wire [63:0]             r_mc_rsp_data;

   wire                    r_rsp_stall;

   wire                    r_rsp_last;
   wire [31:0]             r_rsp_rdctl;
   wire [63:0]             r_rsp_data;

   wire                    r_empty;

   wire                    c_pop;
   wire                    c_hold;

   wire                    c_p0_rsp_;
   wire                    c_p1_rsp_;
   wire                    c_p2_rsp_;
   wire                    c_p3_rsp_;
   wire                    c_p4_rsp_;
   wire                    c_p5_rsp_;
   wire                    c_p6_rsp_;
   wire                    c_p7_rsp_;

   wire                    c_p_rsp_last;
   wire [31:0]             c_p_rsp_rdctl;
   wire [63:0]             c_p_rsp_data;

   wire                    r_p0_rsp_;
   wire                    r_p1_rsp_;
   wire                    r_p2_rsp_;
   wire                    r_p3_rsp_;
   wire                    r_p4_rsp_;
   wire                    r_p5_rsp_;
   wire                    r_p6_rsp_;
   wire                    r_p7_rsp_;

   wire                    r_p_rsp_last;
   wire [31:0]             r_p_rsp_rdctl;
   wire [63:0]             r_p_rsp_data;

   /* ----------      combinatorial blocks    ---------- */

   // Output assignments.
   assign mc_rsp_stall = r_rsp_stall;

   assign p0_rsp_ = r_p0_rsp_;
   assign p1_rsp_ = r_p1_rsp_;
   assign p2_rsp_ = r_p2_rsp_;
   assign p3_rsp_ = r_p3_rsp_;
   assign p4_rsp_ = r_p4_rsp_;
   assign p5_rsp_ = r_p5_rsp_;
   assign p6_rsp_ = r_p6_rsp_;
   assign p7_rsp_ = r_p7_rsp_;

   assign rsp_last = r_p_rsp_last;
   assign rsp_rdctl = r_p_rsp_rdctl;
   assign rsp_data = r_p_rsp_data;


   // Next response to different port
   assign c_mc_rsp_last = (mc_rsp_rdctl[18:16] != r_mc_rsp_rdctl[18:16]);

   // Response pending with no grant, hold.
   assign c_hold = |(~{r_p0_rsp_, r_p1_rsp_, r_p2_rsp_, r_p3_rsp_,
                       r_p4_rsp_, r_p5_rsp_, r_p6_rsp_, r_p7_rsp_} &
                     ~{p0_grant, p1_grant, p2_grant, p3_grant,
                       p4_grant, p5_grant, p6_grant, p7_grant}) & ~reset167;

   // Pop fifo when not empty, unless response with no grant.
   assign c_pop = ~r_empty & ~c_hold;

   assign c_p0_rsp_ = (c_hold) ? r_p0_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd0);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd0);
   assign c_p1_rsp_ = (c_hold) ? r_p1_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd1);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd1);
   assign c_p2_rsp_ = (c_hold) ? r_p2_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd2);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd2);
   assign c_p3_rsp_ = (c_hold) ? r_p3_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd3);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd3);
   assign c_p4_rsp_ = (c_hold) ? r_p4_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd4);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd4);
   assign c_p5_rsp_ = (c_hold) ? r_p5_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd5);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd5);
   assign c_p6_rsp_ = (c_hold) ? r_p6_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd6);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd6);
   assign c_p7_rsp_ = (c_hold) ? r_p7_rsp_ :
                       r_empty | (r_rsp_rdctl[31:24] != 8'd7);
   //                    r_empty | (r_rsp_rdctl[18:16] != 3'd7);

   assign c_p_rsp_last = (c_hold) ? r_p_rsp_last : r_rsp_last;
   assign c_p_rsp_rdctl = (c_hold) ? r_p_rsp_rdctl : r_rsp_rdctl;
   assign c_p_rsp_data = (c_hold) ? r_p_rsp_data : r_rsp_data;

   wire fifo_full;
   wire c_fifo_overflow;
   wire r_fifo_overflow;
   wire c_fifo_underflow;
   wire r_fifo_underflow;
   wire c_ovrflow_alarm;
   wire r_ovrflow_alarm;
   wire c_undflow_alarm;
   wire r_undflow_alarm;
   assign c_fifo_overflow = r_mc_rsp_push && fifo_full;
   assign c_fifo_underflow = c_pop && r_empty;

   assign c_ovrflow_alarm = c_fifo_overflow | r_fifo_overflow;
   assign c_undflow_alarm = c_fifo_underflow | r_fifo_underflow;

   /* ----------      external module calls   ---------- */

   // MC response fifo
   wire  [5:0]          _nc1_, _nc2_;

   defparam fifo.WIDTH = 97;
   defparam fifo.DEPTH = 32;
   defparam fifo.RATIO = 0;  // 0=clk/clk 1=clk/clk_
   defparam fifo.WMTHLD = 24;
   // leda B_3208 off Unequal length LHS and RHS in assignment
   // leda B_3209 off Unequal length port and connection in module instantiation
   wmfifo fifo (
     .clk                       (clk167_),
     .reset                     (reset167_),
     .push                      (r_mc_rsp_push),
     .din                       ({c_mc_rsp_last, r_mc_rsp_rdctl, r_mc_rsp_data}),
     .full                      (fifo_full),
     .hiwm                      (r_rsp_stall),
     .cnt                       (_nc1_),
     .oclk                      (clk167),
     .pop                       (c_pop),
     .dout                      ({r_rsp_last, r_rsp_rdctl, r_rsp_data}),
     .empty                     (r_empty),
     .rcnt                      (_nc2_)
   );
   // leda B_3208 on Unequal length LHS and RHS in assignment
   // leda B_3209 on Unequal length port and connection in module instantiation


   /* ----------            registers         ---------- */

   dff_1 reg_mcpush ( .clk(clk167_), .d(mc_rsp_push), .q(r_mc_rsp_push) );
   dff_32 reg_mcrdctl ( .clk(clk167_), .d(mc_rsp_rdctl), .q(r_mc_rsp_rdctl) );
   dff_64 reg_mcdata ( .clk(clk167_), .d(mc_rsp_data), .q(r_mc_rsp_data) );

   dffs_1 reg_rsp0 ( .clk(clk167), .set(reset167_), .d(c_p0_rsp_), .q(r_p0_rsp_) );
   dffs_1 reg_rsp1 ( .clk(clk167), .set(reset167_), .d(c_p1_rsp_), .q(r_p1_rsp_) );
   dffs_1 reg_rsp2 ( .clk(clk167), .set(reset167_), .d(c_p2_rsp_), .q(r_p2_rsp_) );
   dffs_1 reg_rsp3 ( .clk(clk167), .set(reset167_), .d(c_p3_rsp_), .q(r_p3_rsp_) );
   dffs_1 reg_rsp4 ( .clk(clk167), .set(reset167_), .d(c_p4_rsp_), .q(r_p4_rsp_) );
   dffs_1 reg_rsp5 ( .clk(clk167), .set(reset167_), .d(c_p5_rsp_), .q(r_p5_rsp_) );
   dffs_1 reg_rsp6 ( .clk(clk167), .set(reset167_), .d(c_p6_rsp_), .q(r_p6_rsp_) );
   dffs_1 reg_rsp7 ( .clk(clk167), .set(reset167_), .d(c_p7_rsp_), .q(r_p7_rsp_) );
   dff_1 reg_last ( .clk(clk167), .d(c_p_rsp_last), .q(r_p_rsp_last) );
   dff_32 reg_rdctl ( .clk(clk167), .d(c_p_rsp_rdctl), .q(r_p_rsp_rdctl) );
   dff_64 reg_data ( .clk(clk167), .d(c_p_rsp_data), .q(r_p_rsp_data) );

   dff_1 reg_fifo_underflow ( .clk(clk167), .d(c_fifo_underflow), .q(r_fifo_underflow) );
   dff_1 reg_fifo_overflow ( .clk(clk167), .d(c_fifo_overflow), .q(r_fifo_overflow) );

   dff_1 reg_ovrflow_alarm ( .clk(clk_csr), .d(c_ovrflow_alarm), .q(r_ovrflow_alarm) );
   dff_1 reg_undflow_alarm ( .clk(clk_csr), .d(c_undflow_alarm), .q(r_undflow_alarm) );

endmodule // rspfifo
// leda FM_2_23 on Non driven output ports or signals
