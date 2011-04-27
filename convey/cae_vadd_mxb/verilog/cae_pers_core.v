/*****************************************************************************/
//
// Module	   : cae_pers_core.vpp
// Revision	   : $Revision: 1.17 $
// Last Modified On: $Date: 2011/04/22 16:42:16 $
// Last Modified By: $Author: mruff $
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
// Copyright (c) 2007-2011 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: cae_pers.vpp,v 1.17 2011/04/22 16:42:16 mruff Exp $ */

`timescale 1 ns / 1 ps

//`include "pdk_fpga_defines.vh"

(* keep_hierarchy = "true" *)
module cae_pers_core (
input		clk_csr,
input		clk,
input		clk2x,
input		i_reset,
input		i_csr_reset_n,
input  [1:0]	i_aeid,

input		ppll_reset,
output		ppll_locked,
output		clk_per,

//
// Dispatch Interface
//
input  [31:0]	cae_inst,
input  [63:0]	cae_data,
input		cae_inst_vld,

output [17:0]	cae_aeg_cnt,
output [15:0]	cae_exception,
output [63:0]	cae_ret_data,
output		cae_ret_data_vld,
output		cae_idle,
output		cae_stall,

//
// Management/Debug Interface
//
input  [3:0]	cae_ring_ctl_in,
input  [15:0]	cae_ring_data_in,
output [3:0]	cae_ring_ctl_out,
output [15:0]	cae_ring_data_out,

input		csr_31_31_intlv_dis,

//
// MC Interface(s)
//
output		mc0_req_ld_e, mc0_req_ld_o,
output		mc0_req_st_e, mc0_req_st_o,
output [1:0]	mc0_req_size_e, mc0_req_size_o,
output [47:0]	mc0_req_vadr_e, mc0_req_vadr_o,
output [63:0]	mc0_req_wrd_rdctl_e, mc0_req_wrd_rdctl_o,
output		mc0_rsp_stall_e, mc0_rsp_stall_o,
input		mc0_rd_rq_stall_e, mc0_wr_rq_stall_e,
input		mc0_rd_rq_stall_o, mc0_wr_rq_stall_o,
input  [63:0]	mc0_rsp_data_e, mc0_rsp_data_o,
input		mc0_rsp_push_e, mc0_rsp_push_o,
input  [31:0]	mc0_rsp_rdctl_e, mc0_rsp_rdctl_o,

// Write flush
output		mc0_req_flush_e, mc0_req_flush_o,
input		mc0_rsp_flush_cmplt_e, mc0_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc0_wrctl_e, mc0_wrctl_o,
output		mc0_wr_rsp_stall_e, mc0_wr_rsp_stall_o,
input		mc0_rsp_wrcmp_e, mc0_rsp_wrcmp_o,
input  [14:0]	mc0_rsp_wrctl_e, mc0_rsp_wrctl_o,
input		mc0_wtid_err,
`endif

output		mc1_req_ld_e, mc1_req_ld_o,
output		mc1_req_st_e, mc1_req_st_o,
output [1:0]	mc1_req_size_e, mc1_req_size_o,
output [47:0]	mc1_req_vadr_e, mc1_req_vadr_o,
output [63:0]	mc1_req_wrd_rdctl_e, mc1_req_wrd_rdctl_o,
output		mc1_rsp_stall_e, mc1_rsp_stall_o,
input		mc1_rd_rq_stall_e, mc1_wr_rq_stall_e,
input		mc1_rd_rq_stall_o, mc1_wr_rq_stall_o,
input  [63:0]	mc1_rsp_data_e, mc1_rsp_data_o,
input		mc1_rsp_push_e, mc1_rsp_push_o,
input  [31:0]	mc1_rsp_rdctl_e, mc1_rsp_rdctl_o,

// Write flush
output		mc1_req_flush_e, mc1_req_flush_o,
input		mc1_rsp_flush_cmplt_e, mc1_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc1_wrctl_e, mc1_wrctl_o,
output		mc1_wr_rsp_stall_e, mc1_wr_rsp_stall_o,
input		mc1_rsp_wrcmp_e, mc1_rsp_wrcmp_o,
input  [14:0]	mc1_rsp_wrctl_e, mc1_rsp_wrctl_o,
input		mc1_wtid_err,
`endif

output		mc2_req_ld_e, mc2_req_ld_o,
output		mc2_req_st_e, mc2_req_st_o,
output [1:0]	mc2_req_size_e, mc2_req_size_o,
output [47:0]	mc2_req_vadr_e, mc2_req_vadr_o,
output [63:0]	mc2_req_wrd_rdctl_e, mc2_req_wrd_rdctl_o,
output		mc2_rsp_stall_e, mc2_rsp_stall_o,
input		mc2_rd_rq_stall_e, mc2_wr_rq_stall_e,
input		mc2_rd_rq_stall_o, mc2_wr_rq_stall_o,
input  [63:0]	mc2_rsp_data_e, mc2_rsp_data_o,
input		mc2_rsp_push_e, mc2_rsp_push_o,
input  [31:0]	mc2_rsp_rdctl_e, mc2_rsp_rdctl_o,

// Write flush
output		mc2_req_flush_e, mc2_req_flush_o,
input		mc2_rsp_flush_cmplt_e, mc2_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc2_wrctl_e, mc2_wrctl_o,
output		mc2_wr_rsp_stall_e, mc2_wr_rsp_stall_o,
input		mc2_rsp_wrcmp_e, mc2_rsp_wrcmp_o,
input  [14:0]	mc2_rsp_wrctl_e, mc2_rsp_wrctl_o,
input		mc2_wtid_err,
`endif

output		mc3_req_ld_e, mc3_req_ld_o,
output		mc3_req_st_e, mc3_req_st_o,
output [1:0]	mc3_req_size_e, mc3_req_size_o,
output [47:0]	mc3_req_vadr_e, mc3_req_vadr_o,
output [63:0]	mc3_req_wrd_rdctl_e, mc3_req_wrd_rdctl_o,
output		mc3_rsp_stall_e, mc3_rsp_stall_o,
input		mc3_rd_rq_stall_e, mc3_wr_rq_stall_e,
input		mc3_rd_rq_stall_o, mc3_wr_rq_stall_o,
input  [63:0]	mc3_rsp_data_e, mc3_rsp_data_o,
input		mc3_rsp_push_e, mc3_rsp_push_o,
input  [31:0]	mc3_rsp_rdctl_e, mc3_rsp_rdctl_o,

// Write flush
output		mc3_req_flush_e, mc3_req_flush_o,
input		mc3_rsp_flush_cmplt_e, mc3_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc3_wrctl_e, mc3_wrctl_o,
output		mc3_wr_rsp_stall_e, mc3_wr_rsp_stall_o,
input		mc3_rsp_wrcmp_e, mc3_rsp_wrcmp_o,
input  [14:0]	mc3_rsp_wrctl_e, mc3_rsp_wrctl_o,
input		mc3_wtid_err,
`endif

output		mc4_req_ld_e, mc4_req_ld_o,
output		mc4_req_st_e, mc4_req_st_o,
output [1:0]	mc4_req_size_e, mc4_req_size_o,
output [47:0]	mc4_req_vadr_e, mc4_req_vadr_o,
output [63:0]	mc4_req_wrd_rdctl_e, mc4_req_wrd_rdctl_o,
output		mc4_rsp_stall_e, mc4_rsp_stall_o,
input		mc4_rd_rq_stall_e, mc4_wr_rq_stall_e,
input		mc4_rd_rq_stall_o, mc4_wr_rq_stall_o,
input  [63:0]	mc4_rsp_data_e, mc4_rsp_data_o,
input		mc4_rsp_push_e, mc4_rsp_push_o,
input  [31:0]	mc4_rsp_rdctl_e, mc4_rsp_rdctl_o,

// Write flush
output		mc4_req_flush_e, mc4_req_flush_o,
input		mc4_rsp_flush_cmplt_e, mc4_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc4_wrctl_e, mc4_wrctl_o,
output		mc4_wr_rsp_stall_e, mc4_wr_rsp_stall_o,
input		mc4_rsp_wrcmp_e, mc4_rsp_wrcmp_o,
input  [14:0]	mc4_rsp_wrctl_e, mc4_rsp_wrctl_o,
input		mc4_wtid_err,
`endif

output		mc5_req_ld_e, mc5_req_ld_o,
output		mc5_req_st_e, mc5_req_st_o,
output [1:0]	mc5_req_size_e, mc5_req_size_o,
output [47:0]	mc5_req_vadr_e, mc5_req_vadr_o,
output [63:0]	mc5_req_wrd_rdctl_e, mc5_req_wrd_rdctl_o,
output		mc5_rsp_stall_e, mc5_rsp_stall_o,
input		mc5_rd_rq_stall_e, mc5_wr_rq_stall_e,
input		mc5_rd_rq_stall_o, mc5_wr_rq_stall_o,
input  [63:0]	mc5_rsp_data_e, mc5_rsp_data_o,
input		mc5_rsp_push_e, mc5_rsp_push_o,
input  [31:0]	mc5_rsp_rdctl_e, mc5_rsp_rdctl_o,

// Write flush
output		mc5_req_flush_e, mc5_req_flush_o,
input		mc5_rsp_flush_cmplt_e, mc5_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc5_wrctl_e, mc5_wrctl_o,
output		mc5_wr_rsp_stall_e, mc5_wr_rsp_stall_o,
input		mc5_rsp_wrcmp_e, mc5_rsp_wrcmp_o,
input  [14:0]	mc5_rsp_wrctl_e, mc5_rsp_wrctl_o,
input		mc5_wtid_err,
`endif

output		mc6_req_ld_e, mc6_req_ld_o,
output		mc6_req_st_e, mc6_req_st_o,
output [1:0]	mc6_req_size_e, mc6_req_size_o,
output [47:0]	mc6_req_vadr_e, mc6_req_vadr_o,
output [63:0]	mc6_req_wrd_rdctl_e, mc6_req_wrd_rdctl_o,
output		mc6_rsp_stall_e, mc6_rsp_stall_o,
input		mc6_rd_rq_stall_e, mc6_wr_rq_stall_e,
input		mc6_rd_rq_stall_o, mc6_wr_rq_stall_o,
input  [63:0]	mc6_rsp_data_e, mc6_rsp_data_o,
input		mc6_rsp_push_e, mc6_rsp_push_o,
input  [31:0]	mc6_rsp_rdctl_e, mc6_rsp_rdctl_o,

// Write flush
output		mc6_req_flush_e, mc6_req_flush_o,
input		mc6_rsp_flush_cmplt_e, mc6_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc6_wrctl_e, mc6_wrctl_o,
output		mc6_wr_rsp_stall_e, mc6_wr_rsp_stall_o,
input		mc6_rsp_wrcmp_e, mc6_rsp_wrcmp_o,
input  [14:0]	mc6_rsp_wrctl_e, mc6_rsp_wrctl_o,
input		mc6_wtid_err,
`endif

output		mc7_req_ld_e, mc7_req_ld_o,
output		mc7_req_st_e, mc7_req_st_o,
output [1:0]	mc7_req_size_e, mc7_req_size_o,
output [47:0]	mc7_req_vadr_e, mc7_req_vadr_o,
output [63:0]	mc7_req_wrd_rdctl_e, mc7_req_wrd_rdctl_o,
output		mc7_rsp_stall_e, mc7_rsp_stall_o,
input		mc7_rd_rq_stall_e, mc7_wr_rq_stall_e,
input		mc7_rd_rq_stall_o, mc7_wr_rq_stall_o,
input  [63:0]	mc7_rsp_data_e, mc7_rsp_data_o,
input		mc7_rsp_push_e, mc7_rsp_push_o,
input  [31:0]	mc7_rsp_rdctl_e, mc7_rsp_rdctl_o,

// Write flush
output		mc7_req_flush_e, mc7_req_flush_o,
input		mc7_rsp_flush_cmplt_e, mc7_rsp_flush_cmplt_o,

`ifdef USE_WR_CMP_IF
output [14:0]	mc7_wrctl_e, mc7_wrctl_o,
output		mc7_wr_rsp_stall_e, mc7_wr_rsp_stall_o,
input		mc7_rsp_wrcmp_e, mc7_rsp_wrcmp_o,
input  [14:0]	mc7_rsp_wrctl_e, mc7_rsp_wrctl_o,
input		mc7_wtid_err,
`endif

//
// AE-to-AE Interface
//
output [63:0]	tx_data,
output [23:0]	tx_cmd,
output [1:0]	tx_dest,
output		tx_valid,
input		tx_stall,

input  [63:0]	rx_data,
input  [23:0]	rx_cmd,
input  [1:0]	rx_src,
input		rx_valid,
input  [63:0]	r_xb_alarm
);

`include "pdk_fpga_param.vh"

   /* ----------    local clock generation ---------- */
   (* KEEP = "true" *) wire reset_per;
   generate if (RATIO == 0) begin : gclks
      assign clk_per = clk;
      assign ppll_locked = 1'b1;
      assign reset_per = i_reset;
   end else begin
      //leda B_1011 off - Module instantiation is not fully bound
      BUFG pll_bufg     (.O(pll_clkfbin), .I(pll_clkfbout));
      BUFG clk_per_bufg (.O(clk_per), .I(pll_clkout));
      PLL_BASE #(
	.CLKIN_PERIOD  (6.666),	// 150 MHz
	.DIVCLK_DIVIDE (1),	// 150 MHz (Fpfd)
	.CLKFBOUT_MULT (5),	// 750 MHz (Fvco)
	.CLKOUT0_DIVIDE(6)	// 125 MHz
      ) pll (
	.CLKIN(clk),
	.RST(ppll_reset),
	.CLKFBOUT(pll_clkfbout),
	.CLKFBIN(pll_clkfbin),
	.LOCKED(ppll_locked),
	.CLKOUT0(pll_clkout),
	.CLKOUT1(), .CLKOUT2(), .CLKOUT3(), .CLKOUT4(), .CLKOUT5()
      );
      //leda B_1011 off - Module instantiation is not fully bound

      (* KEEP = "true" *)(* TIG = "true" *) wire r_AsYnC_reset;
      FDSE rst0 (.C(clk), .S(i_reset),
		 .CE(r_AsYnC_reset), .D(!r_AsYnC_reset), .Q(r_AsYnC_reset));
      FD   rst1 (.C(clk_per), .D(r_AsYnC_reset), .Q(r_reset));
      BUFG brst (.O(reset_per), .I(r_reset));
   end endgenerate

   /* ----------          wires & regs        ---------- */

   reg  [63:0]  r_cae_data;
   reg          c_count_max_ovrflw;
   reg  [47:0]  base_address;
   reg  [8:0]   num_threads;
   reg  [31:0]  edge_count;
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
   reg  [63:0]  c_aeg0, c_aeg1, c_aeg2;
   reg  [63:0]  r_aeg0, r_aeg1, r_aeg2;

   reg          c_caep00;
   reg          r_caep00;
   reg  [63:0]  c_ret_data;
   reg  [63:0]  r_ret_data;
   reg          c_ret_data_vld;
   reg          r_ret_data_vld;

   reg  [2:0]   c_state;
   reg  [2:0]   r_state;

   reg          r_idle;
   wire [63:0]  cae_csr_scratch;

   reg          reset;
   localparam IDLE = 3'd0;
   localparam WAIT = 3'd1;

   wire 	vadd_idle;
   

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
	    caep_inst_vld = cae_inst_vld & cae_inst[23];	// CAEP instructions in range 20-3F
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
	  default: c_ret_data = 64'b0;
	endcase
    end

   always @*
    begin
      invalid_aeg_idx = cae_inst_vld && (|c_aeg_idx[17:8]);

      c_aeg0 = (r_aeg_wren && (r_aeg_idx == 18'd0)) ? r_cae_data : r_aeg0;
      c_aeg1 = (r_aeg_wren && (r_aeg_idx == 18'd1)) ? r_cae_data : r_aeg1;
      c_aeg2 = (r_aeg_wren && (r_aeg_idx == 18'd2)) ? r_cae_data : r_aeg2;
  end

   always @*
    begin
      base_address = r_aeg0[47:0];
      num_threads = r_aeg1[8:0];
      edge_count = r_aeg2[31:0];

      c_caep00 = caep_inst_vld && (cae_inst[22:18] == 5'd0) && csr_31_31_intlv_dis;
      c_invalid_intlv = caep_inst_vld && ~csr_31_31_intlv_dis;
    end

   assign cae_aeg_cnt = 18'd40;
   assign cae_exception = {14'h0,
			   invalid_aeg_idx,
			   unimplemented_inst};
   assign cae_ret_data_vld = r_ret_data_vld && !cae_stall;
   assign cae_ret_data = r_ret_data;
   assign cae_idle = (r_state == IDLE) && ~r_caep00;
// && ~vadd_idle;

// leda B_3208 off Unequal length LHS and RHS in assignment LHS
   always @*
    begin
      c_state = r_state;
      case (r_state)
	IDLE:
	  if (r_caep00) begin
	    c_state = WAIT;
	  end
	WAIT: begin
	   if (vadd_idle)
	     c_state = IDLE;
	end
	default:
	  c_state = IDLE;
      endcase
   end
// leda B_3208 on Unequal length LHS and RHS in assignment LHS


   /* ----------      external module calls   ---------- */

   assign mc1_req_ld_e = 1'b0;
   assign mc1_req_st_e = 1'b0;
   assign mc2_req_ld_e = 1'b0;
   assign mc2_req_st_e = 1'b0;
   assign mc3_req_ld_e = 1'b0;
   assign mc3_req_st_e = 1'b0;
   assign mc4_req_ld_e = 1'b0;
   assign mc4_req_st_e = 1'b0;
   assign mc5_req_ld_e = 1'b0;
   assign mc5_req_st_e = 1'b0;
   assign mc6_req_ld_e = 1'b0;
   assign mc6_req_st_e = 1'b0;
   assign mc7_req_ld_e = 1'b0;
   assign mc7_req_st_e = 1'b0;
   assign mc0_req_ld_o = 1'b0;
   assign mc0_req_st_o = 1'b0;
   assign mc1_req_ld_o = 1'b0;
   assign mc1_req_st_o = 1'b0;
   assign mc2_req_ld_o = 1'b0;
   assign mc2_req_st_o = 1'b0;
   assign mc3_req_ld_o = 1'b0;
   assign mc3_req_st_o = 1'b0;
   assign mc4_req_ld_o = 1'b0;
   assign mc4_req_st_o = 1'b0;
   assign mc5_req_ld_o = 1'b0;
   assign mc5_req_st_o = 1'b0;
   assign mc6_req_ld_o = 1'b0;
   assign mc6_req_st_o = 1'b0;
   assign mc7_req_ld_o = 1'b0;
   assign mc7_req_st_o = 1'b0;

   vadd add0 (
      //Outputs
      .mc_req_ld        (mc0_req_ld_e),
      .mc_req_st        (mc0_req_st_e),
      .mc_req_wrd_rdctl (mc0_req_wrd_rdctl_e),
      .mc_req_vadr      (mc0_req_vadr_e),
      .mc_rsp_stall     (mc0_rsp_stall_e),
      .idle             (vadd_idle),
      //Inputs
      .clk              (clk_per),
      .reset            (reset),
      .start            (r_caep00),
      .mc_rd_rq_stall   (mc0_rd_rq_stall_e),
      .mc_wr_rq_stall   (mc0_wr_rq_stall_e),
      .mc_rsp_rdctl     (mc0_rsp_rdctl_e[31:0]),
      .mc_rsp_data      (mc0_rsp_data_e[63:0]),
      .mc_rsp_push      (mc0_rsp_push_e),
      .edge_count	(edge_count[31:0]),
      .num_threads	(num_threads[8:0]),
      .base_address	(base_address[47:0])
    );

/* vadd AUTO_TEMPLATE (
      //Outputs
      .mc_req_ld        (mc@_req_ld_e),
      .mc_req_st        (mc@_req_st_e),
      .mc_req_wrd_rdctl (mc@_req_wrd_rdctl_e),
      .mc_req_vadr      (mc@_req_vadr_e),
      .mc_rsp_stall     (mc@_rsp_stall_e),
      //Inputs
      .clk                (clk_per),
      .reset              (reset),
      .idle               (r_idle),
      .start              (r_caep00),
      .mc_rd_rq_stall     (mc@_rd_rq_stall_e),
      .mc_wr_rq_stall     (mc@_wr_rq_stall_e),
      .mc_rsp_rdctl       (mc@_rsp_rdctl_e[31:0]),
      .mc_rsp_data        (mc@_rsp_data_e[63:0]),
      .mc_rsp_push        (mc@_rsp_push_e),
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
      .cae_csr_status     ({61'b0, r_state}),
      .cae_csr_vis        ({16'b0, cae_exception, 32'b0}),
      .cae_csr_sum        ()
   );

   /* ----------            registers         ---------- */

   always @(posedge clk_per) begin
      if (reset_per)
	reset <= 1'b1;
      else
	reset <= 1'b0;
   end

   always @(posedge clk_per) begin
      r_aeg0 <= c_aeg0;
      r_aeg1 <= c_aeg1;
      r_aeg2 <= c_aeg2;
   end

   always @(posedge clk_per) begin
      r_ret_data     <= c_ret_data;
      r_ret_data_vld <= c_ret_data_vld;
      r_cae_data     <= cae_data;
      r_aeg_idx      <= c_aeg_idx;
      r_aeg_rden     <= c_aeg_rden;
      r_aeg_wren     <= c_aeg_wren;
      r_caep00       <= c_caep00;
   end

   always @(posedge clk_per) begin
      r_state	      <= reset ? 3'b0  : c_state;
      r_invalid_intlv <= reset ? 1'b0  : c_invalid_intlv;
    end

   /* ---------- debug & synopsys off blocks  ---------- */

   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // cae_pers_core
