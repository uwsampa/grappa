/*****************************************************************************/
//
// Module	: wmfifo.v
// Revision	: $Revision: 1.2 $
// Last Modified On: $Date: 2010/10/12 16:34:37 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Description:	FIFO which XST can infer the storage as a
//		distributed RAM. The FIFO supports asynchronous
//		operation by specifying the proper RATIO.
//
//		Since push/pops travel between both clock
//		domains, the ratio needs to be configured
//		as:
//
//		RATIO = MAX(1, CEIL(FastFreq/SlowFreq));
//
//		This is identical to common/lib/fifo.v with two
//		modifications:
//		1. A hiwm (high water mark) output is added. The
//		   high water mark is set by the WMTHLD parameter.
//		2. Pop is internally qualified with empty. This
//		   allows pop do be driven by a grant from arb8,
//		   which may remain asserted when empty is true.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2008 : Created by Convey Computer Corp. This file is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: wmfifo.v,v 1.2 2010/10/12 16:34:37 gedwards Exp $ */

`timescale 1 ns / 1 ps

module wmfifo (
    clk, reset,
    push, din, full, hiwm, cnt,
    oclk,
    pop, dout, empty, rcnt
);

// synthesis attribute keep_hierarchy wmfifo "true";

parameter WIDTH = 256;
parameter DEPTH = 16;
parameter WMTHLD = 8;
parameter PIPE = 1;
parameter RATIO = 0;
parameter RAM_STYLE = "distributed";
parameter QUIESCENT = 0;
localparam DBIT = f_enc_bits(DEPTH);

input			clk, reset;
input			push;
input  [WIDTH-1:0]	din;
output			full;
output			hiwm;
output [DBIT:0]		cnt;
input			oclk;
input			pop;
output [WIDTH-1:0]	dout;
output			empty;
output [DBIT:0]		rcnt;

`include "functions.vh"

    localparam ARAT = RATIO < 0 ? -RATIO : RATIO;
    localparam SBIT = ARAT < 1 ? 0 : f_enc_bits(ARAT*2);

    function [DBIT-1:0] f_mod_inc;
    input [DBIT-1:0] bits;
    input [DBIT-1:0] mod;
    begin
	f_mod_inc = bits == mod ? 'd0 : bits + 'd1;
    end
    endfunction

    //
    // Push logic
    //
    wire [SBIT:0]	pop_sync;
    reg			r_full;
    reg			r_hiwm;
    wire [DBIT-1:0]	c_wptr;
    reg  [DBIT-1:0]	r_wptr;
    wire [DBIT:0]	c_wcnt;
    reg  [DBIT:0]	r_wcnt;

    assign c_wptr = push ? f_mod_inc(r_wptr, DEPTH-1) : r_wptr;
    assign c_wcnt = (r_wcnt - pop_sync) + push;

    always @(posedge clk) begin
	if (reset) begin
	    r_wptr <= 'd0;
	    r_wcnt <= 'd0;
	    r_full <= 'd0;
	    r_hiwm <= 'd0;
	end else begin
	    r_wptr <= c_wptr;
	    r_wcnt <= c_wcnt;
	    r_full <= c_wcnt >= DEPTH;
	    r_hiwm <= c_wcnt >= WMTHLD;
	end
    end
    assign full = r_full;
    assign hiwm = r_hiwm;
    assign cnt = r_wcnt;

    //
    // RAM storage
    //
    (* ram_style = RAM_STYLE *)
    reg [WIDTH-1:0] mem[DEPTH-1:0];

    always @(posedge clk) begin
	if (push)
	    mem[r_wptr] <= din;
    end

    //
    // Pop logic
    //
    wire		oreset;
    wire [SBIT:0]	push_sync;
    wire		c_pop;
    wire		p_pop;
    reg			r_empty;
    wire [DBIT-1:0]	c_rptr;
    reg  [DBIT-1:0]	r_rptr;
    wire [DBIT:0]	c_rcnt;
    reg  [DBIT:0]	r_rcnt;

    assign c_pop = pop & ~empty;
    assign c_rptr = p_pop ? f_mod_inc(r_rptr, DEPTH-1) : r_rptr;
    assign c_rcnt = (r_rcnt + push_sync) - p_pop;

    always @(posedge oclk) begin
	if (oreset) begin
	    r_rptr  <= 'b0;
	    r_rcnt  <= 'b0;
	    r_empty <= 'b1;
	end else begin
	    r_rptr  <= c_rptr;
	    r_rcnt  <= c_rcnt;
	    r_empty <= !p_pop ? (r_rcnt == 'd0 && ~|push_sync) :
				(r_rcnt == 'd1 && ~|push_sync);
	end
    end
    assign rcnt = r_rcnt;

    // Output pipe stage
    generate if (PIPE>0) begin : g0t
	(* keep = "true" *) reg rp_empty, rp_lcl_empty;
	reg  [WIDTH-1:0]	rp_dout;

	assign p_pop = (c_pop || rp_lcl_empty) && !r_empty;
	wire   c_load = c_pop || (!r_empty && rp_lcl_empty);

	always @(posedge oclk) begin
	    if (oreset) begin
		rp_empty <= 'd1;
		rp_lcl_empty <= 'd1;
	    end else begin
		if (c_load) begin
		    rp_empty <= r_empty;
		    rp_lcl_empty <= r_empty;
		end
	    end
	    if (c_load) rp_dout <=  mem[r_rptr];
	end
	assign empty = rp_empty;
	assign dout = rp_dout;
    end else begin : g0f
	assign p_pop = c_pop;
	assign empty = r_empty;
	assign dout =  mem[r_rptr];
    end endgenerate

    //
    // Syncronizers
    //
    generate if (RATIO == 0) begin
	assign oreset = reset;
    end else begin
	(* TIG = "true" *) wire tig_reset = reset;
	reg	  r_oreset;
	reg [1:0] r_sreset;
	always @(posedge oclk) begin
	r_sreset <= (r_sreset << 1) | tig_reset;
	r_oreset <= |{r_sreset, tig_reset};
	end
	assign oreset = r_oreset;
    end endgenerate

    fsync spush (
	.clk(clk),
	.flg(push),
	.oclk(oclk),
	.oflg(push_sync)
    );
    defparam spush.RATIO = RATIO;
    fsync spop (
	.clk(oclk),
	.flg(c_pop),
	.oclk(clk),
	.oflg(pop_sync)
    );
    defparam spop.RATIO = RATIO;


    //
    // Assertions && Coverage
    //
    // synthesis translate_off
    wire cnt_quiescent = !QUIESCENT ? (r_wcnt == 'd0   && r_rcnt == 'd0) :
    				      (r_wcnt == DEPTH &&
				       (r_rcnt + (PIPE && !empty)) == DEPTH);
    assert_quiescent_state #(0, 1, 0, "***ERROR ASSERT: FIFO not empty at EOS")
	qstate_assert (.clk(clk), .reset_n(!reset), .state_expr(cnt_quiescent), .check_value(1'b1), .sample_event(1'b0));

    wire overflow_seen = push && full;
    assert_never #(0, 2, "***ERROR ASSERT: FIFO overflow")
	oflow_assert (.clk(clk), .reset_n(!reset), .test_expr(overflow_seen));

    wire underflow_seen = c_pop && empty;
    assert_never #(0, 2, "***ERROR ASSERT: FIFO underflow")
	uflow_assert (.clk(oclk), .reset_n(!reset), .test_expr(underflow_seen));
    // synthesis translate_on

endmodule // wmfifo
