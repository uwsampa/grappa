/*****************************************************************************/
//
// Module          : vadd.v
// Revision        : $Revision: 1.9 $
// Last Modified On: $Date: 2010/10/28 22:41:11 $
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
/* $Id: vadd.v,v 1.9 2010/10/28 22:41:11 gedwards Exp $ */

`timescale 1 ns / 1 ps

// leda W459 off Constant is extended to the implied width of 32 bits
// leda VER_2_10_3_6 off Vector size missing in constant assignment
module vadd (
   
   // synthesis attribute keep_hierarchy vadd "true";

   /* ----------        port declarations     ---------- */

   input            clk,
   input            reset,

   input            idle,
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

   input  [47:0]    mem_base1,
   input  [47:0]    mem_base2,
   input  [47:0]    mem_base3,
   input  [47:0]    mem_offset,
   input  [31:0]    fplen,

   output [63:0]    sum,
   output           sum_vld,
   output           sum_ovrflw,
   output           res_ovrflw,

   output           rsp_invalid_fp,
   output           rsp_tid_err
   );

   /* ----------         include files        ---------- */

   parameter ADDR_MASK = 4'b0;
   parameter RSP_PORT = 1'b0;

   /* ----------          wires & regs        ---------- */
   /*AUTOWIRE*/
   /*AUTOREG*/

   reg          r_reset;
   reg          r_start, r_start2;

   reg  [2:0]   nxt_state;
   reg  [2:0]   r_state;

   wire [7:0]   op1_req_tid;
   wire [7:0]   op2_req_tid;
   wire [7:0]   c_req_tid;
   reg  [7:0]   r_req_tid;
   wire [7:0]   c_rsp_tid;
   reg  [7:0]   r_rsp_tid;
   wire [7:0]   c_rsp_fp;

   reg  [47:0]  c_ld_addr1;
   reg  [47:0]  r_ld_addr1;
   reg  [47:0]  c_ld_addr2;
   reg  [47:0]  r_ld_addr2;

   reg          c_op_sel;
   reg          r_op_sel;

   wire [63:0]  op_ld_rdctl;

   wire [64:0]  c_result;
   reg  [64:0]  r_result;
   wire         c_result_vld;
   reg          r_result_vld;
   wire         res_qfull;
   wire         res_qempty;
   wire [7:0]   res_qcnt;
   reg  [47:0]  c_st_addr;
   reg  [47:0]  r_st_addr;
   wire [63:0]  res_st_data;
   wire [64:0]  c_sum;
   reg          c_sum_vld;
   reg          r_sum_vld;
   reg  [64:0]   r_sum;
   reg  [64:0]  r_sum2;
   wire [63:0]  op1_ram_out;
   wire [63:0]  op2_ram_out;
   wire         op1_wea;
   wire         op2_wea;
   wire         c_op1_vld;
   reg          r_op1_vld;
   wire         c_op2_vld;
   reg          r_op2_vld;
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
   wire         c_resq_afull;
   reg          r_resq_afull;

   wire         op1_tid_avail;
   wire         op2_tid_avail;

   reg          r_rd_rq_stall;
   reg          r_wr_rq_stall;

   reg  [31:0]  c_ld_cnt1;
   reg  [31:0]  r_ld_cnt1;
   reg  [31:0]  c_ld_cnt2;
   reg  [31:0]  r_ld_cnt2;
   reg  [31:0]  c_st_cnt;
   reg  [31:0]  r_st_cnt;

   wire [31:0]  c_op1_rsp_cnt;
   reg  [31:0]  r_op1_rsp_cnt;
   wire [31:0]  c_op2_rsp_cnt;
   reg  [31:0]  r_op2_rsp_cnt;

   wire         c_rsp_invalid_fp;
   reg          r_rsp_invalid_fp;
   wire         r_rsp_tid_err1;
   wire         r_rsp_tid_err2;
   wire         c_rsp_tid_err;
   reg          r_rsp_tid_err;

   localparam IDLE   = 3'b000;
   localparam LD_OP1 = 3'b001;
   localparam LD_OP2 = 3'b010;
   localparam ST_RES = 3'b011;
   localparam AD_CHK = 3'b100;
   localparam WAIT = 3'b101;

   localparam RESQ_AFULL_CNT = 'd124;

   /* ----------      combinatorial blocks    ---------- */

  // Outputs to smpl_pers
  assign sum = r_sum2[63:0];
  assign sum_vld = r_sum_vld;
  assign sum_ovrflw = r_sum2[64] && r_sum_vld;
  assign res_ovrflw = r_result[64] && r_result_vld;
  assign rsp_invalid_fp = r_rsp_invalid_fp;
  assign rsp_tid_err = r_rsp_tid_err;
  assign c_rsp_tid_err = (r_rsp_tid_err1 || r_rsp_tid_err2) || r_rsp_tid_err;

  // MC interface 
  assign mc_req_ld = r_req_ld;
  assign mc_req_st = r_req_st;
  assign mc_req_vadr = r_req_vadr;
  assign mc_req_wrd_rdctl = r_req_wrd_rdctl;

  assign mc_rsp_stall = 1'b0;
  assign c_resq_afull = (res_qcnt >= RESQ_AFULL_CNT);

// leda W484 off Possible loss of carry/borrow in addition/subtraction
always @* 
begin
  nxt_state = r_state;
  c_ld_addr1 = r_ld_addr1;
  c_ld_addr2 = r_ld_addr2;
  c_st_addr = r_st_addr;
  c_op_sel = r_op_sel;
  c_req_ld = 1'b0;
  c_req_st = 1'b0;
  c_req_vadr = r_req_vadr;
  c_req_wrd_rdctl = r_req_wrd_rdctl;
  c_ld_cnt1 = r_ld_cnt1;
  c_ld_cnt2 = r_ld_cnt2;
  c_st_cnt = r_st_cnt;
  c_sum_vld = (r_st_cnt >= fplen) || r_sum_vld;
  case (r_state)
    IDLE: 
      begin
        c_op_sel = 1'b0;
        c_ld_cnt1 = r_start ? 'd0 : r_ld_cnt1;
        c_ld_cnt2 = r_start ? 'd0 : r_ld_cnt2;
        c_st_cnt = r_start ? 'd0 : r_st_cnt;
        c_sum_vld = 1'b0;
        if (r_start2) begin
          c_ld_addr1 = mem_base1 + mem_offset;
          c_ld_addr2 = mem_base2 + mem_offset;
          c_st_addr = mem_base3 + mem_offset;
          nxt_state = AD_CHK;
          c_req_vadr = r_ld_addr1;
        end
      end
    AD_CHK: 
      begin
        if (fplen > 'd0)
          nxt_state = LD_OP1;
        else begin
          c_sum_vld = 1'b1;
          nxt_state = WAIT;
        end
      end
    LD_OP1: 
      begin
        c_req_vadr = r_ld_addr1;
        c_req_wrd_rdctl = op_ld_rdctl;
        if (~r_rd_rq_stall && op1_tid_avail)
        begin
          c_req_ld = 1'b1;
          c_ld_addr1 = r_ld_addr1 + 48'h8;
          nxt_state = LD_OP2;
          c_op_sel = 1'b1;
          c_ld_cnt1 = r_ld_cnt1 + 32'h1;
        end
        if (r_resq_afull || (r_ld_cnt1 >= fplen))
          nxt_state = ST_RES;
      end
    LD_OP2: 
      begin  
        c_req_vadr = r_ld_addr2;
        c_req_wrd_rdctl = op_ld_rdctl;
        if (~r_rd_rq_stall && op2_tid_avail)
        begin
          c_req_ld = 1'b1;
          c_ld_addr2 = r_ld_addr2 + 48'h8;
          nxt_state = LD_OP1;
          c_op_sel = 1'b0;
          c_ld_cnt2 = r_ld_cnt2 + 32'h1;
        end
        if (r_resq_afull || (r_ld_cnt1 >= fplen))
          nxt_state = ST_RES;
      end
    ST_RES: 
      begin
        c_req_vadr = r_st_addr; 
        c_req_wrd_rdctl = res_st_data;
        if (~r_wr_rq_stall && ~res_qempty) begin
          c_req_st = 1'b1;
          c_st_addr = r_st_addr + 48'h8;
          c_st_cnt = r_st_cnt + 'd1;
        end
        if ((r_st_cnt >= fplen) && (r_op1_rsp_cnt >= fplen) && (r_op2_rsp_cnt >= fplen))
         nxt_state = WAIT;
        else if (!res_qempty)
         nxt_state = ST_RES;
        else if (~r_op_sel && (r_ld_cnt1 < fplen) && op1_tid_avail)
         nxt_state = LD_OP1;
        else if (r_op_sel && (r_ld_cnt2 < fplen) && op2_tid_avail)
         nxt_state = LD_OP2;
      end
    WAIT: 
      begin
        if (idle)
         nxt_state = IDLE;
      end
    default:  nxt_state = IDLE;
  endcase
end

  // Load operands

  assign c_req_tid = r_op_sel ? op2_req_tid : op1_req_tid;

  assign op_ld_rdctl = {32'b0, 8'b0, 7'b0, r_op_sel, 8'b0, c_req_tid[7:0]};

  assign c_rsp_tid = r_mc_rsp_rdctl[7:0];
  assign c_rsp_fp = r_mc_rsp_rdctl[31:24];
  assign op1_wea = r_mc_rsp_push && ~r_mc_rsp_rdctl[16];
  assign op2_wea = r_mc_rsp_push && r_mc_rsp_rdctl[16];

  assign c_op1_rsp_cnt = r_start ? 32'd0 : r_op1_rsp_cnt + {31'd0, op1_wea};
  assign c_op2_rsp_cnt = r_start ? 32'd0 : r_op2_rsp_cnt + {31'd0, op2_wea};

  assign c_result[64:0] = {1'b0, op1_ram_out[63:0]} + {1'b0, op2_ram_out[63:0]};
  assign c_result_vld = c_op1_vld && c_op2_vld && ~r_resq_afull;

  assign c_sum[64:0] = r_result_vld ? r_sum[64:0] + r_result[64:0] : r_sum[64:0];
// leda W484 on Possible loss of carry/borrow in addition/subtraction

  assign c_rsp_invalid_fp = (r_mc_rsp_push && (c_rsp_fp[7:0] != {4'h0, (ADDR_MASK & 4'h7)})) || r_rsp_invalid_fp;

   /* ----------      external module calls   ---------- */

   wire  [7:0]   _nc0_;

   defparam result_q.DEPTH = 128;
   defparam result_q.WIDTH = 64;
   // leda B_3208 off Unequal length LHS and RHS in assignment
   // leda B_3209 off Unequal length port and connection in module instantiation
   fifo result_q (
    .clk    (clk),
    .reset  (r_reset),
    .push   (r_result_vld),
    .din    (r_result[63:0]),
    .full   (res_qfull),
    .cnt    (res_qcnt),
    .oclk   (clk),
    .pop    (c_req_st),
    .dout   (res_st_data[63:0]),
    .empty  (res_qempty),
    .rcnt   (_nc0_)
);
   // leda B_3208 on Unequal length LHS and RHS in assignment
   // leda B_3209 on Unequal length port and connection in module instantiation

roq_256x64_dbg op1_regs (
   .clk         (clk),
   .reset       (r_reset),
   .req_ld      (c_req_ld & ~r_op_sel),
   .req_tid     (op1_req_tid),
   .tid_avail   (op1_tid_avail),
   .rsp_push    (op1_wea),
   .rsp_tid     (c_rsp_tid),
   .rsp_din     (r_mc_rsp_data),
   .dout        (op1_ram_out),
   .dout_vld    (c_op1_vld),
   .pop         (c_result_vld),
   .r_rsp_tid_err (r_rsp_tid_err1)
   );

roq_256x64_dbg op2_regs (
   .clk         (clk),
   .reset       (r_reset),
   .req_ld      (c_req_ld & r_op_sel),
   .req_tid     (op2_req_tid),
   .tid_avail   (op2_tid_avail),
   .rsp_push    (op2_wea),
   .rsp_tid     (c_rsp_tid),
   .rsp_din     (r_mc_rsp_data),
   .dout        (op2_ram_out),
   .dout_vld    (c_op2_vld),
   .pop         (c_result_vld),
   .r_rsp_tid_err (r_rsp_tid_err2)
   );

   /* ----------            registers         ---------- */

   always @(posedge clk) begin
      r_reset <= reset;
   end

   always @(posedge clk) begin
    if (r_reset) begin
      r_start <= 1'b0;
      r_start2 <= 1'b0;
      r_state <= 'b0;
    end else begin
      r_start <= start;
      r_start2 <= r_start;
      r_state <= nxt_state;
    end
   end

   always @(posedge clk) begin
      r_req_ld <= c_req_ld;
      r_req_st <= c_req_st;
      r_req_vadr <= c_req_vadr;
      r_req_wrd_rdctl <= c_req_wrd_rdctl;
      r_rd_rq_stall <= mc_rd_rq_stall;
      r_wr_rq_stall <= mc_wr_rq_stall;
   end

   always @(posedge clk) begin
      r_ld_cnt1 <= c_ld_cnt1;
      r_ld_cnt2 <= c_ld_cnt2;
      r_st_cnt <= c_st_cnt;
      r_op1_rsp_cnt <= c_op1_rsp_cnt;
      r_op2_rsp_cnt <= c_op2_rsp_cnt;
   end

   always @(posedge clk) begin
      r_ld_addr1   <= r_reset ? 48'd0 : c_ld_addr1;
      r_ld_addr2   <= r_reset ? 48'd0 : c_ld_addr2;
      r_st_addr    <= r_reset ? 48'd0 : c_st_addr;
      r_op_sel     <= r_reset ? 1'b0  : c_op_sel;
      r_req_tid    <= r_reset ? 8'd0  : c_req_tid;
      r_rsp_tid    <= r_reset ? 8'd0  : c_rsp_tid;
      r_op1_vld    <= r_reset ? 1'b0  : c_op1_vld;
      r_op2_vld    <= r_reset ? 1'b0  : c_op2_vld;
      r_result_vld <= r_reset ? 1'b0  : c_result_vld;
      r_sum_vld    <= r_reset ? 1'b0  : c_sum_vld;
      r_result     <= r_reset ? 65'd0 : c_result;
      r_sum        <= (r_reset || r_start) ? 65'b0 : c_sum;
      r_sum2       <= r_reset ? 65'd0 : r_sum;
      r_resq_afull <= r_reset ? 1'b0  : c_resq_afull;
      r_rsp_invalid_fp <= (r_reset || r_start) ? 1'b0 : c_rsp_invalid_fp;
      r_rsp_tid_err <= (r_reset || r_start) ? 1'b0 : c_rsp_tid_err;
   end

   always @(posedge clk) begin
    if (r_reset) begin
      r_mc_rsp_push <= 1'b0;
      r_mc_rsp_rdctl <= 32'b0;
      r_mc_rsp_data <= 64'b0;
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
// leda W459 off Constant is extended to the implied width of 32 bits
// leda VER_2_10_3_6 on Vector size missing in constant assignment

// This is the search path for the autoinst commands in emacs.
// After modification you must save file, then reld with C-x C-v
//
// Local Variables:
// verilog-library-directories:("." "../../common/xilinx") 
// End:

