/*****************************************************************************/
//
// Module          : vadd_pair.vpp
// Revision        : $Revision: 1.3 $
// Last Modified On: $Date: 2010/10/22 02:45:21 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : gedwards
// Created On      : Wed Oct 10 09:26:08 2007
//
//-----------------------------------------------------------------------------
//
// Description     : Pair of vector add units
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: vadd_pair.vpp,v 1.3 2010/10/22 02:45:21 gedwards Exp $ */

`timescale 1 ns / 1 ps

module vadd_pair (/*AUTOARG*/
   // Outputs
   mc_req_ld_e, mc_req_st_e, mc_req_wrd_rdctl_e, mc_req_vadr_e,
   mc_rsp_stall_e, sum_e, sum_vld_e, sum_ovrflw_e, res_ovrflw_e,
   rsp_invalid_fp_e, rsp_tid_err_e, mc_req_ld_o, mc_req_st_o,
   mc_req_wrd_rdctl_o, mc_req_vadr_o, mc_rsp_stall_o, sum_o,
   sum_vld_o, sum_ovrflw_o, res_ovrflw_o, rsp_invalid_fp_o,
   rsp_tid_err_o,
   // Inputs
   clk, reset, idle, start, mem_base1, mem_base2, mem_base3,
   mem_offset_e, fplen_e, mc_rd_rq_stall_e, mc_wr_rq_stall_e,
   mc_rsp_rdctl_e, mc_rsp_data_e, mc_rsp_push_e, mem_offset_o,
   fplen_o, mc_rd_rq_stall_o, mc_wr_rq_stall_o, mc_rsp_rdctl_o,
   mc_rsp_data_o, mc_rsp_push_o
   ) ;

   // synthesis attribute keep_hierarchy vadd_pair "true";
   
   /* ----------        port declarations     ---------- */

   input          clk;
   input          reset;
   input          idle;
   input          start;
   input  [47:0]  mem_base1;
   input  [47:0]  mem_base2;
   input  [47:0]  mem_base3;
   input  [47:0]  mem_offset_e;
   input  [31:0]  fplen_e;
   input          mc_rd_rq_stall_e;
   input          mc_wr_rq_stall_e;
   input  [31:0]  mc_rsp_rdctl_e;
   input  [63:0]  mc_rsp_data_e;
   input          mc_rsp_push_e;
   output         mc_req_ld_e;
   output         mc_req_st_e;
   output [63:0]  mc_req_wrd_rdctl_e;
   output [47:0]  mc_req_vadr_e;
   output         mc_rsp_stall_e;
   output [63:0]  sum_e;
   output         sum_vld_e;
   output         sum_ovrflw_e;
   output         res_ovrflw_e;
   output         rsp_invalid_fp_e;
   output         rsp_tid_err_e;

   input  [47:0]  mem_offset_o;
   input  [31:0]  fplen_o;
   input          mc_rd_rq_stall_o;
   input          mc_wr_rq_stall_o;
   input  [31:0]  mc_rsp_rdctl_o;
   input  [63:0]  mc_rsp_data_o;
   input          mc_rsp_push_o;
   output         mc_req_ld_o;
   output         mc_req_st_o;
   output [63:0]  mc_req_wrd_rdctl_o;
   output [47:0]  mc_req_vadr_o;
   output         mc_rsp_stall_o;
   output [63:0]  sum_o;
   output         sum_vld_o;
   output         sum_ovrflw_o;
   output         res_ovrflw_o;
   output         rsp_invalid_fp_o;
   output         rsp_tid_err_o;

   /*AUTOINPUT*/
   /*AUTOOUTPUT*/
   /* ----------         include files        ---------- */
   parameter MC_MASK = 3'b000;

   /* ----------          wires & regs        ---------- */
   wire         mc_req_ld_e;
   wire         mc_req_st_e;
   wire [63:0]  mc_req_wrd_rdctl_e;
   wire [47:0]  mc_req_vadr_e;
   wire         mc_rsp_stall_e;
   wire [63:0]  sum_e;
   wire         sum_vld_e;
   wire         sum_ovrflw_e;
   wire         res_ovrflw_e;


   wire         mc_req_ld_o;
   wire         mc_req_st_o;
   wire [63:0]  mc_req_wrd_rdctl_o;
   wire [47:0]  mc_req_vadr_o;
   wire         mc_rsp_stall_o;
   wire [63:0]  sum_o;
   wire         sum_vld_o;
   wire         sum_ovrflw_o;
   wire         res_ovrflw_o;

   /* ----------      combinatorial blocks    ---------- */

   /* ----------      external module calls   ---------- */

   defparam add_e.ADDR_MASK = {1'b0, MC_MASK};
   defparam add_e.RSP_PORT = 1'b0;
   vadd add_e
     (
      //Outputs
      .mc_req_ld        (mc_req_ld_e),
      .mc_req_st        (mc_req_st_e),
      .mc_req_wrd_rdctl (mc_req_wrd_rdctl_e),
      .mc_req_vadr      (mc_req_vadr_e),
      .mc_rsp_stall     (mc_rsp_stall_e),
      .sum              (sum_e),
      .sum_vld          (sum_vld_e),
      .sum_ovrflw       (sum_ovrflw_e),
      .res_ovrflw       (res_ovrflw_e),
      .rsp_invalid_fp   (rsp_invalid_fp_e),
      .rsp_tid_err      (rsp_tid_err_e),
      //Inputs
      .clk              (clk),
      .reset            (reset),
      .idle             (idle),
      .start            (start),
      .mem_base1        (mem_base1),
      .mem_base2        (mem_base2),
      .mem_base3        (mem_base3),
      .mem_offset       (mem_offset_e),
      .fplen            (fplen_e),
      .mc_rd_rq_stall   (mc_rd_rq_stall_e),
      .mc_wr_rq_stall   (mc_wr_rq_stall_e),
      .mc_rsp_rdctl     (mc_rsp_rdctl_e[31:0]),
      .mc_rsp_data      (mc_rsp_data_e[63:0]),
      .mc_rsp_push      (mc_rsp_push_e)
      );

   defparam add_o.ADDR_MASK = {1'b1, MC_MASK};
   defparam add_o.RSP_PORT = 1'b1;
   vadd add_o
     (
      //Outputs
      .mc_req_ld        (mc_req_ld_o),
      .mc_req_st        (mc_req_st_o),
      .mc_req_wrd_rdctl (mc_req_wrd_rdctl_o),
      .mc_req_vadr      (mc_req_vadr_o),
      .mc_rsp_stall     (mc_rsp_stall_o),
      .sum              (sum_o[63:0]),
      .sum_vld          (sum_vld_o),
      .sum_ovrflw       (sum_ovrflw_o),
      .res_ovrflw       (res_ovrflw_o),
      .rsp_invalid_fp   (rsp_invalid_fp_o),
      .rsp_tid_err      (rsp_tid_err_o),
      //Inputs
      .clk              (clk),
      .reset            (reset),
      .idle             (idle),
      .start            (start),
      .mem_base1        (mem_base1),
      .mem_base2        (mem_base2),
      .mem_base3        (mem_base3),
      .mem_offset       (mem_offset_o),
      .fplen            (fplen_o),
      .mc_rd_rq_stall   (mc_rd_rq_stall_o),
      .mc_wr_rq_stall   (mc_wr_rq_stall_o),
      .mc_rsp_rdctl     (mc_rsp_rdctl_o[31:0]),
      .mc_rsp_data      (mc_rsp_data_o[63:0]),
      .mc_rsp_push      (mc_rsp_push_o)
      );


   /* ----------            registers         ---------- */

   /* ---------- debug & synopsys off blocks  ---------- */
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // smpl_pers

