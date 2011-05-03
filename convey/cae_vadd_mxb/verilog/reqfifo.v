/*****************************************************************************/
//
// Module          : reqfifo.vpp
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
// Description     : MC request XBAR FIFO.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: reqfifo.vpp,v 1.4 2010/11/02 19:45:15 gedwards Exp $ */

// leda FM_2_23 off Non driven output ports or signals
module reqfifo (/*AUTOARG*/
   // Outputs
   req_idle, req_stall, mc0_req_, mc1_req_, mc2_req_, mc3_req_,
   mc4_req_, mc5_req_, mc6_req_, mc7_req_, mc_req_last, mc_req_ld_st,
   mc_req_vadr, mc_req_size, mc_req_wrd_rdctl, r_ovrflow_alarm,
   r_undflow_alarm,
   // Inputs
   clk167, reset167, clk_csr, req_push, req_last, req_ld_st, req_vadr,
   req_size, req_wrd_rdctl, clk167_, reset167_, mc0_grant, mc1_grant,
   mc2_grant, mc3_grant, mc4_grant, mc5_grant, mc6_grant, mc7_grant
   ) ;

   // synthesis attribute keep_hierarchy reqfifo "true";

   /* ----------        port declarations     ---------- */

   // request interface

   input                   clk167;
   input                   reset167;
   input                   clk_csr;

   input                   req_push;
   input                   req_last;
   input                   req_ld_st;
   input  [47:0]           req_vadr;
   input  [1:0]            req_size;
   input  [63:0]           req_wrd_rdctl;

   output                  req_idle;
   output                  req_stall;

   // MC request arbiter interface.

   input                   clk167_;
   input                   reset167_;

   input                   mc0_grant;
   input                   mc1_grant;
   input                   mc2_grant;
   input                   mc3_grant;
   input                   mc4_grant;
   input                   mc5_grant;
   input                   mc6_grant;
   input                   mc7_grant;

   output                  mc0_req_;
   output                  mc1_req_;
   output                  mc2_req_;
   output                  mc3_req_;
   output                  mc4_req_;
   output                  mc5_req_;
   output                  mc6_req_;
   output                  mc7_req_;

   output                  mc_req_last;
   output                  mc_req_ld_st;
   output [47:0]           mc_req_vadr;
   output [1:0]            mc_req_size;
   output [63:0]           mc_req_wrd_rdctl;

   output                  r_ovrflow_alarm;
   output                  r_undflow_alarm;

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   wire                    r_req_idle;
   wire                    r_req_stall;

   wire                    r_req_last;
   wire                    r_req_ld_st;
   wire [47:0]             r_req_vadr;
   wire [1:0]              r_req_size;
   wire [63:0]             r_req_wrd_rdctl;

   wire                    r_empty;

   wire                    c_pop;
   wire                    c_hold;

   wire                    c_mc0_req_;
   wire                    c_mc1_req_;
   wire                    c_mc2_req_;
   wire                    c_mc3_req_;
   wire                    c_mc4_req_;
   wire                    c_mc5_req_;
   wire                    c_mc6_req_;
   wire                    c_mc7_req_;

   wire                    c_mc_req_last;
   wire                    c_mc_req_ld_st;
   wire [47:0]             c_mc_req_vadr;
   wire [1:0]              c_mc_req_size;
   wire [63:0]             c_mc_req_wrd_rdctl;

   wire                    r_mc0_req_;
   wire                    r_mc1_req_;
   wire                    r_mc2_req_;
   wire                    r_mc3_req_;
   wire                    r_mc4_req_;
   wire                    r_mc5_req_;
   wire                    r_mc6_req_;
   wire                    r_mc7_req_;

   wire                    r_mc_req_last;
   wire                    r_mc_req_ld_st;
   wire [47:0]             r_mc_req_vadr;
   wire [1:0]              r_mc_req_size;
   wire [63:0]             r_mc_req_wrd_rdctl;

   /* ----------      combinatorial blocks    ---------- */

   // Output assignments.
   assign req_idle = r_req_idle;
   assign req_stall = r_req_stall;

   assign mc0_req_ = r_mc0_req_;
   assign mc1_req_ = r_mc1_req_;
   assign mc2_req_ = r_mc2_req_;
   assign mc3_req_ = r_mc3_req_;
   assign mc4_req_ = r_mc4_req_;
   assign mc5_req_ = r_mc5_req_;
   assign mc6_req_ = r_mc6_req_;
   assign mc7_req_ = r_mc7_req_;

   assign mc_req_last = r_mc_req_last;
   assign mc_req_ld_st = r_mc_req_ld_st;
   assign mc_req_vadr = r_mc_req_vadr;
   assign mc_req_size = r_mc_req_size;
   assign mc_req_wrd_rdctl = r_mc_req_wrd_rdctl;


   // Request pending with no grant, hold.
   assign c_hold = |(~{r_mc0_req_, r_mc1_req_, r_mc2_req_, r_mc3_req_,
                       r_mc4_req_, r_mc5_req_, r_mc6_req_, r_mc7_req_} &
                     ~{mc0_grant, mc1_grant, mc2_grant, mc3_grant,
                       mc4_grant, mc5_grant, mc6_grant, mc7_grant}) & ~reset167_;

   // Pop fifo when not empty, unless request with no grant.
   assign c_pop = ~r_empty & ~c_hold;

   assign c_mc0_req_ = (c_hold) ? r_mc0_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd0);
   assign c_mc1_req_ = (c_hold) ? r_mc1_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd1);
   assign c_mc2_req_ = (c_hold) ? r_mc2_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd2);
   assign c_mc3_req_ = (c_hold) ? r_mc3_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd3);
   assign c_mc4_req_ = (c_hold) ? r_mc4_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd4);
   assign c_mc5_req_ = (c_hold) ? r_mc5_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd5);
   assign c_mc6_req_ = (c_hold) ? r_mc6_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd6);
   assign c_mc7_req_ = (c_hold) ? r_mc7_req_ :
                       r_empty | (r_req_vadr[8:6] != 3'd7);

   assign c_mc_req_last = (c_hold) ? r_mc_req_last : r_req_last;
   assign c_mc_req_ld_st = (c_hold) ? r_mc_req_ld_st : r_req_ld_st;
   assign c_mc_req_vadr = (c_hold) ? r_mc_req_vadr : r_req_vadr;
   assign c_mc_req_size = (c_hold) ? r_mc_req_size : r_req_size;
   assign c_mc_req_wrd_rdctl = (c_hold) ? r_mc_req_wrd_rdctl : r_req_wrd_rdctl;

   wire fifo_full;
   wire c_fifo_overflow;
   wire r_fifo_overflow;
   wire c_fifo_underflow;
   wire r_fifo_underflow;
   wire c_ovrflow_alarm;
   wire r_ovrflow_alarm;
   wire c_undflow_alarm;
   wire r_undflow_alarm;
   assign c_fifo_overflow = req_push && fifo_full;
   assign c_fifo_underflow = c_pop && r_empty;
   
   assign c_ovrflow_alarm = c_fifo_overflow | r_fifo_overflow;
   assign c_undflow_alarm = c_fifo_underflow | r_fifo_underflow;

   /* ----------      external module calls   ---------- */

   /* ----------      external module calls   ---------- */

   // request fifo
   wire                 _nc0_;
   wire  [5:0]          _nc1_, _nc2_;

   defparam fifo.WIDTH = 116;
   defparam fifo.DEPTH = 32;
   defparam fifo.RATIO = 0;  // 0=clk/clk 1=clk/clk_
   defparam fifo.WMTHLD = 26;
   // leda B_3208 off Unequal length LHS and RHS in assignment
   // leda B_3209 off Unequal length port and connection in module instantiation
   wmfifo fifo (
     .clk                       (clk167),
     .reset                     (reset167),
     .push                      (req_push),
     .din                       ({req_last, req_ld_st,
                                  req_vadr, req_size, req_wrd_rdctl}),
     .full                      (fifo_full),
     .hiwm                      (r_req_stall),
     .cnt                       (_nc1_),
     .oclk                      (clk167_),
     .pop                       (c_pop),
     .dout                      ({r_req_last, r_req_ld_st,
                                  r_req_vadr, r_req_size, r_req_wrd_rdctl}),
     .empty                     (r_empty),
     .rcnt                      (_nc2_)
   );
   // leda B_3208 on Unequal length LHS and RHS in assignment
   // leda B_3209 on Unequal length port and connection in module instantiation


   /* ----------            registers         ---------- */

   // clk167_ to clk167
   dff_1 reg_reqidle ( .clk(clk167), .d(r_empty), .q(r_req_idle) );

   dffs_1 reg_mcreq0 ( .clk(clk167_), .set(reset167_), .d(c_mc0_req_), .q(r_mc0_req_) );
   dffs_1 reg_mcreq1 ( .clk(clk167_), .set(reset167_), .d(c_mc1_req_), .q(r_mc1_req_) );
   dffs_1 reg_mcreq2 ( .clk(clk167_), .set(reset167_), .d(c_mc2_req_), .q(r_mc2_req_) );
   dffs_1 reg_mcreq3 ( .clk(clk167_), .set(reset167_), .d(c_mc3_req_), .q(r_mc3_req_) );
   dffs_1 reg_mcreq4 ( .clk(clk167_), .set(reset167_), .d(c_mc4_req_), .q(r_mc4_req_) );
   dffs_1 reg_mcreq5 ( .clk(clk167_), .set(reset167_), .d(c_mc5_req_), .q(r_mc5_req_) );
   dffs_1 reg_mcreq6 ( .clk(clk167_), .set(reset167_), .d(c_mc6_req_), .q(r_mc6_req_) );
   dffs_1 reg_mcreq7 ( .clk(clk167_), .set(reset167_), .d(c_mc7_req_), .q(r_mc7_req_) );
   dff_1 reg_mclast ( .clk(clk167_), .d(c_mc_req_last), .q(r_mc_req_last) );
   dff_1 reg_mcldst ( .clk(clk167_), .d(c_mc_req_ld_st), .q(r_mc_req_ld_st) );
   dff_48 reg_mcvadr ( .clk(clk167_), .d(c_mc_req_vadr), .q(r_mc_req_vadr) );
   dff_2 reg_mcsize ( .clk(clk167_), .d(c_mc_req_size), .q(r_mc_req_size) );
   dff_64 reg_mcwdrc ( .clk(clk167_), .d(c_mc_req_wrd_rdctl), .q(r_mc_req_wrd_rdctl) );

   dff_1 reg_fifo_underflow ( .clk(clk167), .d(c_fifo_underflow), .q(r_fifo_underflow) );
   dff_1 reg_fifo_overflow ( .clk(clk167), .d(c_fifo_overflow), .q(r_fifo_overflow) );

   dff_1 reg_ovrflow_alarm ( .clk(clk_csr), .d(c_ovrflow_alarm), .q(r_ovrflow_alarm) );
   dff_1 reg_undflow_alarm ( .clk(clk_csr), .d(c_undflow_alarm), .q(r_undflow_alarm) );

endmodule // reqfifo
// leda FM_2_23 on Non driven output ports or signals
