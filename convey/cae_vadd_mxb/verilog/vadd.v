/*****************************************************************************/
//
// Module          : vadd.vpp
// Revision        : $Revision: 1.7 $
// Last Modified On: $Date: 2010/10/22 21:27:26 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : gedwards
// Created On      : Wed Oct 10 09:26:08 2007
//
//-----------------------------------------------------------------------------
//
// Description     : Sample personality vector add unit
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: vadd.vpp,v 1.7 2010/10/22 21:27:26 gedwards Exp $ */

`timescale 1 ns / 1 ps

module vadd (
   
   // synthesis attribute keep_hierarchy vadd "true";

   /* ----------        port declarations     ---------- */

   input            clk,
   input            reset,

   output           idle,
   input            start,

   output           mc_req_ld,
   output           mc_req_st,
   output  [47:0]   mc_req_vadr,
   output  [63:0]   mc_req_wrd_rdctl,
   input            mc_rd_rq_stall,
   input            mc_wr_rq_stall,

   input  [31:0]    mc_rsp_rdctl,
   input  [63:0]    mc_rsp_data,
   input            mc_rsp_push,
   output           mc_rsp_stall,

   input  [31:0]    edge_count,
   input  [8:0]     num_threads,
   input  [47:0]    base_address

   );

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   reg          r_reset;
   reg          r_start;

   reg  [2:0]   nxt_state;
   reg  [2:0]   r_state;

   reg  [8:0]   c_base_count;
   reg  [8:0]   r_base_count;
   reg  [31:0]  c_ld_count;
   reg  [31:0]  r_ld_count;
   reg  [31:0]  c_rsp_count;
   reg  [31:0]  r_rsp_count;
   wire [47:0]  fifo_dout_vadr;
   wire [31:0]  fifo_dout_rdctl;

   reg          c_fifo_push;
   reg          c_fifo_pop;
   wire         req_fifo_full;
   wire         req_fifo_afull;
   wire         req_fifo_empty;
   wire [5:0]   req_fifo_cnt;
   reg          c_req_ld;
   reg          r_req_ld;
   reg          c_req_st;
   reg          r_req_st;
   reg  [47:0]  c_req_vadr;
   reg  [47:0]  r_req_vadr;
   reg  [63:0]  c_req_wrd_rdctl;
   reg  [63:0]  r_req_wrd_rdctl;
   reg          r_mc_rsp_push;
   reg  [31:0]  r_mc_rsp_rdctl;
   reg  [63:0]  r_mc_rsp_data;

   reg          r_mc_rd_rq_stall;
   reg          r_mc_wr_rq_stall;

   localparam IDLE     = 3'd0;
   localparam LD_BASES = 3'd1;
   localparam RUN      = 3'd2;

   /* ----------      combinatorial blocks    ---------- */

   // MC interface 
   assign mc_req_ld = r_req_ld;
   assign mc_req_st = r_req_st;
   assign mc_req_vadr = r_req_vadr;
   assign mc_req_wrd_rdctl = r_req_wrd_rdctl;

   assign req_fifo_afull = (req_fifo_cnt > 'd28);
   assign mc_rsp_stall = (r_state == LD_BASES) || req_fifo_afull;
   assign idle = ~start && (r_state == IDLE);
   
   always @* 
    begin
      c_req_st = 1'b0;
      c_req_ld = 1'b0;
      c_fifo_push = 1'b0;
      c_fifo_pop = 1'b0;
      c_req_wrd_rdctl = r_req_wrd_rdctl;
      nxt_state = r_state;
      c_base_count = r_base_count;
      c_ld_count = r_ld_count;
      c_rsp_count = r_rsp_count;
      c_req_vadr = r_req_vadr;
      case (r_state)
        IDLE: 
          begin
            if (start) begin // was r_start
              nxt_state = LD_BASES;
              c_base_count = 'd0;
              c_req_vadr = base_address;
              c_rsp_count = edge_count;
              c_ld_count = edge_count;
            end
          end
        LD_BASES: 
          begin
            if (!r_mc_rd_rq_stall) begin
              c_req_ld = 1'b1;
              c_base_count = r_base_count + 1'b1;
              c_ld_count = r_ld_count - 1'b1;
              c_req_wrd_rdctl = c_base_count;
              c_req_vadr = r_req_vadr + 'h8;
              if (r_base_count == (num_threads - 1)) begin
                nxt_state = RUN;
              end
            end
          end
        RUN: 
          begin
            c_fifo_push = r_mc_rsp_push && (r_ld_count > 'd0);
            c_ld_count = r_mc_rsp_push ? r_ld_count - 1'b1 : r_ld_count;
            c_rsp_count = r_mc_rsp_push ? r_rsp_count - 1'b1 : r_rsp_count;
            c_req_ld = (!r_mc_rd_rq_stall && !req_fifo_empty);
            c_fifo_pop = c_req_ld;
            c_req_wrd_rdctl = fifo_dout_rdctl[31:0];
            c_req_vadr = fifo_dout_vadr[47:0];
            if ((r_rsp_count == 'd0) && (req_fifo_empty)) begin
              nxt_state = IDLE;
            end
          end
        default:  nxt_state = IDLE;
      endcase
    end

   /* ----------      external module calls   ---------- */

   wire [23:0] _nc00_;

   defparam request_q.DEPTH = 32;
   defparam request_q.WIDTH = (48+32);
   fifo request_q (
    .clk    (clk),
    .reset  (r_reset),
    .push   (c_fifo_push),
    .din    ({r_mc_rsp_rdctl[31:0], r_mc_rsp_data[47:0]}),
    .full   (req_fifo_full),
    .cnt    (req_fifo_cnt),
    .oclk   (clk),
    .pop    (c_fifo_pop),
    .dout   ({fifo_dout_rdctl[31:0], fifo_dout_vadr[47:0]}),
    .empty  (req_fifo_empty),
    .rcnt   ()
   );

   /* ----------            registers         ---------- */

   always @(posedge clk) begin
      r_reset <= reset;
   end

   always @(posedge clk) begin
    if (r_reset) begin
      r_start <= 1'b0;
      r_state <= 'b0;
    end else begin
      r_start <= start;
      r_state <= nxt_state;
    end
   end

   always @(posedge clk) begin
      r_req_ld <= c_req_ld;
      r_req_st <= c_req_st;
      r_req_vadr <= c_req_vadr;
      r_req_wrd_rdctl <= c_req_wrd_rdctl;
      r_mc_rd_rq_stall <= mc_rd_rq_stall;
      r_mc_wr_rq_stall <= mc_wr_rq_stall;
   end

   always @(posedge clk) begin
      r_ld_count <= c_ld_count;
      r_rsp_count <= c_rsp_count;
      r_base_count <= c_base_count;
   end

   always @(posedge clk) begin
    if (r_reset) begin
      r_mc_rsp_push <= 1'b0;
      r_mc_rsp_rdctl <= 1'b0;
      r_mc_rsp_data <= 1'b0;
    end else begin
      r_mc_rsp_push <= mc_rsp_push;
      r_mc_rsp_rdctl <= mc_rsp_rdctl;
      r_mc_rsp_data <= mc_rsp_data;
    end
   end
      

   /* ---------- debug & synopsys off blocks  ---------- */
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // vadd

// This is the search path for the autoinst commands in emacs.
// After modification you must save file, then reld with C-x C-v
//
// Local Variables:
// verilog-library-directories:("." "../../common/xilinx") 
// End:

