/*****************************************************************************/
//
// Module          : roq_256x64_dbg.vpp
// Revision        : $Revision: 1.2 $
// Last Modified On: $Date: 2010/10/28 22:40:24 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : gedwards
// Created On      : Wed Oct 10 09:26:08 2007
//
//-----------------------------------------------------------------------------
//
// Description     : Reordering queue for memory loads
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: roq_256x64_dbg.vpp,v 1.2 2010/10/28 22:40:24 gedwards Exp $ */

`timescale 1 ns / 1 ps

module roq_256x64_dbg (/*AUTOARG*/
   // Outputs
   req_tid, tid_avail, dout, dout_vld, r_rsp_tid_err,
   // Inputs
   clk, reset, req_ld, rsp_push, rsp_tid, rsp_din, pop
   ) ;
  
   /* ----------        port declarations     ---------- */

   input		clk;
   input		reset;

   input		req_ld;
   output [7:0]		req_tid;
   output		tid_avail;

   input		rsp_push;
   input  [7:0]		rsp_tid;
   input  [63:0]	rsp_din;

   output [63:0]	dout;
   output		dout_vld;
   input		pop;

   output		r_rsp_tid_err;

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   wire		 r_t1_req_ld;
   wire  [7:0]   c_t0_req_tid;
   wire  [7:0]   r_t1_req_tid;
   wire  [7:0]   r_t2_req_tid;

   reg   [255:0] c_t1_tid_vec;
   wire  [255:0] r_t2_tid_vec;
   wire  [255:0] c_t0_tid_free;
   wire  [255:0] r_t1_tid_free;
   wire  [255:0] c_t0_tid_lock;
   wire  [255:0] r_t1_tid_lock;

   wire  [7:0]   c_t0_rptr;
   wire  [7:0]   r_t1_rptr;
   reg   [8:0]   c_tid_free_cnt;
   wire  [8:0]   r_tid_free_cnt;

   wire		 r_t1_rsp_push;

   wire  [7:0]   _nc00_;

   wire		 c_t1_dout_vld;
   wire		 r_t2_dout_vld;

   wire [63:0]	bram_dout;
   wire [2:0]	fifo_cnt, r_fifo_cnt;
   wire		fifo_full;
   wire		fifo_empty;
   wire		fifo_push;

   wire		c_t0_rsp_tid_active;
   wire		r_t1_rsp_tid_active;
   wire		c_rsp_tid_err;
   wire		r_rsp_tid_err;

   /* ----------      combinatorial blocks    ---------- */

// leda W484 off Possible loss of carry/borrow in addition/subtraction
   assign req_tid = r_t1_req_tid;

   assign c_t0_tid_lock = 1'b1 << r_t1_req_tid;
   assign c_t0_tid_free = 1'b1 << rsp_tid;

   assign c_t0_rsp_tid_active = r_t2_tid_vec >> rsp_tid;
   assign c_rsp_tid_err = !r_t1_rsp_tid_active && r_t1_rsp_push;

   always @* begin
    case ({r_t1_req_ld, r_t1_rsp_push})
	2'b00:  c_t1_tid_vec = r_t2_tid_vec;
	2'b01:  c_t1_tid_vec = r_t2_tid_vec & ~r_t1_tid_free;
	2'b10:  c_t1_tid_vec = r_t2_tid_vec | r_t1_tid_lock;
	2'b11:  c_t1_tid_vec = (r_t2_tid_vec & ~r_t1_tid_free) | r_t1_tid_lock;
    endcase
   end

   assign c_t1_dout_vld = ~r_t2_tid_vec[r_t1_rptr] && (r_t2_req_tid != r_t1_rptr);
   assign fifo_push = c_t1_dout_vld && !fifo_full;

   always @* begin
    case ({req_ld, pop})
	2'b00:  c_tid_free_cnt = r_tid_free_cnt;
	2'b01:  c_tid_free_cnt = r_tid_free_cnt + 9'b1;
	2'b10:  c_tid_free_cnt = r_tid_free_cnt - 9'b1;
	2'b11:  c_tid_free_cnt = r_tid_free_cnt;
    endcase
   end

   assign tid_avail = (r_tid_free_cnt > 9'd1);
   assign c_t0_rptr = r_t1_rptr + fifo_push;
   assign c_t0_req_tid = r_t1_req_tid + req_ld;
// leda W484 off Possible loss of carry/borrow in addition/subtraction

   dpram512x72 dpram (
      .clka     (clk),
      .dina     ({8'b0, rsp_din[63:0]}),
      .addra    ({1'b0, rsp_tid[7:0]}),
      .wea      (rsp_push),
      .clkb     (clk),
      .addrb    ({1'b0, c_t0_rptr[7:0]}),
      .enb      (1'b1),
      .doutb    ({_nc00_, bram_dout[63:0]})
   );

   assign dout_vld = !fifo_empty;

// leda B_3208 off Unequal length LHS and RHS in assignment
// leda B_3209 off Unequal length port and connection in module instantiation
   defparam rsp_fifo.DEPTH = 4;
   defparam rsp_fifo.WIDTH = 64;
   fifo rsp_fifo (
    .clk    (clk),
    .reset  (reset),
    .push   (fifo_push),
    .din    (bram_dout),
    .full   (fifo_full),
    .cnt    (fifo_cnt),
    .oclk   (clk),
    .pop    (pop),
    .dout   (dout),
    .empty  (fifo_empty),
    .rcnt   ()
);
// leda B_3208 on Unequal length LHS and RHS in assignment
// leda B_3209 on Unequal length port and connection in module instantiation

 
   /* ----------      external module calls   ---------- */


   /* ----------            registers         ---------- */

   dffc_1 reg_req_ld ( .clk(clk), .reset(reset), .d(req_ld), .q(r_t1_req_ld) );
   dffc_8 reg_t1_req_tid ( .clk(clk), .reset(reset), .d(c_t0_req_tid), .q(r_t1_req_tid) );
   dffc_8 reg_t2_req_tid ( .clk(clk), .reset(reset), .d(r_t1_req_tid), .q(r_t2_req_tid) );
   dffc_8 reg_rptr ( .clk(clk), .reset(reset), .d(c_t0_rptr), .q(r_t1_rptr) );
   dffsi_9 reg_tid_free_cnt ( .clk(clk), .reset(reset), .init(9'd256), .d(c_tid_free_cnt), .q(r_tid_free_cnt) );
   dffc_256 reg_tid_lock ( .clk(clk), .reset(reset), .d(c_t0_tid_lock), .q(r_t1_tid_lock) );
   dffc_256 reg_tid_free ( .clk(clk), .reset(reset), .d(c_t0_tid_free), .q(r_t1_tid_free) );
   dffc_256 reg_tid_vec ( .clk(clk), .reset(reset), .d(c_t1_tid_vec), .q(r_t2_tid_vec) );

   dffc_1 reg_rsp_push ( .clk(clk), .reset(reset), .d(rsp_push), .q(r_t1_rsp_push) );
   dffc_1 reg_dout_vld ( .clk(clk), .reset(reset), .d(c_t1_dout_vld), .q(r_t2_dout_vld) );

   dffc_3 reg_fifo_cnt ( .clk(clk), .reset(reset), .d(fifo_cnt), .q(r_fifo_cnt) );

   dffc_1 reg_t1_rsp_tid_active ( .clk(clk), .reset(reset), .d(c_t0_rsp_tid_active), .q(r_t1_rsp_tid_active) );
   dffc_1 reg_rsp_tid_err ( .clk(clk), .reset(reset), .d(c_rsp_tid_err), .q(r_rsp_tid_err) );

   /* ---------- debug & synopsys off blocks  ---------- */
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // roq_256x64

// This is the search path for the autoinst commands in emacs.
// After modification you must save file, then reld with C-x C-v
//
// Local Variables:
// verilog-library-directories:("." "../../common/xilinx") 
// End:

