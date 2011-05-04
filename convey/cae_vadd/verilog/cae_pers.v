/*****************************************************************************/
//
// Module          : cae_pers.vpp
// Revision        : $Revision: 1.9 $
// Last Modified On: $Date: 2010/12/01 16:47:27 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : gedwards
// Created On      : Wed Oct 10 09:26:08 2007
//
//-----------------------------------------------------------------------------
//
// Description     : Sample Custom Personality
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: cae_pers.vpp,v 1.9 2010/12/01 16:47:27 gedwards Exp $ */

`timescale 1 ns / 1 ps

module cae_pers (
   
   // synthesis attribute keep_hierarchy cae_pers "true";

   /* ----------        port declarations     ---------- */

   input            clk_csr,
   input            clk,
   input            clk2x,
   input            i_reset,
   input            i_csr_reset_n,
   input  [1:0]     i_aeid,

   input            ppll_reset,
   output           ppll_locked,
   output           clk_per,

   //
   // Dispatch Interface
   //
   input  [31:0]    cae_inst,
   input  [63:0]    cae_data,
   input            cae_inst_vld,

   output  [17:0]   cae_aeg_cnt,
   output  [15:0]   cae_exception,
   output  [63:0]   cae_ret_data,
   output           cae_ret_data_vld,
   output           cae_idle,
   output           cae_stall,

   //
   // Management/Debug Interface
   //
   input  [3:0]     cae_ring_ctl_in,
   input  [15:0]    cae_ring_data_in,
   output  [3:0]    cae_ring_ctl_out,
   output  [15:0]   cae_ring_data_out,

   input            csr_31_31_intlv_dis,

   //
   // AE-to-AE Interface
   //
   output [63:0]    tx_data,
   output [23:0]    tx_cmd,
   output [1:0]     tx_dest,
   output           tx_valid,
   input            tx_stall,

   input  [63:0]    rx_data,
   input  [23:0]    rx_cmd,
   input  [1:0]     rx_src,
   input            rx_valid,

   //
   // MC Interface
   //
   output         mc0_req_ld_e,
   output         mc0_req_ld_o,
   output         mc0_req_st_e,
   output         mc0_req_st_o,
   output [1:0]   mc0_req_size_e,
   output [1:0]   mc0_req_size_o,
   output [47:0]  mc0_req_vadr_e,
   output [47:0]  mc0_req_vadr_o,
   output [63:0]  mc0_req_wrd_rdctl_e,
   output [63:0]  mc0_req_wrd_rdctl_o,
   output         mc0_rsp_stall_e,
   output         mc0_rsp_stall_o,
   output         mc1_req_ld_e,
   output         mc1_req_ld_o,
   output         mc1_req_st_e,
   output         mc1_req_st_o,
   output [1:0]   mc1_req_size_e,
   output [1:0]   mc1_req_size_o,
   output [47:0]  mc1_req_vadr_e,
   output [47:0]  mc1_req_vadr_o,
   output [63:0]  mc1_req_wrd_rdctl_e,
   output [63:0]  mc1_req_wrd_rdctl_o,
   output         mc1_rsp_stall_e,
   output         mc1_rsp_stall_o,
   output         mc2_req_ld_e,
   output         mc2_req_ld_o,
   output         mc2_req_st_e,
   output         mc2_req_st_o,
   output [1:0]   mc2_req_size_e,
   output [1:0]   mc2_req_size_o,
   output [47:0]  mc2_req_vadr_e,
   output [47:0]  mc2_req_vadr_o,
   output [63:0]  mc2_req_wrd_rdctl_e,
   output [63:0]  mc2_req_wrd_rdctl_o,
   output         mc2_rsp_stall_e,
   output         mc2_rsp_stall_o,
   output         mc3_req_ld_e,
   output         mc3_req_ld_o,
   output         mc3_req_st_e,
   output         mc3_req_st_o,
   output [1:0]   mc3_req_size_e,
   output [1:0]   mc3_req_size_o,
   output [47:0]  mc3_req_vadr_e,
   output [47:0]  mc3_req_vadr_o,
   output [63:0]  mc3_req_wrd_rdctl_e,
   output [63:0]  mc3_req_wrd_rdctl_o,
   output         mc3_rsp_stall_e,
   output         mc3_rsp_stall_o,
   output         mc4_req_ld_e,
   output         mc4_req_ld_o,
   output         mc4_req_st_e,
   output         mc4_req_st_o,
   output [1:0]   mc4_req_size_e,
   output [1:0]   mc4_req_size_o,
   output [47:0]  mc4_req_vadr_e,
   output [47:0]  mc4_req_vadr_o,
   output [63:0]  mc4_req_wrd_rdctl_e,
   output [63:0]  mc4_req_wrd_rdctl_o,
   output         mc4_rsp_stall_e,
   output         mc4_rsp_stall_o,
   output         mc5_req_ld_e,
   output         mc5_req_ld_o,
   output         mc5_req_st_e,
   output         mc5_req_st_o,
   output [1:0]   mc5_req_size_e,
   output [1:0]   mc5_req_size_o,
   output [47:0]  mc5_req_vadr_e,
   output [47:0]  mc5_req_vadr_o,
   output [63:0]  mc5_req_wrd_rdctl_e,
   output [63:0]  mc5_req_wrd_rdctl_o,
   output         mc5_rsp_stall_e,
   output         mc5_rsp_stall_o,
   output         mc6_req_ld_e,
   output         mc6_req_ld_o,
   output         mc6_req_st_e,
   output         mc6_req_st_o,
   output [1:0]   mc6_req_size_e,
   output [1:0]   mc6_req_size_o,
   output [47:0]  mc6_req_vadr_e,
   output [47:0]  mc6_req_vadr_o,
   output [63:0]  mc6_req_wrd_rdctl_e,
   output [63:0]  mc6_req_wrd_rdctl_o,
   output         mc6_rsp_stall_e,
   output         mc6_rsp_stall_o,
   output         mc7_req_ld_e,
   output         mc7_req_ld_o,
   output         mc7_req_st_e,
   output         mc7_req_st_o,
   output [1:0]   mc7_req_size_e,
   output [1:0]   mc7_req_size_o,
   output [47:0]  mc7_req_vadr_e,
   output [47:0]  mc7_req_vadr_o,
   output [63:0]  mc7_req_wrd_rdctl_e,
   output [63:0]  mc7_req_wrd_rdctl_o,
   output         mc7_rsp_stall_e,
   output         mc7_rsp_stall_o,

   input          mc0_rd_rq_stall_e,
   input          mc0_wr_rq_stall_e,
   input          mc0_rd_rq_stall_o,
   input          mc0_wr_rq_stall_o,
   input [63:0]   mc0_rsp_data_e,   
   input [63:0]   mc0_rsp_data_o,   
   input          mc0_rsp_push_e,   
   input          mc0_rsp_push_o,   
   input [31:0]   mc0_rsp_rdctl_e,  
   input [31:0]   mc0_rsp_rdctl_o,  
   input          mc1_rd_rq_stall_e,
   input          mc1_wr_rq_stall_e,
   input          mc1_rd_rq_stall_o,
   input          mc1_wr_rq_stall_o,
   input [63:0]   mc1_rsp_data_e,
   input [63:0]   mc1_rsp_data_o,
   input          mc1_rsp_push_e,
   input          mc1_rsp_push_o,
   input [31:0]   mc1_rsp_rdctl_e,
   input [31:0]   mc1_rsp_rdctl_o,
   input          mc2_rd_rq_stall_e,
   input          mc2_wr_rq_stall_e,
   input          mc2_rd_rq_stall_o,
   input          mc2_wr_rq_stall_o,
   input [63:0]   mc2_rsp_data_e,
   input [63:0]   mc2_rsp_data_o,
   input          mc2_rsp_push_e,
   input          mc2_rsp_push_o,
   input [31:0]   mc2_rsp_rdctl_e,
   input [31:0]   mc2_rsp_rdctl_o,
   input          mc3_rd_rq_stall_e,
   input          mc3_wr_rq_stall_e,
   input          mc3_rd_rq_stall_o,
   input          mc3_wr_rq_stall_o,
   input [63:0]   mc3_rsp_data_e,
   input [63:0]   mc3_rsp_data_o,
   input          mc3_rsp_push_e,
   input          mc3_rsp_push_o,
   input [31:0]   mc3_rsp_rdctl_e,
   input [31:0]   mc3_rsp_rdctl_o,
   input          mc4_rd_rq_stall_e,
   input          mc4_wr_rq_stall_e,
   input          mc4_rd_rq_stall_o,
   input          mc4_wr_rq_stall_o,
   input [63:0]   mc4_rsp_data_e,
   input [63:0]   mc4_rsp_data_o,
   input          mc4_rsp_push_e,
   input          mc4_rsp_push_o,
   input [31:0]   mc4_rsp_rdctl_e,
   input [31:0]   mc4_rsp_rdctl_o,
   input          mc5_rd_rq_stall_e,
   input          mc5_wr_rq_stall_e,
   input          mc5_rd_rq_stall_o,
   input          mc5_wr_rq_stall_o,
   input [63:0]   mc5_rsp_data_e,
   input [63:0]   mc5_rsp_data_o,
   input          mc5_rsp_push_e,
   input          mc5_rsp_push_o,
   input [31:0]   mc5_rsp_rdctl_e,
   input [31:0]   mc5_rsp_rdctl_o,
   input          mc6_rd_rq_stall_e,
   input          mc6_wr_rq_stall_e,
   input          mc6_rd_rq_stall_o,
   input          mc6_wr_rq_stall_o,
   input [63:0]   mc6_rsp_data_e,
   input [63:0]   mc6_rsp_data_o,
   input          mc6_rsp_push_e,
   input          mc6_rsp_push_o,
   input [31:0]   mc6_rsp_rdctl_e,
   input [31:0]   mc6_rsp_rdctl_o,
   input          mc7_rd_rq_stall_e,
   input          mc7_wr_rq_stall_e,
   input          mc7_rd_rq_stall_o,
   input          mc7_wr_rq_stall_o,
   input [63:0]   mc7_rsp_data_e,
   input [63:0]   mc7_rsp_data_o,
   input          mc7_rsp_push_e,
   input          mc7_rsp_push_o,
   input [31:0]   mc7_rsp_rdctl_e,
   input [31:0]   mc7_rsp_rdctl_o,

   //
   // Write flush
   //
   output         mc0_req_flush_e,
   output         mc0_req_flush_o,
   input          mc0_rsp_flush_cmplt_e,
   input          mc0_rsp_flush_cmplt_o,
   output         mc1_req_flush_e,
   output         mc1_req_flush_o,
   input          mc1_rsp_flush_cmplt_e,
   input          mc1_rsp_flush_cmplt_o,
   output         mc2_req_flush_e,
   output         mc2_req_flush_o,
   input          mc2_rsp_flush_cmplt_e,
   input          mc2_rsp_flush_cmplt_o,
   output         mc3_req_flush_e,
   output         mc3_req_flush_o,
   input          mc3_rsp_flush_cmplt_e,
   input          mc3_rsp_flush_cmplt_o,
   output         mc4_req_flush_e,
   output         mc4_req_flush_o,
   input          mc4_rsp_flush_cmplt_e,
   input          mc4_rsp_flush_cmplt_o,
   output         mc5_req_flush_e,
   output         mc5_req_flush_o,
   input          mc5_rsp_flush_cmplt_e,
   input          mc5_rsp_flush_cmplt_o,
   output         mc6_req_flush_e,
   output         mc6_req_flush_o,
   input          mc6_rsp_flush_cmplt_e,
   input          mc6_rsp_flush_cmplt_o,
   output         mc7_req_flush_e,
   output         mc7_req_flush_o,
   input          mc7_rsp_flush_cmplt_e,
   input          mc7_rsp_flush_cmplt_o

   );

   /* ----------         include files        ---------- */
`include "pdk_fpga_param.vh"
   parameter CNT_BITS = 7'd45;

   /* ----------    local clock generation ---------- */
   wire reset_bg;
   generate if (RATIO == 0) begin : gclks
      assign clk_per = clk;
      assign ppll_locked = 1'b1;
      assign reset_bg = i_reset;
   end else begin
      //leda B_1011 off - Module instantiation is not fully bound
      BUFG pll_bufg     (.O(pll_clkfbin), .I(pll_clkfbout));
      BUFG clk_per_bufg (.O(clk_per), .I(pll_clkout));
      PLL_BASE pll (
        .CLKIN(clk),
        .RST(ppll_reset),
        .CLKFBOUT(pll_clkfbout),
        .CLKFBIN(pll_clkfbin),
        .LOCKED(ppll_locked),
        .CLKOUT0(pll_clkout),
        .CLKOUT1(), .CLKOUT2(), .CLKOUT3(), .CLKOUT4(), .CLKOUT5()
      );
      //leda B_1011 off - Module instantiation is not fully bound
      defparam pll.CLKIN_PERIOD = 6.667;        // 150 MHz
      defparam pll.DIVCLK_DIVIDE = 3;
      defparam pll.CLKFBOUT_MULT = 20;          // Fvco = 1000 MHz
      //defparam pll.CLKOUT0_DIVIDE = 13;               // 76.9 MHz
      defparam pll.CLKOUT0_DIVIDE = 6;          // 167 MHz
      //defparam pll.CLKOUT0_DIVIDE = 4;                // 250 MHz

      (* keep = "true" *)(* TIG = "true" *)
      wire [1:0] _AsYnC_reset = {!_AsYnC_reset[0], !i_reset};
      FDS rrst (.C(clk_per), .D(1'b0), .Q(r_reset), .S(_AsYnC_reset[1]));
      BUFG brst (.O(reset_bg), .I(r_reset));
   end endgenerate


   /* ----------          wires & regs        ---------- */

   reg  [63:0]  r_cae_data;
   reg          c_count_max_ovrflw;
   reg  [47:0]  mem_base1;
   reg  [47:0]  mem_base2;
   reg  [47:0]  mem_base3;
   reg  [47:0]  mem_last_offst;
   reg          c_aeg_wren;
   reg          r_aeg_wren;
   reg          c_aeg_rden;
   reg          r_aeg_rden;
   reg          unimplemented_inst;
   reg          unaligned_addr;
   reg          invalid_aeg_idx;
   reg          c_invalid_intlv;
   reg          r_invalid_intlv;
   reg          caep_inst_vld;
   reg  [17:0]  c_aeg_idx;
   reg  [17:0]  r_aeg_idx;
   reg  [63:0]  c_aeg0, c_aeg1, c_aeg2, c_aeg3, c_aeg4;
   reg  [63:0]  r_aeg0, r_aeg1, r_aeg2, r_aeg3, r_aeg4;

   reg  [63:0]  c_aeg10, c_aeg11, c_aeg12, c_aeg13, c_aeg14, c_aeg15, c_aeg16, c_aeg17;
   reg  [63:0]  c_aeg18, c_aeg19, c_aeg20, c_aeg21, c_aeg22, c_aeg23, c_aeg24, c_aeg25;
   reg  [63:0]  r_aeg10, r_aeg11, r_aeg12, r_aeg13, r_aeg14, r_aeg15, r_aeg16, r_aeg17;
   reg  [63:0]  r_aeg18, r_aeg19, r_aeg20, r_aeg21, r_aeg22, r_aeg23, r_aeg24, r_aeg25;

   reg  [63:0]  c_aeg30, c_aeg31, c_aeg32, c_aeg33;
   reg  [63:0]  r_aeg30, r_aeg31, r_aeg32, r_aeg33;

   reg  [63:0]  c_aeg40;
   reg  [63:0]  r_aeg40;

   reg  [63:0]  r_au0_sum;
   reg  [63:0]  r_au1_sum;
   reg  [63:0]  r_au2_sum;
   reg  [63:0]  r_au3_sum;
   reg  [63:0]  r_au4_sum;
   reg  [63:0]  r_au5_sum;
   reg  [63:0]  r_au6_sum;
   reg  [63:0]  r_au7_sum;
   reg  [63:0]  r_au8_sum;
   reg  [63:0]  r_au9_sum;
   reg  [63:0]  r_au10_sum;
   reg  [63:0]  r_au11_sum;
   reg  [63:0]  r_au12_sum;
   reg  [63:0]  r_au13_sum;
   reg  [63:0]  r_au14_sum;
   reg  [63:0]  r_au15_sum;

   wire [63:0]  ap0_sum_e;
   wire [63:0]  ap0_sum_o;
   wire [63:0]  ap1_sum_e;
   wire [63:0]  ap1_sum_o;
   wire [63:0]  ap2_sum_e;
   wire [63:0]  ap2_sum_o;
   wire [63:0]  ap3_sum_e;
   wire [63:0]  ap3_sum_o;
   wire [63:0]  ap4_sum_e;
   wire [63:0]  ap4_sum_o;
   wire [63:0]  ap5_sum_e;
   wire [63:0]  ap5_sum_o;
   wire [63:0]  ap6_sum_e;
   wire [63:0]  ap6_sum_o;
   wire [63:0]  ap7_sum_e;
   wire [63:0]  ap7_sum_o;

   wire         ap0_sum_vld_e;
   wire         ap0_sum_vld_o;
   wire         ap1_sum_vld_e;
   wire         ap1_sum_vld_o;
   wire         ap2_sum_vld_e;
   wire         ap2_sum_vld_o;
   wire         ap3_sum_vld_e;
   wire         ap3_sum_vld_o;
   wire         ap4_sum_vld_e;
   wire         ap4_sum_vld_o;
   wire         ap5_sum_vld_e;
   wire         ap5_sum_vld_o;
   wire         ap6_sum_vld_e;
   wire         ap6_sum_vld_o;
   wire         ap7_sum_vld_e;
   wire         ap7_sum_vld_o;

   wire         ap0_sum_ovrflw_e;
   wire         ap0_sum_ovrflw_o;
   wire         ap1_sum_ovrflw_e;
   wire         ap1_sum_ovrflw_o;
   wire         ap2_sum_ovrflw_e;
   wire         ap2_sum_ovrflw_o;
   wire         ap3_sum_ovrflw_e;
   wire         ap3_sum_ovrflw_o;
   wire         ap4_sum_ovrflw_e;
   wire         ap4_sum_ovrflw_o;
   wire         ap5_sum_ovrflw_e;
   wire         ap5_sum_ovrflw_o;
   wire         ap6_sum_ovrflw_e;
   wire         ap6_sum_ovrflw_o;
   wire         ap7_sum_ovrflw_e;
   wire         ap7_sum_ovrflw_o;

   wire         ap0_res_ovrflw_e;
   wire         ap0_res_ovrflw_o;
   wire         ap1_res_ovrflw_e;
   wire         ap1_res_ovrflw_o;
   wire         ap2_res_ovrflw_e;
   wire         ap2_res_ovrflw_o;
   wire         ap3_res_ovrflw_e;
   wire         ap3_res_ovrflw_o;
   wire         ap4_res_ovrflw_e;
   wire         ap4_res_ovrflw_o;
   wire         ap5_res_ovrflw_e;
   wire         ap5_res_ovrflw_o;
   wire         ap6_res_ovrflw_e;
   wire         ap6_res_ovrflw_o;
   wire         ap7_res_ovrflw_e;
   wire         ap7_res_ovrflw_o;

   reg  [15:0]  r_sum_vld_vec;
   reg  [15:0]  r_sum_ovrflw_vec;
   reg  [15:0]  r_res_ovrflw_vec;
   reg          c_sum_ovrflw;
   reg          r_sum_ovrflw;
   reg          c_res_ovrflw;
   reg          r_res_ovrflw;
   reg          c_caep00;
   reg          r_caep00;
   reg  [63:0]  c_ret_data;
   reg  [63:0]  r_ret_data;
   reg          c_ret_data_vld;
   reg          r_ret_data_vld;

   reg  [63:0]  c_sum_mux;
   reg  [63:0]  r_sum_mux;
   reg  [64:0]  c_sum;
   reg  [64:0]  r_sum;
   reg  [63:0]  r_sum_csr;
   reg          sum_ovrflw;
   reg  [3:0]   c_sum_cnt;
   reg  [3:0]   r_sum_cnt;

   reg  [2:0]   c_state;
   reg  [2:0]   r_state;

   reg          r_idle;
   wire [63:0]  cae_csr_scratch;

   reg          reset;
   localparam IDLE = 3'd0;
   localparam VADD = 3'd1;
   localparam SUM  = 3'd2;
   localparam WAIT = 3'd3;
   localparam TOT  = 3'd4;

   /* ----------      combinatorial blocks    ---------- */

   assign cae_stall = (r_state != IDLE) || c_caep00 || r_caep00;

   // AE-to-AE interface not used
   assign tx_data = 64'b0;
   assign tx_cmd = 24'b0;
   assign tx_dest = 2'b0;
   assign tx_valid = 1'b0;

   // all memory requests are 8-byte
   assign mc0_req_size_e = 2'b11;
   assign mc1_req_size_e = 2'b11;
   assign mc2_req_size_e = 2'b11;
   assign mc3_req_size_e = 2'b11;
   assign mc4_req_size_e = 2'b11;
   assign mc5_req_size_e = 2'b11;
   assign mc6_req_size_e = 2'b11;
   assign mc7_req_size_e = 2'b11;
   assign mc0_req_size_o = 2'b11;
   assign mc1_req_size_o = 2'b11;
   assign mc2_req_size_o = 2'b11;
   assign mc3_req_size_o = 2'b11;
   assign mc4_req_size_o = 2'b11;
   assign mc5_req_size_o = 2'b11;
   assign mc6_req_size_o = 2'b11;
   assign mc7_req_size_o = 2'b11;

   // Write flush
   assign mc0_req_flush_e = 1'b0;
   assign mc0_req_flush_o = 1'b0;
   assign mc1_req_flush_e = 1'b0;
   assign mc1_req_flush_o = 1'b0;
   assign mc2_req_flush_e = 1'b0;
   assign mc2_req_flush_o = 1'b0;
   assign mc3_req_flush_e = 1'b0;
   assign mc3_req_flush_o = 1'b0;
   assign mc4_req_flush_e = 1'b0;
   assign mc4_req_flush_o = 1'b0;
   assign mc5_req_flush_e = 1'b0;
   assign mc5_req_flush_o = 1'b0;
   assign mc6_req_flush_e = 1'b0;
   assign mc6_req_flush_o = 1'b0;
   assign mc7_req_flush_e = 1'b0;
   assign mc7_req_flush_o = 1'b0;

   always @*
    begin
	c_aeg_wren = 1'b0;
	c_aeg_rden = 1'b0;
	c_aeg_idx = 18'b0;
	caep_inst_vld = 1'b0;
	unimplemented_inst = 1'b0;
	casex (cae_inst[28:24])
	  5'b1101?: begin	// Format 4 instructions
	    if (cae_inst[24:18]==7'h40) begin
	      c_aeg_idx[17:0] = cae_data[17:0];
	      c_aeg_wren = cae_inst_vld;
	    end
	    else if (cae_inst[24:18]==7'h68) begin
	      c_aeg_idx[17:0] = cae_data[17:0];
	      c_aeg_rden = cae_inst_vld;
	    end
	    else if (cae_inst[24:18]==7'h70) begin
	      c_aeg_idx[17:0] = {6'h0, cae_inst[17:6]};
	      c_aeg_rden = cae_inst_vld;
	    end
 	    else begin
	      unimplemented_inst = cae_inst_vld;
	    end
	  end
	  5'b11100: begin	// Format 5 instructions
	    if (cae_inst[23:18]==6'h18) begin
	      c_aeg_idx[17:0] = {6'h0, cae_inst[17:12], cae_inst[5:0]};
	      c_aeg_wren = cae_inst_vld;
	    end
            else if(cae_inst[23:18]==6'h20) begin
              c_aeg_idx[17:0] = {6'b0, cae_inst[17:12], cae_inst[5:0]};
              c_aeg_wren  = cae_inst_vld;
            end
 	    else begin
	      unimplemented_inst = cae_inst_vld;
	    end
	  end
	  5'b11101: begin	// Format 6 instructions
	    if (cae_inst[23:18]==6'h1C) begin
	      c_aeg_idx[17:0] = {6'h0, cae_inst[17:6]};
	      c_aeg_rden = cae_inst_vld;
	    end
 	    else begin
	      unimplemented_inst = cae_inst_vld;
	    end
	  end
	  5'b11110: begin	// Format 7 instructions
	    caep_inst_vld = cae_inst_vld & cae_inst[23]; 	// CAEP instructions in range 20-3F
	    unimplemented_inst = cae_inst_vld & ~cae_inst[23];
	  end
	  default: begin	// Format 7 instructions
	    unimplemented_inst = cae_inst_vld;
	  end
	endcase
    end

   always @*
    begin
      c_ret_data_vld = r_aeg_rden;
	case (r_aeg_idx[5:0])
	  6'd0: c_ret_data = r_aeg0;
	  6'd1: c_ret_data = r_aeg1;
	  6'd2: c_ret_data = r_aeg2;
	  6'd3: c_ret_data = r_aeg3;
	  6'd4: c_ret_data = r_aeg4;
	  6'd10: c_ret_data = r_aeg10;
	  6'd11: c_ret_data = r_aeg11;
	  6'd12: c_ret_data = r_aeg12;
	  6'd13: c_ret_data = r_aeg13;
	  6'd14: c_ret_data = r_aeg14;
	  6'd15: c_ret_data = r_aeg15;
	  6'd16: c_ret_data = r_aeg16;
	  6'd17: c_ret_data = r_aeg17;
	  6'd18: c_ret_data = r_aeg18;
	  6'd19: c_ret_data = r_aeg19;
	  6'd20: c_ret_data = r_aeg20;
	  6'd21: c_ret_data = r_aeg21;
	  6'd22: c_ret_data = r_aeg22;
	  6'd23: c_ret_data = r_aeg23;
	  6'd24: c_ret_data = r_aeg24;
	  6'd25: c_ret_data = r_aeg25;
	  6'd30: c_ret_data = r_aeg30;
	  6'd31: c_ret_data = r_aeg31;
	  6'd32: c_ret_data = r_aeg32;
	  6'd33: c_ret_data = r_aeg33;
	  6'd40: c_ret_data = r_aeg40;
          6'd50: c_ret_data = cae_csr_scratch;
	  default: c_ret_data = 64'b0;
	endcase
    end

   always @*
    begin
      invalid_aeg_idx = cae_inst_vld && (|c_aeg_idx[17:8]);

      c_aeg0 = (r_aeg_wren && (r_aeg_idx == 18'd0)) ? r_cae_data : r_aeg0;
      c_aeg1 = (r_aeg_wren && (r_aeg_idx == 18'd1)) ? r_cae_data : r_aeg1;
      c_aeg2 = (r_aeg_wren && (r_aeg_idx == 18'd2)) ? r_cae_data : r_aeg2;
      c_aeg3 = (r_aeg_wren && (r_aeg_idx == 18'd3)) ? r_cae_data : r_aeg3;
      c_aeg4 = (r_aeg_wren && (r_aeg_idx == 18'd4)) ? r_cae_data : r_aeg4;

      c_aeg10 = (r_aeg_wren && (r_aeg_idx == 18'd10)) ? r_cae_data : (r_state == SUM) ? r_au0_sum : r_aeg10;
      c_aeg11 = (r_aeg_wren && (r_aeg_idx == 18'd11)) ? r_cae_data : (r_state == SUM) ? r_au1_sum : r_aeg11;
      c_aeg12 = (r_aeg_wren && (r_aeg_idx == 18'd12)) ? r_cae_data : (r_state == SUM) ? r_au2_sum : r_aeg12;
      c_aeg13 = (r_aeg_wren && (r_aeg_idx == 18'd13)) ? r_cae_data : (r_state == SUM) ? r_au3_sum : r_aeg13;
      c_aeg14 = (r_aeg_wren && (r_aeg_idx == 18'd14)) ? r_cae_data : (r_state == SUM) ? r_au4_sum : r_aeg14;
      c_aeg15 = (r_aeg_wren && (r_aeg_idx == 18'd15)) ? r_cae_data : (r_state == SUM) ? r_au5_sum : r_aeg15;
      c_aeg16 = (r_aeg_wren && (r_aeg_idx == 18'd16)) ? r_cae_data : (r_state == SUM) ? r_au6_sum : r_aeg16;
      c_aeg17 = (r_aeg_wren && (r_aeg_idx == 18'd17)) ? r_cae_data : (r_state == SUM) ? r_au7_sum : r_aeg17;
      c_aeg18 = (r_aeg_wren && (r_aeg_idx == 18'd18)) ? r_cae_data : (r_state == SUM) ? r_au8_sum : r_aeg18;
      c_aeg19 = (r_aeg_wren && (r_aeg_idx == 18'd19)) ? r_cae_data : (r_state == SUM) ? r_au9_sum : r_aeg19;
      c_aeg20 = (r_aeg_wren && (r_aeg_idx == 18'd20)) ? r_cae_data : (r_state == SUM) ? r_au10_sum : r_aeg20;
      c_aeg21 = (r_aeg_wren && (r_aeg_idx == 18'd21)) ? r_cae_data : (r_state == SUM) ? r_au11_sum : r_aeg21;
      c_aeg22 = (r_aeg_wren && (r_aeg_idx == 18'd22)) ? r_cae_data : (r_state == SUM) ? r_au12_sum : r_aeg22;
      c_aeg23 = (r_aeg_wren && (r_aeg_idx == 18'd23)) ? r_cae_data : (r_state == SUM) ? r_au13_sum : r_aeg23;
      c_aeg24 = (r_aeg_wren && (r_aeg_idx == 18'd24)) ? r_cae_data : (r_state == SUM) ? r_au14_sum : r_aeg24;
      c_aeg25 = (r_aeg_wren && (r_aeg_idx == 18'd25)) ? r_cae_data : (r_state == SUM) ? r_au15_sum : r_aeg25;

      c_aeg30 = (r_aeg_wren && (r_aeg_idx == 18'd30)) ? r_cae_data : (r_state == TOT) ? r_sum[63:0] : r_aeg30;
      c_aeg31 = (r_aeg_wren && (r_aeg_idx == 18'd31)) ? r_cae_data : (r_state == TOT) ? r_sum[63:0] : r_aeg31;
      c_aeg32 = (r_aeg_wren && (r_aeg_idx == 18'd32)) ? r_cae_data : (r_state == TOT) ? r_sum[63:0] : r_aeg32;
      c_aeg33 = (r_aeg_wren && (r_aeg_idx == 18'd33)) ? r_cae_data : (r_state == TOT) ? r_sum[63:0] : r_aeg33;

      c_aeg40 = (r_aeg_wren && (r_aeg_idx == 18'd40)) ? r_cae_data : (r_state == TOT) ? r_sum[63:0] : r_aeg40;
  end

   always @*
    begin
      mem_base1 = r_aeg0[47:0];
      mem_base2 = r_aeg1[47:0];
      mem_base3 = r_aeg2[47:0];
      mem_last_offst = {r_aeg3[CNT_BITS-1:0], 3'b0};
      c_count_max_ovrflw = r_aeg3[CNT_BITS];

      unaligned_addr = r_caep00 && (|mem_base1[8:0] || |mem_base2[8:0] || |mem_base3[8:0]);
      c_caep00 = caep_inst_vld && (cae_inst[22:18] == 5'd0) && ~unaligned_addr && csr_31_31_intlv_dis;
      c_sum_ovrflw = |r_sum_ovrflw_vec;
      c_res_ovrflw = |r_res_ovrflw_vec;
      c_invalid_intlv = caep_inst_vld && ~csr_31_31_intlv_dis;
    end

   assign cae_aeg_cnt = 18'd40;
   assign cae_exception = {10'h0, 
                           r_invalid_intlv, 
                           r_sum_ovrflw, 
                           r_res_ovrflw, 
                           unaligned_addr, 
                           invalid_aeg_idx, 
                           unimplemented_inst};
   assign cae_ret_data_vld = r_ret_data_vld && !cae_stall;
   assign cae_ret_data = r_ret_data;
   assign cae_idle = (r_state == IDLE) && ~r_caep00;

// leda B_3208 off Unequal length LHS and RHS in assignment LHS
   always @*
    begin
      c_state = r_state;
      c_sum_cnt = r_sum_cnt;
      c_sum_mux = 64'b0;
      c_sum = r_sum + r_sum_mux;
      sum_ovrflw = r_sum[64];
      case (r_state)
        IDLE:
          if (r_caep00) begin
            c_sum = 65'd0;
            c_state = VADD;
          end
        VADD:
          if (&r_sum_vld_vec)
            c_state = SUM;
        SUM: begin
          c_sum_cnt = {r_sum_cnt + 4'h1};
          case (r_sum_cnt)
            4'd0:  c_sum_mux = r_au0_sum;
            4'd1:  c_sum_mux = r_au1_sum;
            4'd2:  c_sum_mux = r_au2_sum;
            4'd3:  c_sum_mux = r_au3_sum;
            4'd4:  c_sum_mux = r_au4_sum;
            4'd5:  c_sum_mux = r_au5_sum;
            4'd6:  c_sum_mux = r_au6_sum;
            4'd7:  c_sum_mux = r_au7_sum;
            4'd8:  c_sum_mux = r_au8_sum;
            4'd9:  c_sum_mux = r_au9_sum;
            4'd10:  c_sum_mux = r_au10_sum;
            4'd11:  c_sum_mux = r_au11_sum;
            4'd12:  c_sum_mux = r_au12_sum;
            4'd13:  c_sum_mux = r_au13_sum;
            4'd14:  c_sum_mux = r_au14_sum;
            4'd15:  c_sum_mux = r_au15_sum;
            default: c_sum_mux = r_au0_sum;
          endcase
          if (r_sum_cnt == 4'd15)
            c_state = WAIT;
        end
        WAIT: begin
            c_state = TOT;
        end
        TOT: begin
            c_state = IDLE;
        end
        default:
          c_state = IDLE;
      endcase
   end
// leda B_3208 on Unequal length LHS and RHS in assignment LHS


   /* ----------      external module calls   ---------- */

   defparam add_pair0.MC_MASK = {3'b000};
   vadd_pair add_pair0
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc0_req_ld_e),		 // Templated
      .mc_req_st_e			(mc0_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc0_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc0_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc0_rsp_stall_e),	 // Templated
      .sum_e				(ap0_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap0_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap0_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap0_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc0_req_ld_o),		 // Templated
      .mc_req_st_o			(mc0_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc0_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc0_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc0_rsp_stall_o),	 // Templated
      .sum_o				(ap0_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap0_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap0_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap0_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc0_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc0_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc0_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc0_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc0_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc0_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc0_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc0_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc0_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc0_rsp_push_o));	 // Templated

   defparam add_pair1.MC_MASK = {3'b001};
   vadd_pair add_pair1
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc1_req_ld_e),		 // Templated
      .mc_req_st_e			(mc1_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc1_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc1_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc1_rsp_stall_e),	 // Templated
      .sum_e				(ap1_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap1_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap1_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap1_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc1_req_ld_o),		 // Templated
      .mc_req_st_o			(mc1_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc1_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc1_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc1_rsp_stall_o),	 // Templated
      .sum_o				(ap1_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap1_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap1_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap1_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc1_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc1_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc1_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc1_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc1_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc1_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc1_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc1_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc1_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc1_rsp_push_o));	 // Templated

   defparam add_pair2.MC_MASK = {3'b010};
   vadd_pair add_pair2
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc2_req_ld_e),		 // Templated
      .mc_req_st_e			(mc2_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc2_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc2_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc2_rsp_stall_e),	 // Templated
      .sum_e				(ap2_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap2_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap2_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap2_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc2_req_ld_o),		 // Templated
      .mc_req_st_o			(mc2_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc2_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc2_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc2_rsp_stall_o),	 // Templated
      .sum_o				(ap2_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap2_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap2_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap2_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc2_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc2_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc2_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc2_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc2_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc2_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc2_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc2_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc2_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc2_rsp_push_o));	 // Templated

   defparam add_pair3.MC_MASK = {3'b011};
   vadd_pair add_pair3
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc3_req_ld_e),		 // Templated
      .mc_req_st_e			(mc3_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc3_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc3_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc3_rsp_stall_e),	 // Templated
      .sum_e				(ap3_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap3_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap3_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap3_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc3_req_ld_o),		 // Templated
      .mc_req_st_o			(mc3_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc3_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc3_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc3_rsp_stall_o),	 // Templated
      .sum_o				(ap3_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap3_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap3_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap3_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc3_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc3_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc3_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc3_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc3_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc3_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc3_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc3_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc3_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc3_rsp_push_o));	 // Templated

   defparam add_pair4.MC_MASK = {3'b100};
   vadd_pair add_pair4
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc4_req_ld_e),		 // Templated
      .mc_req_st_e			(mc4_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc4_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc4_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc4_rsp_stall_e),	 // Templated
      .sum_e				(ap4_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap4_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap4_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap4_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc4_req_ld_o),		 // Templated
      .mc_req_st_o			(mc4_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc4_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc4_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc4_rsp_stall_o),	 // Templated
      .sum_o				(ap4_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap4_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap4_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap4_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc4_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc4_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc4_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc4_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc4_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc4_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc4_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc4_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc4_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc4_rsp_push_o));	 // Templated

   defparam add_pair5.MC_MASK = {3'b101};
   vadd_pair add_pair5
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc5_req_ld_e),		 // Templated
      .mc_req_st_e			(mc5_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc5_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc5_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc5_rsp_stall_e),	 // Templated
      .sum_e				(ap5_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap5_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap5_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap5_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc5_req_ld_o),		 // Templated
      .mc_req_st_o			(mc5_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc5_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc5_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc5_rsp_stall_o),	 // Templated
      .sum_o				(ap5_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap5_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap5_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap5_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc5_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc5_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc5_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc5_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc5_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc5_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc5_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc5_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc5_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc5_rsp_push_o));	 // Templated

   defparam add_pair6.MC_MASK = {3'b110};
   vadd_pair add_pair6
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc6_req_ld_e),		 // Templated
      .mc_req_st_e			(mc6_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc6_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc6_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc6_rsp_stall_e),	 // Templated
      .sum_e				(ap6_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap6_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap6_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap6_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc6_req_ld_o),		 // Templated
      .mc_req_st_o			(mc6_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc6_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc6_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc6_rsp_stall_o),	 // Templated
      .sum_o				(ap6_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap6_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap6_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap6_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc6_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc6_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc6_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc6_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc6_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc6_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc6_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc6_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc6_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc6_rsp_push_o));	 // Templated

   defparam add_pair7.MC_MASK = {3'b111};
   vadd_pair add_pair7
     ( /*AUTOINST*/
      // Outputs
      .mc_req_ld_e			(mc7_req_ld_e),		 // Templated
      .mc_req_st_e			(mc7_req_st_e),		 // Templated
      .mc_req_wrd_rdctl_e		(mc7_req_wrd_rdctl_e),	 // Templated
      .mc_req_vadr_e			(mc7_req_vadr_e),	 // Templated
      .mc_rsp_stall_e			(mc7_rsp_stall_e),	 // Templated
      .sum_e				(ap7_sum_e[63:0]),	 // Templated
      .sum_vld_e			(ap7_sum_vld_e),	 // Templated
      .sum_ovrflw_e			(ap7_sum_ovrflw_e),	 // Templated
      .res_ovrflw_e			(ap7_res_ovrflw_e),	 // Templated
      .mc_req_ld_o			(mc7_req_ld_o),		 // Templated
      .mc_req_st_o			(mc7_req_st_o),		 // Templated
      .mc_req_wrd_rdctl_o		(mc7_req_wrd_rdctl_o),	 // Templated
      .mc_req_vadr_o			(mc7_req_vadr_o),	 // Templated
      .mc_rsp_stall_o			(mc7_rsp_stall_o),	 // Templated
      .sum_o				(ap7_sum_o[63:0]),	 // Templated
      .sum_vld_o			(ap7_sum_vld_o),	 // Templated
      .sum_ovrflw_o			(ap7_sum_ovrflw_o),	 // Templated
      .res_ovrflw_o			(ap7_res_ovrflw_o),	 // Templated
      // Inputs
      .clk				(clk_per),		 // Templated
      .reset				(reset),		 // Templated
      .idle				(r_idle),		 // Templated
      .start				(r_caep00),		 // Templated
      .mem_base1			(mem_base1[47:0]),
      .mem_base2			(mem_base2[47:0]),
      .mem_base3			(mem_base3[47:0]),
      .mem_last_offst			(mem_last_offst[47:0]),
      .mc_rd_rq_stall_e			(mc7_rd_rq_stall_e),	 // Templated
      .mc_wr_rq_stall_e			(mc7_wr_rq_stall_e),	 // Templated
      .mc_rsp_rdctl_e			(mc7_rsp_rdctl_e[31:0]), // Templated
      .mc_rsp_data_e			(mc7_rsp_data_e[63:0]),	 // Templated
      .mc_rsp_push_e			(mc7_rsp_push_e),	 // Templated
      .mc_rd_rq_stall_o			(mc7_rd_rq_stall_o),	 // Templated
      .mc_wr_rq_stall_o			(mc7_wr_rq_stall_o),	 // Templated
      .mc_rsp_rdctl_o			(mc7_rsp_rdctl_o[31:0]), // Templated
      .mc_rsp_data_o			(mc7_rsp_data_o[63:0]),	 // Templated
      .mc_rsp_push_o			(mc7_rsp_push_o));	 // Templated

/* vadd_pair AUTO_TEMPLATE (
      //Outputs
      .mc_req_ld_e        (mc@_req_ld_e),
      .mc_req_st_e        (mc@_req_st_e),
      .mc_req_wrd_rdctl_e (mc@_req_wrd_rdctl_e),
      .mc_req_vadr_e      (mc@_req_vadr_e),
      .mc_rsp_stall_e     (mc@_rsp_stall_e),
      .mc_req_ld_o        (mc@_req_ld_o),
      .mc_req_st_o        (mc@_req_st_o),
      .mc_req_wrd_rdctl_o (mc@_req_wrd_rdctl_o),
      .mc_req_vadr_o      (mc@_req_vadr_o),
      .mc_rsp_stall_o     (mc@_rsp_stall_o),
      .sum_e              (ap@_sum_e[]),
      .sum_o              (ap@_sum_o[]),
      .sum_vld_e          (ap@_sum_vld_e),
      .sum_vld_o          (ap@_sum_vld_o),
      .sum_ovrflw_e       (ap@_sum_ovrflw_e),
      .sum_ovrflw_o       (ap@_sum_ovrflw_o),
      .res_ovrflw_e       (ap@_res_ovrflw_e),
      .res_ovrflw_o       (ap@_res_ovrflw_o),
      //Inputs
      .clk                (clk_per),
      .reset              (reset),
      .idle               (r_idle),
      .start              (r_caep00),
      .mc_rd_rq_stall_e   (mc@_rd_rq_stall_e),
      .mc_wr_rq_stall_e   (mc@_wr_rq_stall_e),
      .mc_rsp_rdctl_e     (mc@_rsp_rdctl_e[31:0]),
      .mc_rsp_data_e      (mc@_rsp_data_e[63:0]),
      .mc_rsp_push_e      (mc@_rsp_push_e),
      .mc_rd_rq_stall_o   (mc@_rd_rq_stall_o),
      .mc_wr_rq_stall_o   (mc@_wr_rq_stall_o),
      .mc_rsp_rdctl_o     (mc@_rsp_rdctl_o[31:0]),
      .mc_rsp_data_o      (mc@_rsp_data_o[63:0]),
      .mc_rsp_push_o      (mc@_rsp_push_o),
    ); */

   cae_csr cae_csr (
      // Outputs
      .ring_ctl_out       (cae_ring_ctl_out),
      .ring_data_out      (cae_ring_data_out),
      .cae_csr_scratch    (cae_csr_scratch),
      // Inputs
      .clk_csr            (clk_csr),
      .i_csr_reset_n      (i_csr_reset_n),
      .ring_ctl_in        (cae_ring_ctl_in),
      .ring_data_in       (cae_ring_data_in),
      .cae_csr_status                   ({61'b0, r_state}),
      .cae_csr_vis                      ({16'b0, cae_exception, 16'b0, r_sum_vld_vec}),
      .cae_csr_sum        (r_sum_csr)
   );

   /* ----------            registers         ---------- */

   always @(posedge clk_per) begin
      if (reset_bg)
        reset <= 1'b1;
      else
        reset <= 1'b0;
   end

   always @(posedge clk_per) begin
      r_aeg0 <= c_aeg0;
      r_aeg1 <= c_aeg1;
      r_aeg2 <= c_aeg2;
      r_aeg3 <= c_aeg3;
      r_aeg4 <= c_aeg4;
      r_aeg10 <= c_aeg10;
      r_aeg11 <= c_aeg11;
      r_aeg12 <= c_aeg12;
      r_aeg13 <= c_aeg13;
      r_aeg14 <= c_aeg14;
      r_aeg15 <= c_aeg15;
      r_aeg16 <= c_aeg16;
      r_aeg17 <= c_aeg17;
      r_aeg18 <= c_aeg18;
      r_aeg19 <= c_aeg19;
      r_aeg20 <= c_aeg20;
      r_aeg21 <= c_aeg21;
      r_aeg22 <= c_aeg22;
      r_aeg23 <= c_aeg23;
      r_aeg24 <= c_aeg24;
      r_aeg25 <= c_aeg25;
      r_aeg30 <= c_aeg30;
      r_aeg31 <= c_aeg31;
      r_aeg32 <= c_aeg32;
      r_aeg33 <= c_aeg33;
      r_aeg40 <= c_aeg40;
   end


   always @(posedge clk_per) begin
      r_ret_data     <= c_ret_data;
      r_ret_data_vld <= c_ret_data_vld;
      r_cae_data     <= cae_data;
      r_aeg_idx      <= c_aeg_idx;
      r_aeg_rden     <= c_aeg_rden;
      r_aeg_wren     <= c_aeg_wren;
      r_caep00       <= c_caep00;
      r_idle         <= cae_idle;
   end

   always @(posedge clk_per) begin
      r_state         <= reset ? 3'b0  : c_state;
      r_sum_mux       <= reset ? 64'b0 : c_sum_mux;
      r_sum           <= reset ? 65'b0 : c_sum;
      r_sum_ovrflw    <= reset ? 1'b0  : c_sum_ovrflw;
      r_res_ovrflw    <= reset ? 1'b0  : c_res_ovrflw;
      r_sum_cnt       <= reset ? 4'b0  : c_sum_cnt;
      r_invalid_intlv <= reset ? 1'b0  : c_invalid_intlv;
    end

   always @(posedge clk_per) begin
      r_au0_sum <= ap0_sum_e;
      r_au1_sum <= ap0_sum_o;
      r_au2_sum <= ap1_sum_e;
      r_au3_sum <= ap1_sum_o;
      r_au4_sum <= ap2_sum_e;
      r_au5_sum <= ap2_sum_o;
      r_au6_sum <= ap3_sum_e;
      r_au7_sum <= ap3_sum_o;
      r_au8_sum <= ap4_sum_e;
      r_au9_sum <= ap4_sum_o;
      r_au10_sum <= ap5_sum_e;
      r_au11_sum <= ap5_sum_o;
      r_au12_sum <= ap6_sum_e;
      r_au13_sum <= ap6_sum_o;
      r_au14_sum <= ap7_sum_e;
      r_au15_sum <= ap7_sum_o;
   end

   always @(posedge clk_per) begin
      r_sum_vld_vec[0] <= ap0_sum_vld_e;
      r_sum_vld_vec[1] <= ap0_sum_vld_o;
      r_sum_vld_vec[2] <= ap1_sum_vld_e;
      r_sum_vld_vec[3] <= ap1_sum_vld_o;
      r_sum_vld_vec[4] <= ap2_sum_vld_e;
      r_sum_vld_vec[5] <= ap2_sum_vld_o;
      r_sum_vld_vec[6] <= ap3_sum_vld_e;
      r_sum_vld_vec[7] <= ap3_sum_vld_o;
      r_sum_vld_vec[8] <= ap4_sum_vld_e;
      r_sum_vld_vec[9] <= ap4_sum_vld_o;
      r_sum_vld_vec[10] <= ap5_sum_vld_e;
      r_sum_vld_vec[11] <= ap5_sum_vld_o;
      r_sum_vld_vec[12] <= ap6_sum_vld_e;
      r_sum_vld_vec[13] <= ap6_sum_vld_o;
      r_sum_vld_vec[14] <= ap7_sum_vld_e;
      r_sum_vld_vec[15] <= ap7_sum_vld_o;
   end

   always @(posedge clk_per) begin
      r_res_ovrflw_vec[0] <= ap0_res_ovrflw_e;
      r_res_ovrflw_vec[1] <= ap0_res_ovrflw_o;
      r_res_ovrflw_vec[2] <= ap1_res_ovrflw_e;
      r_res_ovrflw_vec[3] <= ap1_res_ovrflw_o;
      r_res_ovrflw_vec[4] <= ap2_res_ovrflw_e;
      r_res_ovrflw_vec[5] <= ap2_res_ovrflw_o;
      r_res_ovrflw_vec[6] <= ap3_res_ovrflw_e;
      r_res_ovrflw_vec[7] <= ap3_res_ovrflw_o;
      r_res_ovrflw_vec[8] <= ap4_res_ovrflw_e;
      r_res_ovrflw_vec[9] <= ap4_res_ovrflw_o;
      r_res_ovrflw_vec[10] <= ap5_res_ovrflw_e;
      r_res_ovrflw_vec[11] <= ap5_res_ovrflw_o;
      r_res_ovrflw_vec[12] <= ap6_res_ovrflw_e;
      r_res_ovrflw_vec[13] <= ap6_res_ovrflw_o;
      r_res_ovrflw_vec[14] <= ap7_res_ovrflw_e;
      r_res_ovrflw_vec[15] <= ap7_res_ovrflw_o;
   end

   always @(posedge clk_per) begin
      r_sum_ovrflw_vec[0] <= ap0_sum_ovrflw_e;
      r_sum_ovrflw_vec[1] <= ap0_sum_ovrflw_o;
      r_sum_ovrflw_vec[2] <= ap1_sum_ovrflw_e;
      r_sum_ovrflw_vec[3] <= ap1_sum_ovrflw_o;
      r_sum_ovrflw_vec[4] <= ap2_sum_ovrflw_e;
      r_sum_ovrflw_vec[5] <= ap2_sum_ovrflw_o;
      r_sum_ovrflw_vec[6] <= ap3_sum_ovrflw_e;
      r_sum_ovrflw_vec[7] <= ap3_sum_ovrflw_o;
      r_sum_ovrflw_vec[8] <= ap4_sum_ovrflw_e;
      r_sum_ovrflw_vec[9] <= ap4_sum_ovrflw_o;
      r_sum_ovrflw_vec[10] <= ap5_sum_ovrflw_e;
      r_sum_ovrflw_vec[11] <= ap5_sum_ovrflw_o;
      r_sum_ovrflw_vec[12] <= ap6_sum_ovrflw_e;
      r_sum_ovrflw_vec[13] <= ap6_sum_ovrflw_o;
      r_sum_ovrflw_vec[14] <= ap7_sum_ovrflw_e;
      r_sum_ovrflw_vec[15] <= ap7_sum_ovrflw_o;
   end

   always @(posedge clk_csr) begin
      r_sum_csr <= reset ? 64'd0 : r_sum[63:0];
   end

   /* ---------- debug & synopsys off blocks  ---------- */
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // cae_pers

