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
   input  [47:0]    mem_last_offst,

   output [63:0]    sum,
   output           sum_vld,
   output           sum_ovrflw,
   output           res_ovrflw
   );

   /* ----------         include files        ---------- */

   parameter ADDR_MASK = 4'b0;
   parameter RSP_PORT = 1'b0;

   /* ----------          wires & regs        ---------- */

   wire [47:0]  c_last_addr1;
   wire [47:0]  c_last_addr2;
   wire [47:0]  c_last_addr3;
   reg  [47:0]  r_last_addr1;
   reg  [47:0]  r_last_addr2;
   reg  [47:0]  r_last_addr3;

   reg          r_reset;
   reg          r_start, r_start2;
   wire         idle_reset;

   reg  [2:0]   nxt_state;
   reg  [2:0]   r_state;

   wire [7:0]   op1_req_tid;
   wire [7:0]   op2_req_tid;
   wire [7:0]   c_req_tid;
   reg  [7:0]   r_req_tid;
   wire [7:0]   c_rsp_tid;
   reg  [7:0]   r_rsp_tid;

   wire [47:0]  ld_addr1;
   wire [47:0]  ld_addr2;
   wire [47:0]  st_addr;

   reg  [40:0]  c_ld_addr1_sh;
   reg  [40:0]  r_ld_addr1_sh;
   reg  [40:0]  c_ld_addr2_sh;
   reg  [40:0]  r_ld_addr2_sh;

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
   reg  [40:0]  c_st_addr_sh;
   reg  [40:0]  r_st_addr_sh;
   wire [63:0]  res_st_data;
   wire [64:0]  c_sum;
   wire         c_sum_vld;
   reg          r_sum_vld;
   //wire  [64:0]  r_sum;
   reg  [64:0]  r_sum;
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
   reg          r_mc_rd_rq_stall;
   reg          r_mc_wr_rq_stall;

   localparam IDLE   = 3'b000;
   localparam LD_OP1 = 3'b001;
   localparam LD_OP2 = 3'b010;
   localparam ST_RES = 3'b011;
   localparam AD_CHK = 3'b100;

   localparam RESQ_AFULL_CNT = 'd124;

   /* ----------      combinatorial blocks    ---------- */

  // leda W484 off Possible loss of carry/borrow in addition/subtraction

  // Outputs to smpl_pers
  assign sum = r_sum2[63:0];
  assign sum_vld = r_sum_vld;
  assign sum_ovrflw = r_sum2[64] && r_sum_vld;
  assign res_ovrflw = r_result[64] && r_result_vld;

  assign idle_reset = r_reset || idle;

  // MC interface 
  assign mc_req_ld = r_req_ld;
  assign mc_req_st = r_req_st;
  assign mc_req_vadr = r_req_vadr;
  assign mc_req_wrd_rdctl = r_req_wrd_rdctl;

  assign mc_rsp_stall = 1'b0;
  assign c_resq_afull = (res_qcnt >= RESQ_AFULL_CNT);

  assign c_last_addr1 = mem_base1 + mem_last_offst;
  assign c_last_addr2 = mem_base2 + mem_last_offst;
  assign c_last_addr3 = mem_base3 + mem_last_offst;

  assign ld_addr1 = {r_ld_addr1_sh[40:3], ADDR_MASK, r_ld_addr1_sh[2:0], 3'b000};
  assign ld_addr2 = {r_ld_addr2_sh[40:3], ADDR_MASK, r_ld_addr2_sh[2:0], 3'b000};
  assign st_addr = {r_st_addr_sh[40:3], ADDR_MASK, r_st_addr_sh[2:0], 3'b000};

always @* 
begin
  nxt_state = r_state;
  c_ld_addr1_sh = r_ld_addr1_sh;
  c_ld_addr2_sh = r_ld_addr2_sh;
  c_st_addr_sh = r_st_addr_sh;
  c_op_sel = r_op_sel;
  c_req_ld = 1'b0;
  c_req_st = 1'b0;
  c_req_vadr = r_req_vadr;
  c_req_wrd_rdctl = r_req_wrd_rdctl;
  case (r_state)
    IDLE: 
      begin
        c_op_sel = 1'b0;
        if (r_start2) begin
          c_ld_addr1_sh = {mem_base1[47:10], 3'b0};
          c_ld_addr2_sh = {mem_base2[47:10], 3'b0};
          c_st_addr_sh = {mem_base3[47:10], 3'b0};
          nxt_state = AD_CHK;
        end
      end
    AD_CHK: 
      begin
        if (ld_addr1 < r_last_addr1)
          nxt_state = LD_OP1;
        else
          nxt_state = IDLE;
      end
    LD_OP1: 
      begin
        c_req_vadr = ld_addr1;
        c_req_wrd_rdctl = op_ld_rdctl;
        if (~r_mc_rd_rq_stall && op1_tid_avail)
        begin
          c_req_ld = 1'b1;
          c_ld_addr1_sh = r_ld_addr1_sh + 41'h1;
          nxt_state = LD_OP2;
          c_op_sel = 1'b1;
        end
        if (r_resq_afull || (ld_addr1 >= r_last_addr1))
          nxt_state = ST_RES;
      end
    LD_OP2: 
      begin  
        c_req_vadr = ld_addr2;
        c_req_wrd_rdctl = op_ld_rdctl;
        if (~r_mc_rd_rq_stall && op2_tid_avail)
        begin
          c_req_ld = 1'b1;
          c_ld_addr2_sh = r_ld_addr2_sh + 41'h1;
          nxt_state = LD_OP1;
          c_op_sel = 1'b0;
        end
        if (r_resq_afull || (ld_addr1 >= r_last_addr1))
          nxt_state = ST_RES;
      end
    ST_RES: 
      begin
        c_req_vadr = st_addr; 
        c_req_wrd_rdctl = res_st_data;
        if (~r_mc_wr_rq_stall && ~res_qempty) begin
          c_req_st = 1'b1;
          c_st_addr_sh = r_st_addr_sh + 41'h1;
        end
        if (st_addr >= r_last_addr3)
         nxt_state = IDLE;
        else if (!res_qempty)
         nxt_state = ST_RES;
        else if (~r_op_sel && (ld_addr1 < r_last_addr1) && op1_tid_avail)
         nxt_state = LD_OP1;
        else if (r_op_sel && (ld_addr2 < r_last_addr2) && op2_tid_avail)
         nxt_state = LD_OP2;
      end
    default:  nxt_state = IDLE;
  endcase
end

  // Load operands

  assign c_req_tid = r_op_sel ? op2_req_tid : op1_req_tid;

  assign op_ld_rdctl = {32'b0, r_op_sel, 23'b0, c_req_tid[7:0]};

  assign c_rsp_tid = r_mc_rsp_rdctl[7:0];
  assign op1_wea = r_mc_rsp_push && ~r_mc_rsp_rdctl[31];
  assign op2_wea = r_mc_rsp_push && r_mc_rsp_rdctl[31];

  assign c_result[64:0] = {1'b0, op1_ram_out[63:0]} + {1'b0, op2_ram_out[63:0]};
  assign c_result_vld = c_op1_vld && c_op2_vld && ~r_resq_afull;

  assign c_sum[64:0] = r_result_vld ? r_sum[64:0] + r_result[64:0] : r_sum[64:0];
  assign c_sum_vld = (st_addr >= r_last_addr3);

  // leda W484 on Possible loss of carry/borrow in addition/subtraction

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

roq_256x64 op1_regs (
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
   .pop         (c_result_vld)    
   );

roq_256x64 op2_regs (
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
   .pop         (c_result_vld)
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
      r_mc_rd_rq_stall <= mc_rd_rq_stall;
      r_mc_wr_rq_stall <= mc_wr_rq_stall;
   end

   always @(posedge clk) begin
      r_last_addr1 <= c_last_addr1;
      r_last_addr2 <= c_last_addr2;
      r_last_addr3 <= c_last_addr3;
   end

   always @(posedge clk) begin
    if (idle_reset) begin
      r_ld_addr1_sh <= 'd0;
      r_ld_addr2_sh <= 'd0;
      r_st_addr_sh <= 'd0;
      r_op_sel <= 'd0;
      r_req_tid <= 'd0;
      r_rsp_tid <= 'd0;
      r_op1_vld <= 'd0;
      r_op2_vld <= 'd0;
      r_result_vld <= 'd0;
      r_sum_vld <= 'd0;
      r_result <= 'd0;
      r_sum <= 65'b0;
      r_sum2 <= 'd0;
      r_resq_afull <= 'd0;
    end else begin
      r_ld_addr1_sh <= c_ld_addr1_sh;
      r_ld_addr2_sh <= c_ld_addr2_sh;
      r_st_addr_sh <= c_st_addr_sh;
      r_op_sel <= c_op_sel;
      r_req_tid <= c_req_tid;
      r_rsp_tid <= c_rsp_tid;
      r_op1_vld <= c_op1_vld;
      r_op2_vld <= c_op2_vld;
      r_result_vld <= c_result_vld;
      r_sum_vld <= c_sum_vld;
      r_result <= c_result;
      r_sum <= c_sum;
      r_sum2 <= r_sum;
      r_resq_afull <= c_resq_afull;
    end
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

