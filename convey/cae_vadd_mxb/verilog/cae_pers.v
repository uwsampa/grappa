/*****************************************************************************/
//
// Module          : cae_pers.vpp
// Revision        : $Revision: 1.6 $
// Last Modified On: $Date: 2010/11/02 19:45:15 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : gedwards
// Created On      : Wed Oct 10 09:26:08 2007
//
//-----------------------------------------------------------------------------
//
// Description     : Sample Custom Personality wrapper
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: cae_pers.vpp,v 1.6 2010/11/02 19:45:15 gedwards Exp $ */

`timescale 1 ns / 1 ps

// leda FM_2_23 off Non driven output ports or signals
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

   input [63:0]   mc0_rsp_data_e,   
   input [63:0]   mc0_rsp_data_o,   
   input          mc0_rsp_push_e,   
   input          mc0_rsp_push_o,   
   input [31:0]   mc0_rsp_rdctl_e,  
   input [31:0]   mc0_rsp_rdctl_o,  
   input [63:0]   mc1_rsp_data_e,
   input [63:0]   mc1_rsp_data_o,
   input          mc1_rsp_push_e,
   input          mc1_rsp_push_o,
   input [31:0]   mc1_rsp_rdctl_e,
   input [31:0]   mc1_rsp_rdctl_o,
   input [63:0]   mc2_rsp_data_e,
   input [63:0]   mc2_rsp_data_o,
   input          mc2_rsp_push_e,
   input          mc2_rsp_push_o,
   input [31:0]   mc2_rsp_rdctl_e,
   input [31:0]   mc2_rsp_rdctl_o,
   input [63:0]   mc3_rsp_data_e,
   input [63:0]   mc3_rsp_data_o,
   input          mc3_rsp_push_e,
   input          mc3_rsp_push_o,
   input [31:0]   mc3_rsp_rdctl_e,
   input [31:0]   mc3_rsp_rdctl_o,
   input [63:0]   mc4_rsp_data_e,
   input [63:0]   mc4_rsp_data_o,
   input          mc4_rsp_push_e,
   input          mc4_rsp_push_o,
   input [31:0]   mc4_rsp_rdctl_e,
   input [31:0]   mc4_rsp_rdctl_o,
   input [63:0]   mc5_rsp_data_e,
   input [63:0]   mc5_rsp_data_o,
   input          mc5_rsp_push_e,
   input          mc5_rsp_push_o,
   input [31:0]   mc5_rsp_rdctl_e,
   input [31:0]   mc5_rsp_rdctl_o,
   input [63:0]   mc6_rsp_data_e,
   input [63:0]   mc6_rsp_data_o,
   input          mc6_rsp_push_e,
   input          mc6_rsp_push_o,
   input [31:0]   mc6_rsp_rdctl_e,
   input [31:0]   mc6_rsp_rdctl_o,
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
   input          mc7_rsp_flush_cmplt_o,

   input          mc0_rd_rq_stall_e,
   input          mc0_wr_rq_stall_e,
   input          mc0_rd_rq_stall_o,
   input          mc0_wr_rq_stall_o,
   input          mc1_rd_rq_stall_e,
   input          mc1_wr_rq_stall_e,
   input          mc1_rd_rq_stall_o,
   input          mc1_wr_rq_stall_o,
   input          mc2_rd_rq_stall_e,
   input          mc2_wr_rq_stall_e,
   input          mc2_rd_rq_stall_o,
   input          mc2_wr_rq_stall_o,
   input          mc3_rd_rq_stall_e,
   input          mc3_wr_rq_stall_e,
   input          mc3_rd_rq_stall_o,
   input          mc3_wr_rq_stall_o,
   input          mc4_rd_rq_stall_e,
   input          mc4_wr_rq_stall_e,
   input          mc4_rd_rq_stall_o,
   input          mc4_wr_rq_stall_o,
   input          mc5_rd_rq_stall_e,
   input          mc5_wr_rq_stall_e,
   input          mc5_rd_rq_stall_o,
   input          mc5_wr_rq_stall_o,
   input          mc6_rd_rq_stall_e,
   input          mc6_wr_rq_stall_e,
   input          mc6_rd_rq_stall_o,
   input          mc6_wr_rq_stall_o,
   input          mc7_rd_rq_stall_e,
   input          mc7_wr_rq_stall_e,
   input          mc7_rd_rq_stall_o,
   input          mc7_wr_rq_stall_o

   );

   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */


   // Port 0 even request
   wire          p0_mc_req_idle_e;
   wire          p0_mc_req_ld_e;
   wire          p0_mc_req_st_e;
   wire [63:0]   p0_mc_req_wrd_rdctl_e;
   wire [47:0]   p0_mc_req_vadr_e;
   wire [1:0]    p0_mc_req_size_e;
   wire          p0_mc_req_stall_e;

   // Port 0 odd request
   wire          p0_mc_req_idle_o;
   wire          p0_mc_req_ld_o;
   wire          p0_mc_req_st_o;
   wire [63:0]   p0_mc_req_wrd_rdctl_o;
   wire [47:0]   p0_mc_req_vadr_o;
   wire [1:0]    p0_mc_req_size_o;
   wire          p0_mc_req_stall_o;

   // Port 0 even response
   wire [31:0]   p0_mc_rsp_rdctl_e;
   wire [63:0]   p0_mc_rsp_data_e;
   wire          p0_mc_rsp_push_e;
   wire          p0_mc_rsp_stall_e;

   // Port 0 odd response
   wire [31:0]   p0_mc_rsp_rdctl_o;
   wire [63:0]   p0_mc_rsp_data_o;
   wire          p0_mc_rsp_push_o;
   wire          p0_mc_rsp_stall_o;

   // Port 1 even request
   wire          p1_mc_req_idle_e;
   wire          p1_mc_req_ld_e;
   wire          p1_mc_req_st_e;
   wire [63:0]   p1_mc_req_wrd_rdctl_e;
   wire [47:0]   p1_mc_req_vadr_e;
   wire [1:0]    p1_mc_req_size_e;
   wire          p1_mc_req_stall_e;

   // Port 1 odd request
   wire          p1_mc_req_idle_o;
   wire          p1_mc_req_ld_o;
   wire          p1_mc_req_st_o;
   wire [63:0]   p1_mc_req_wrd_rdctl_o;
   wire [47:0]   p1_mc_req_vadr_o;
   wire [1:0]    p1_mc_req_size_o;
   wire          p1_mc_req_stall_o;

   // Port 1 even response
   wire [31:0]   p1_mc_rsp_rdctl_e;
   wire [63:0]   p1_mc_rsp_data_e;
   wire          p1_mc_rsp_push_e;
   wire          p1_mc_rsp_stall_e;

   // Port 1 odd response
   wire [31:0]   p1_mc_rsp_rdctl_o;
   wire [63:0]   p1_mc_rsp_data_o;
   wire          p1_mc_rsp_push_o;
   wire          p1_mc_rsp_stall_o;

   // Port 2 even request
   wire          p2_mc_req_idle_e;
   wire          p2_mc_req_ld_e;
   wire          p2_mc_req_st_e;
   wire [63:0]   p2_mc_req_wrd_rdctl_e;
   wire [47:0]   p2_mc_req_vadr_e;
   wire [1:0]    p2_mc_req_size_e;
   wire          p2_mc_req_stall_e;

   // Port 2 odd request
   wire          p2_mc_req_idle_o;
   wire          p2_mc_req_ld_o;
   wire          p2_mc_req_st_o;
   wire [63:0]   p2_mc_req_wrd_rdctl_o;
   wire [47:0]   p2_mc_req_vadr_o;
   wire [1:0]    p2_mc_req_size_o;
   wire          p2_mc_req_stall_o;

   // Port 2 even response
   wire [31:0]   p2_mc_rsp_rdctl_e;
   wire [63:0]   p2_mc_rsp_data_e;
   wire          p2_mc_rsp_push_e;
   wire          p2_mc_rsp_stall_e;

   // Port 2 odd response
   wire [31:0]   p2_mc_rsp_rdctl_o;
   wire [63:0]   p2_mc_rsp_data_o;
   wire          p2_mc_rsp_push_o;
   wire          p2_mc_rsp_stall_o;

   // Port 3 even request
   wire          p3_mc_req_idle_e;
   wire          p3_mc_req_ld_e;
   wire          p3_mc_req_st_e;
   wire [63:0]   p3_mc_req_wrd_rdctl_e;
   wire [47:0]   p3_mc_req_vadr_e;
   wire [1:0]    p3_mc_req_size_e;
   wire          p3_mc_req_stall_e;

   // Port 3 odd request
   wire          p3_mc_req_idle_o;
   wire          p3_mc_req_ld_o;
   wire          p3_mc_req_st_o;
   wire [63:0]   p3_mc_req_wrd_rdctl_o;
   wire [47:0]   p3_mc_req_vadr_o;
   wire [1:0]    p3_mc_req_size_o;
   wire          p3_mc_req_stall_o;

   // Port 3 even response
   wire [31:0]   p3_mc_rsp_rdctl_e;
   wire [63:0]   p3_mc_rsp_data_e;
   wire          p3_mc_rsp_push_e;
   wire          p3_mc_rsp_stall_e;

   // Port 3 odd response
   wire [31:0]   p3_mc_rsp_rdctl_o;
   wire [63:0]   p3_mc_rsp_data_o;
   wire          p3_mc_rsp_push_o;
   wire          p3_mc_rsp_stall_o;

   // Port 4 even request
   wire          p4_mc_req_idle_e;
   wire          p4_mc_req_ld_e;
   wire          p4_mc_req_st_e;
   wire [63:0]   p4_mc_req_wrd_rdctl_e;
   wire [47:0]   p4_mc_req_vadr_e;
   wire [1:0]    p4_mc_req_size_e;
   wire          p4_mc_req_stall_e;

   // Port 4 odd request
   wire          p4_mc_req_idle_o;
   wire          p4_mc_req_ld_o;
   wire          p4_mc_req_st_o;
   wire [63:0]   p4_mc_req_wrd_rdctl_o;
   wire [47:0]   p4_mc_req_vadr_o;
   wire [1:0]    p4_mc_req_size_o;
   wire          p4_mc_req_stall_o;

   // Port 4 even response
   wire [31:0]   p4_mc_rsp_rdctl_e;
   wire [63:0]   p4_mc_rsp_data_e;
   wire          p4_mc_rsp_push_e;
   wire          p4_mc_rsp_stall_e;

   // Port 4 odd response
   wire [31:0]   p4_mc_rsp_rdctl_o;
   wire [63:0]   p4_mc_rsp_data_o;
   wire          p4_mc_rsp_push_o;
   wire          p4_mc_rsp_stall_o;

   // Port 5 even request
   wire          p5_mc_req_idle_e;
   wire          p5_mc_req_ld_e;
   wire          p5_mc_req_st_e;
   wire [63:0]   p5_mc_req_wrd_rdctl_e;
   wire [47:0]   p5_mc_req_vadr_e;
   wire [1:0]    p5_mc_req_size_e;
   wire          p5_mc_req_stall_e;

   // Port 5 odd request
   wire          p5_mc_req_idle_o;
   wire          p5_mc_req_ld_o;
   wire          p5_mc_req_st_o;
   wire [63:0]   p5_mc_req_wrd_rdctl_o;
   wire [47:0]   p5_mc_req_vadr_o;
   wire [1:0]    p5_mc_req_size_o;
   wire          p5_mc_req_stall_o;

   // Port 5 even response
   wire [31:0]   p5_mc_rsp_rdctl_e;
   wire [63:0]   p5_mc_rsp_data_e;
   wire          p5_mc_rsp_push_e;
   wire          p5_mc_rsp_stall_e;

   // Port 5 odd response
   wire [31:0]   p5_mc_rsp_rdctl_o;
   wire [63:0]   p5_mc_rsp_data_o;
   wire          p5_mc_rsp_push_o;
   wire          p5_mc_rsp_stall_o;

   // Port 6 even request
   wire          p6_mc_req_idle_e;
   wire          p6_mc_req_ld_e;
   wire          p6_mc_req_st_e;
   wire [63:0]   p6_mc_req_wrd_rdctl_e;
   wire [47:0]   p6_mc_req_vadr_e;
   wire [1:0]    p6_mc_req_size_e;
   wire          p6_mc_req_stall_e;

   // Port 6 odd request
   wire          p6_mc_req_idle_o;
   wire          p6_mc_req_ld_o;
   wire          p6_mc_req_st_o;
   wire [63:0]   p6_mc_req_wrd_rdctl_o;
   wire [47:0]   p6_mc_req_vadr_o;
   wire [1:0]    p6_mc_req_size_o;
   wire          p6_mc_req_stall_o;

   // Port 6 even response
   wire [31:0]   p6_mc_rsp_rdctl_e;
   wire [63:0]   p6_mc_rsp_data_e;
   wire          p6_mc_rsp_push_e;
   wire          p6_mc_rsp_stall_e;

   // Port 6 odd response
   wire [31:0]   p6_mc_rsp_rdctl_o;
   wire [63:0]   p6_mc_rsp_data_o;
   wire          p6_mc_rsp_push_o;
   wire          p6_mc_rsp_stall_o;

   // Port 7 even request
   wire          p7_mc_req_idle_e;
   wire          p7_mc_req_ld_e;
   wire          p7_mc_req_st_e;
   wire [63:0]   p7_mc_req_wrd_rdctl_e;
   wire [47:0]   p7_mc_req_vadr_e;
   wire [1:0]    p7_mc_req_size_e;
   wire          p7_mc_req_stall_e;

   // Port 7 odd request
   wire          p7_mc_req_idle_o;
   wire          p7_mc_req_ld_o;
   wire          p7_mc_req_st_o;
   wire [63:0]   p7_mc_req_wrd_rdctl_o;
   wire [47:0]   p7_mc_req_vadr_o;
   wire [1:0]    p7_mc_req_size_o;
   wire          p7_mc_req_stall_o;

   // Port 7 even response
   wire [31:0]   p7_mc_rsp_rdctl_e;
   wire [63:0]   p7_mc_rsp_data_e;
   wire          p7_mc_rsp_push_e;
   wire          p7_mc_rsp_stall_e;

   // Port 7 odd response
   wire [31:0]   p7_mc_rsp_rdctl_o;
   wire [63:0]   p7_mc_rsp_data_o;
   wire          p7_mc_rsp_push_o;
   wire          p7_mc_rsp_stall_o;


   wire          cae_core_idle;
   wire          c_xbar_idle;
   reg           r_xbar_idle;
   wire          c_cae_idle;
   reg           r_cae_idle;

   wire [7:0]    r_xbo_rsp_ovrflw_alarm;
   wire [7:0]    r_xbe_rsp_ovrflw_alarm;
   wire [7:0]    r_xbo_rsp_undflw_alarm;
   wire [7:0]    r_xbe_rsp_undflw_alarm;
   wire [7:0]    r_xbo_req_ovrflw_alarm;
   wire [7:0]    r_xbe_req_ovrflw_alarm;
   wire [7:0]    r_xbo_req_undflw_alarm;
   wire [7:0]    r_xbe_req_undflw_alarm;

   wire [63:0]   r_xb_alarm;

   /*AUTOWIRE*/

   /* ----------      combinatorial blocks    ---------- */

   assign c_xbar_idle = (
                         p0_mc_req_idle_e &
                         p0_mc_req_idle_o &
                         p1_mc_req_idle_e &
                         p1_mc_req_idle_o &
                         p2_mc_req_idle_e &
                         p2_mc_req_idle_o &
                         p3_mc_req_idle_e &
                         p3_mc_req_idle_o &
                         p4_mc_req_idle_e &
                         p4_mc_req_idle_o &
                         p5_mc_req_idle_e &
                         p5_mc_req_idle_o &
                         p6_mc_req_idle_e &
                         p6_mc_req_idle_o &
                         p7_mc_req_idle_e &
                         p7_mc_req_idle_o &
                         1'b1
                        );

   assign c_cae_idle = cae_core_idle && r_xbar_idle;
   assign cae_idle = r_cae_idle;

   assign r_xb_alarm = {r_xbo_rsp_ovrflw_alarm[7:0], r_xbe_rsp_ovrflw_alarm[7:0],
                        r_xbo_rsp_undflw_alarm[7:0], r_xbe_rsp_undflw_alarm[7:0],
                        r_xbo_req_ovrflw_alarm[7:0], r_xbe_req_ovrflw_alarm[7:0],
                        r_xbo_req_undflw_alarm[7:0], r_xbe_req_undflw_alarm[7:0]};

   /* ----------      external module calls   ---------- */

   cae_pers_core cae_pers_core 
   (
      .clk_csr			(clk_csr),
      .clk			(clk),
      .clk2x			(clk2x),
      .i_reset			(i_reset),
      .i_csr_reset_n		(i_csr_reset_n),
      .i_aeid			(i_aeid[1:0]),
      .ppll_reset		(ppll_reset),
      .cae_inst			(cae_inst[31:0]),
      .cae_data			(cae_data[63:0]),
      .cae_inst_vld		(cae_inst_vld),
      .cae_ring_ctl_in		(cae_ring_ctl_in[3:0]),
      .cae_ring_data_in		(cae_ring_data_in[15:0]),
      .csr_31_31_intlv_dis	(csr_31_31_intlv_dis),
      .ppll_locked		(ppll_locked),
      .clk_per			(clk_per),
      .cae_aeg_cnt		(cae_aeg_cnt[17:0]),
      .cae_exception		(cae_exception[15:0]),
      .cae_ret_data		(cae_ret_data[63:0]),
      .cae_ret_data_vld		(cae_ret_data_vld),
      .cae_idle			(cae_core_idle),
      .cae_stall		(cae_stall),
      .cae_ring_ctl_out		(cae_ring_ctl_out[3:0]),
      .cae_ring_data_out	(cae_ring_data_out[15:0]),
      .mc0_req_ld_e        	(p0_mc_req_ld_e),
      .mc0_req_st_e        	(p0_mc_req_st_e),
      .mc0_req_wrd_rdctl_e 	(p0_mc_req_wrd_rdctl_e),
      .mc0_req_vadr_e      	(p0_mc_req_vadr_e),
      .mc0_req_size_e      	(p0_mc_req_size_e),
      //.mc0_rd_rq_stall_e   	(p0_mc_rd_rq_stall_e),
      //.mc0_wr_rq_stall_e   	(p0_mc_wr_rq_stall_e),
      .mc0_rd_rq_stall_e   	(p0_mc_req_stall_e),
      .mc0_wr_rq_stall_e   	(p0_mc_req_stall_e),
      .mc0_req_ld_o        	(p0_mc_req_ld_o),
      .mc0_req_st_o        	(p0_mc_req_st_o),
      .mc0_req_wrd_rdctl_o 	(p0_mc_req_wrd_rdctl_o),
      .mc0_req_vadr_o      	(p0_mc_req_vadr_o),
      .mc0_req_size_o      	(p0_mc_req_size_o),
      //.mc0_rd_rq_stall_o   	(p0_mc_rd_rq_stall_o),
      //.mc0_wr_rq_stall_o   	(p0_mc_wr_rq_stall_o),
      .mc0_rd_rq_stall_o   	(p0_mc_req_stall_o),
      .mc0_wr_rq_stall_o   	(p0_mc_req_stall_o),
      .mc0_rsp_rdctl_e     	(p0_mc_rsp_rdctl_e[31:0]),
      .mc0_rsp_data_e      	(p0_mc_rsp_data_e[63:0]),
      .mc0_rsp_push_e      	(p0_mc_rsp_push_e),
      .mc0_rsp_stall_e     	(p0_mc_rsp_stall_e),
      .mc0_rsp_rdctl_o     	(p0_mc_rsp_rdctl_o[31:0]),
      .mc0_rsp_data_o      	(p0_mc_rsp_data_o[63:0]),
      .mc0_rsp_push_o      	(p0_mc_rsp_push_o),
      .mc0_rsp_stall_o     	(p0_mc_rsp_stall_o),
      .mc1_req_ld_e        	(p1_mc_req_ld_e),
      .mc1_req_st_e        	(p1_mc_req_st_e),
      .mc1_req_wrd_rdctl_e 	(p1_mc_req_wrd_rdctl_e),
      .mc1_req_vadr_e      	(p1_mc_req_vadr_e),
      .mc1_req_size_e      	(p1_mc_req_size_e),
      //.mc1_rd_rq_stall_e   	(p1_mc_rd_rq_stall_e),
      //.mc1_wr_rq_stall_e   	(p1_mc_wr_rq_stall_e),
      .mc1_rd_rq_stall_e   	(p1_mc_req_stall_e),
      .mc1_wr_rq_stall_e   	(p1_mc_req_stall_e),
      .mc1_req_ld_o        	(p1_mc_req_ld_o),
      .mc1_req_st_o        	(p1_mc_req_st_o),
      .mc1_req_wrd_rdctl_o 	(p1_mc_req_wrd_rdctl_o),
      .mc1_req_vadr_o      	(p1_mc_req_vadr_o),
      .mc1_req_size_o      	(p1_mc_req_size_o),
      //.mc1_rd_rq_stall_o   	(p1_mc_rd_rq_stall_o),
      //.mc1_wr_rq_stall_o   	(p1_mc_wr_rq_stall_o),
      .mc1_rd_rq_stall_o   	(p1_mc_req_stall_o),
      .mc1_wr_rq_stall_o   	(p1_mc_req_stall_o),
      .mc1_rsp_rdctl_e     	(p1_mc_rsp_rdctl_e[31:0]),
      .mc1_rsp_data_e      	(p1_mc_rsp_data_e[63:0]),
      .mc1_rsp_push_e      	(p1_mc_rsp_push_e),
      .mc1_rsp_stall_e     	(p1_mc_rsp_stall_e),
      .mc1_rsp_rdctl_o     	(p1_mc_rsp_rdctl_o[31:0]),
      .mc1_rsp_data_o      	(p1_mc_rsp_data_o[63:0]),
      .mc1_rsp_push_o      	(p1_mc_rsp_push_o),
      .mc1_rsp_stall_o     	(p1_mc_rsp_stall_o),
      .mc2_req_ld_e        	(p2_mc_req_ld_e),
      .mc2_req_st_e        	(p2_mc_req_st_e),
      .mc2_req_wrd_rdctl_e 	(p2_mc_req_wrd_rdctl_e),
      .mc2_req_vadr_e      	(p2_mc_req_vadr_e),
      .mc2_req_size_e      	(p2_mc_req_size_e),
      //.mc2_rd_rq_stall_e   	(p2_mc_rd_rq_stall_e),
      //.mc2_wr_rq_stall_e   	(p2_mc_wr_rq_stall_e),
      .mc2_rd_rq_stall_e   	(p2_mc_req_stall_e),
      .mc2_wr_rq_stall_e   	(p2_mc_req_stall_e),
      .mc2_req_ld_o        	(p2_mc_req_ld_o),
      .mc2_req_st_o        	(p2_mc_req_st_o),
      .mc2_req_wrd_rdctl_o 	(p2_mc_req_wrd_rdctl_o),
      .mc2_req_vadr_o      	(p2_mc_req_vadr_o),
      .mc2_req_size_o      	(p2_mc_req_size_o),
      //.mc2_rd_rq_stall_o   	(p2_mc_rd_rq_stall_o),
      //.mc2_wr_rq_stall_o   	(p2_mc_wr_rq_stall_o),
      .mc2_rd_rq_stall_o   	(p2_mc_req_stall_o),
      .mc2_wr_rq_stall_o   	(p2_mc_req_stall_o),
      .mc2_rsp_rdctl_e     	(p2_mc_rsp_rdctl_e[31:0]),
      .mc2_rsp_data_e      	(p2_mc_rsp_data_e[63:0]),
      .mc2_rsp_push_e      	(p2_mc_rsp_push_e),
      .mc2_rsp_stall_e     	(p2_mc_rsp_stall_e),
      .mc2_rsp_rdctl_o     	(p2_mc_rsp_rdctl_o[31:0]),
      .mc2_rsp_data_o      	(p2_mc_rsp_data_o[63:0]),
      .mc2_rsp_push_o      	(p2_mc_rsp_push_o),
      .mc2_rsp_stall_o     	(p2_mc_rsp_stall_o),
      .mc3_req_ld_e        	(p3_mc_req_ld_e),
      .mc3_req_st_e        	(p3_mc_req_st_e),
      .mc3_req_wrd_rdctl_e 	(p3_mc_req_wrd_rdctl_e),
      .mc3_req_vadr_e      	(p3_mc_req_vadr_e),
      .mc3_req_size_e      	(p3_mc_req_size_e),
      //.mc3_rd_rq_stall_e   	(p3_mc_rd_rq_stall_e),
      //.mc3_wr_rq_stall_e   	(p3_mc_wr_rq_stall_e),
      .mc3_rd_rq_stall_e   	(p3_mc_req_stall_e),
      .mc3_wr_rq_stall_e   	(p3_mc_req_stall_e),
      .mc3_req_ld_o        	(p3_mc_req_ld_o),
      .mc3_req_st_o        	(p3_mc_req_st_o),
      .mc3_req_wrd_rdctl_o 	(p3_mc_req_wrd_rdctl_o),
      .mc3_req_vadr_o      	(p3_mc_req_vadr_o),
      .mc3_req_size_o      	(p3_mc_req_size_o),
      //.mc3_rd_rq_stall_o   	(p3_mc_rd_rq_stall_o),
      //.mc3_wr_rq_stall_o   	(p3_mc_wr_rq_stall_o),
      .mc3_rd_rq_stall_o   	(p3_mc_req_stall_o),
      .mc3_wr_rq_stall_o   	(p3_mc_req_stall_o),
      .mc3_rsp_rdctl_e     	(p3_mc_rsp_rdctl_e[31:0]),
      .mc3_rsp_data_e      	(p3_mc_rsp_data_e[63:0]),
      .mc3_rsp_push_e      	(p3_mc_rsp_push_e),
      .mc3_rsp_stall_e     	(p3_mc_rsp_stall_e),
      .mc3_rsp_rdctl_o     	(p3_mc_rsp_rdctl_o[31:0]),
      .mc3_rsp_data_o      	(p3_mc_rsp_data_o[63:0]),
      .mc3_rsp_push_o      	(p3_mc_rsp_push_o),
      .mc3_rsp_stall_o     	(p3_mc_rsp_stall_o),
      .mc4_req_ld_e        	(p4_mc_req_ld_e),
      .mc4_req_st_e        	(p4_mc_req_st_e),
      .mc4_req_wrd_rdctl_e 	(p4_mc_req_wrd_rdctl_e),
      .mc4_req_vadr_e      	(p4_mc_req_vadr_e),
      .mc4_req_size_e      	(p4_mc_req_size_e),
      //.mc4_rd_rq_stall_e   	(p4_mc_rd_rq_stall_e),
      //.mc4_wr_rq_stall_e   	(p4_mc_wr_rq_stall_e),
      .mc4_rd_rq_stall_e   	(p4_mc_req_stall_e),
      .mc4_wr_rq_stall_e   	(p4_mc_req_stall_e),
      .mc4_req_ld_o        	(p4_mc_req_ld_o),
      .mc4_req_st_o        	(p4_mc_req_st_o),
      .mc4_req_wrd_rdctl_o 	(p4_mc_req_wrd_rdctl_o),
      .mc4_req_vadr_o      	(p4_mc_req_vadr_o),
      .mc4_req_size_o      	(p4_mc_req_size_o),
      //.mc4_rd_rq_stall_o   	(p4_mc_rd_rq_stall_o),
      //.mc4_wr_rq_stall_o   	(p4_mc_wr_rq_stall_o),
      .mc4_rd_rq_stall_o   	(p4_mc_req_stall_o),
      .mc4_wr_rq_stall_o   	(p4_mc_req_stall_o),
      .mc4_rsp_rdctl_e     	(p4_mc_rsp_rdctl_e[31:0]),
      .mc4_rsp_data_e      	(p4_mc_rsp_data_e[63:0]),
      .mc4_rsp_push_e      	(p4_mc_rsp_push_e),
      .mc4_rsp_stall_e     	(p4_mc_rsp_stall_e),
      .mc4_rsp_rdctl_o     	(p4_mc_rsp_rdctl_o[31:0]),
      .mc4_rsp_data_o      	(p4_mc_rsp_data_o[63:0]),
      .mc4_rsp_push_o      	(p4_mc_rsp_push_o),
      .mc4_rsp_stall_o     	(p4_mc_rsp_stall_o),
      .mc5_req_ld_e        	(p5_mc_req_ld_e),
      .mc5_req_st_e        	(p5_mc_req_st_e),
      .mc5_req_wrd_rdctl_e 	(p5_mc_req_wrd_rdctl_e),
      .mc5_req_vadr_e      	(p5_mc_req_vadr_e),
      .mc5_req_size_e      	(p5_mc_req_size_e),
      //.mc5_rd_rq_stall_e   	(p5_mc_rd_rq_stall_e),
      //.mc5_wr_rq_stall_e   	(p5_mc_wr_rq_stall_e),
      .mc5_rd_rq_stall_e   	(p5_mc_req_stall_e),
      .mc5_wr_rq_stall_e   	(p5_mc_req_stall_e),
      .mc5_req_ld_o        	(p5_mc_req_ld_o),
      .mc5_req_st_o        	(p5_mc_req_st_o),
      .mc5_req_wrd_rdctl_o 	(p5_mc_req_wrd_rdctl_o),
      .mc5_req_vadr_o      	(p5_mc_req_vadr_o),
      .mc5_req_size_o      	(p5_mc_req_size_o),
      //.mc5_rd_rq_stall_o   	(p5_mc_rd_rq_stall_o),
      //.mc5_wr_rq_stall_o   	(p5_mc_wr_rq_stall_o),
      .mc5_rd_rq_stall_o   	(p5_mc_req_stall_o),
      .mc5_wr_rq_stall_o   	(p5_mc_req_stall_o),
      .mc5_rsp_rdctl_e     	(p5_mc_rsp_rdctl_e[31:0]),
      .mc5_rsp_data_e      	(p5_mc_rsp_data_e[63:0]),
      .mc5_rsp_push_e      	(p5_mc_rsp_push_e),
      .mc5_rsp_stall_e     	(p5_mc_rsp_stall_e),
      .mc5_rsp_rdctl_o     	(p5_mc_rsp_rdctl_o[31:0]),
      .mc5_rsp_data_o      	(p5_mc_rsp_data_o[63:0]),
      .mc5_rsp_push_o      	(p5_mc_rsp_push_o),
      .mc5_rsp_stall_o     	(p5_mc_rsp_stall_o),
      .mc6_req_ld_e        	(p6_mc_req_ld_e),
      .mc6_req_st_e        	(p6_mc_req_st_e),
      .mc6_req_wrd_rdctl_e 	(p6_mc_req_wrd_rdctl_e),
      .mc6_req_vadr_e      	(p6_mc_req_vadr_e),
      .mc6_req_size_e      	(p6_mc_req_size_e),
      //.mc6_rd_rq_stall_e   	(p6_mc_rd_rq_stall_e),
      //.mc6_wr_rq_stall_e   	(p6_mc_wr_rq_stall_e),
      .mc6_rd_rq_stall_e   	(p6_mc_req_stall_e),
      .mc6_wr_rq_stall_e   	(p6_mc_req_stall_e),
      .mc6_req_ld_o        	(p6_mc_req_ld_o),
      .mc6_req_st_o        	(p6_mc_req_st_o),
      .mc6_req_wrd_rdctl_o 	(p6_mc_req_wrd_rdctl_o),
      .mc6_req_vadr_o      	(p6_mc_req_vadr_o),
      .mc6_req_size_o      	(p6_mc_req_size_o),
      //.mc6_rd_rq_stall_o   	(p6_mc_rd_rq_stall_o),
      //.mc6_wr_rq_stall_o   	(p6_mc_wr_rq_stall_o),
      .mc6_rd_rq_stall_o   	(p6_mc_req_stall_o),
      .mc6_wr_rq_stall_o   	(p6_mc_req_stall_o),
      .mc6_rsp_rdctl_e     	(p6_mc_rsp_rdctl_e[31:0]),
      .mc6_rsp_data_e      	(p6_mc_rsp_data_e[63:0]),
      .mc6_rsp_push_e      	(p6_mc_rsp_push_e),
      .mc6_rsp_stall_e     	(p6_mc_rsp_stall_e),
      .mc6_rsp_rdctl_o     	(p6_mc_rsp_rdctl_o[31:0]),
      .mc6_rsp_data_o      	(p6_mc_rsp_data_o[63:0]),
      .mc6_rsp_push_o      	(p6_mc_rsp_push_o),
      .mc6_rsp_stall_o     	(p6_mc_rsp_stall_o),
      .mc7_req_ld_e        	(p7_mc_req_ld_e),
      .mc7_req_st_e        	(p7_mc_req_st_e),
      .mc7_req_wrd_rdctl_e 	(p7_mc_req_wrd_rdctl_e),
      .mc7_req_vadr_e      	(p7_mc_req_vadr_e),
      .mc7_req_size_e      	(p7_mc_req_size_e),
      //.mc7_rd_rq_stall_e   	(p7_mc_rd_rq_stall_e),
      //.mc7_wr_rq_stall_e   	(p7_mc_wr_rq_stall_e),
      .mc7_rd_rq_stall_e   	(p7_mc_req_stall_e),
      .mc7_wr_rq_stall_e   	(p7_mc_req_stall_e),
      .mc7_req_ld_o        	(p7_mc_req_ld_o),
      .mc7_req_st_o        	(p7_mc_req_st_o),
      .mc7_req_wrd_rdctl_o 	(p7_mc_req_wrd_rdctl_o),
      .mc7_req_vadr_o      	(p7_mc_req_vadr_o),
      .mc7_req_size_o      	(p7_mc_req_size_o),
      //.mc7_rd_rq_stall_o   	(p7_mc_rd_rq_stall_o),
      //.mc7_wr_rq_stall_o   	(p7_mc_wr_rq_stall_o),
      .mc7_rd_rq_stall_o   	(p7_mc_req_stall_o),
      .mc7_wr_rq_stall_o   	(p7_mc_req_stall_o),
      .mc7_rsp_rdctl_e     	(p7_mc_rsp_rdctl_e[31:0]),
      .mc7_rsp_data_e      	(p7_mc_rsp_data_e[63:0]),
      .mc7_rsp_push_e      	(p7_mc_rsp_push_e),
      .mc7_rsp_stall_e     	(p7_mc_rsp_stall_e),
      .mc7_rsp_rdctl_o     	(p7_mc_rsp_rdctl_o[31:0]),
      .mc7_rsp_data_o      	(p7_mc_rsp_data_o[63:0]),
      .mc7_rsp_push_o      	(p7_mc_rsp_push_o),
      .mc7_rsp_stall_o     	(p7_mc_rsp_stall_o),
      .tx_stall			(tx_stall),
      .rx_data			(rx_data[63:0]),
      .rx_cmd			(rx_cmd[23:0]),
      .rx_src			(rx_src[1:0]),
      .rx_valid			(rx_valid),
      .tx_data			(tx_data[63:0]),
      .tx_cmd			(tx_cmd[23:0]),
      .tx_dest			(tx_dest[1:0]),
      .tx_valid			(tx_valid),
      .r_xb_alarm               (r_xb_alarm[63:0]),
      /*AUTOINST*/
    // Outputs
    .mc0_req_flush_e			(mc0_req_flush_e),
    .mc0_req_flush_o			(mc0_req_flush_o),
    .mc1_req_flush_e			(mc1_req_flush_e),
    .mc1_req_flush_o			(mc1_req_flush_o),
    .mc2_req_flush_e			(mc2_req_flush_e),
    .mc2_req_flush_o			(mc2_req_flush_o),
    .mc3_req_flush_e			(mc3_req_flush_e),
    .mc3_req_flush_o			(mc3_req_flush_o),
    .mc4_req_flush_e			(mc4_req_flush_e),
    .mc4_req_flush_o			(mc4_req_flush_o),
    .mc5_req_flush_e			(mc5_req_flush_e),
    .mc5_req_flush_o			(mc5_req_flush_o),
    .mc6_req_flush_e			(mc6_req_flush_e),
    .mc6_req_flush_o			(mc6_req_flush_o),
    .mc7_req_flush_e			(mc7_req_flush_e),
    .mc7_req_flush_o			(mc7_req_flush_o),
    // Inputs
    .mc0_rsp_flush_cmplt_e		(mc0_rsp_flush_cmplt_e),
    .mc0_rsp_flush_cmplt_o		(mc0_rsp_flush_cmplt_o),
    .mc1_rsp_flush_cmplt_e		(mc1_rsp_flush_cmplt_e),
    .mc1_rsp_flush_cmplt_o		(mc1_rsp_flush_cmplt_o),
    .mc2_rsp_flush_cmplt_e		(mc2_rsp_flush_cmplt_e),
    .mc2_rsp_flush_cmplt_o		(mc2_rsp_flush_cmplt_o),
    .mc3_rsp_flush_cmplt_e		(mc3_rsp_flush_cmplt_e),
    .mc3_rsp_flush_cmplt_o		(mc3_rsp_flush_cmplt_o),
    .mc4_rsp_flush_cmplt_e		(mc4_rsp_flush_cmplt_e),
    .mc4_rsp_flush_cmplt_o		(mc4_rsp_flush_cmplt_o),
    .mc5_rsp_flush_cmplt_e		(mc5_rsp_flush_cmplt_e),
    .mc5_rsp_flush_cmplt_o		(mc5_rsp_flush_cmplt_o),
    .mc6_rsp_flush_cmplt_e		(mc6_rsp_flush_cmplt_e),
    .mc6_rsp_flush_cmplt_o		(mc6_rsp_flush_cmplt_o),
    .mc7_rsp_flush_cmplt_e		(mc7_rsp_flush_cmplt_e),
    .mc7_rsp_flush_cmplt_o		(mc7_rsp_flush_cmplt_o));

   pdk_req_mxb req_xbe (
      .p0_mc_req_idle	(p0_mc_req_idle_e),
      .p0_mc_req_stall	(p0_mc_req_stall_e),
      .mc0_req_ld		(mc0_req_ld_e),
      .mc0_req_st		(mc0_req_st_e),
      .mc0_req_vadr		(mc0_req_vadr_e),
      .mc0_req_size		(mc0_req_size_e),
      .mc0_req_wrd_rdctl	(mc0_req_wrd_rdctl_e[63:0]),
      .p0_mc_req_ld		(p0_mc_req_ld_e),
      .p0_mc_req_st		(p0_mc_req_st_e),
      //.p0_mc_req_last	(p0_mc_req_last_e),
      .p0_mc_req_last	(1'b0),
      .p0_mc_req_vadr	(p0_mc_req_vadr_e),
      .p0_mc_req_size	(p0_mc_req_size_e),
      .p0_mc_req_wrd_rdctl	(p0_mc_req_wrd_rdctl_e[63:0]),
      .mc0_rd_rq_stall	(mc0_rd_rq_stall_e),
      .mc0_wr_rq_stall	(mc0_wr_rq_stall_e),
      .p1_mc_req_idle	(p1_mc_req_idle_e),
      .p1_mc_req_stall	(p1_mc_req_stall_e),
      .mc1_req_ld		(mc1_req_ld_e),
      .mc1_req_st		(mc1_req_st_e),
      .mc1_req_vadr		(mc1_req_vadr_e),
      .mc1_req_size		(mc1_req_size_e),
      .mc1_req_wrd_rdctl	(mc1_req_wrd_rdctl_e[63:0]),
      .p1_mc_req_ld		(p1_mc_req_ld_e),
      .p1_mc_req_st		(p1_mc_req_st_e),
      //.p1_mc_req_last	(p1_mc_req_last_e),
      .p1_mc_req_last	(1'b0),
      .p1_mc_req_vadr	(p1_mc_req_vadr_e),
      .p1_mc_req_size	(p1_mc_req_size_e),
      .p1_mc_req_wrd_rdctl	(p1_mc_req_wrd_rdctl_e[63:0]),
      .mc1_rd_rq_stall	(mc1_rd_rq_stall_e),
      .mc1_wr_rq_stall	(mc1_wr_rq_stall_e),
      .p2_mc_req_idle	(p2_mc_req_idle_e),
      .p2_mc_req_stall	(p2_mc_req_stall_e),
      .mc2_req_ld		(mc2_req_ld_e),
      .mc2_req_st		(mc2_req_st_e),
      .mc2_req_vadr		(mc2_req_vadr_e),
      .mc2_req_size		(mc2_req_size_e),
      .mc2_req_wrd_rdctl	(mc2_req_wrd_rdctl_e[63:0]),
      .p2_mc_req_ld		(p2_mc_req_ld_e),
      .p2_mc_req_st		(p2_mc_req_st_e),
      //.p2_mc_req_last	(p2_mc_req_last_e),
      .p2_mc_req_last	(1'b0),
      .p2_mc_req_vadr	(p2_mc_req_vadr_e),
      .p2_mc_req_size	(p2_mc_req_size_e),
      .p2_mc_req_wrd_rdctl	(p2_mc_req_wrd_rdctl_e[63:0]),
      .mc2_rd_rq_stall	(mc2_rd_rq_stall_e),
      .mc2_wr_rq_stall	(mc2_wr_rq_stall_e),
      .p3_mc_req_idle	(p3_mc_req_idle_e),
      .p3_mc_req_stall	(p3_mc_req_stall_e),
      .mc3_req_ld		(mc3_req_ld_e),
      .mc3_req_st		(mc3_req_st_e),
      .mc3_req_vadr		(mc3_req_vadr_e),
      .mc3_req_size		(mc3_req_size_e),
      .mc3_req_wrd_rdctl	(mc3_req_wrd_rdctl_e[63:0]),
      .p3_mc_req_ld		(p3_mc_req_ld_e),
      .p3_mc_req_st		(p3_mc_req_st_e),
      //.p3_mc_req_last	(p3_mc_req_last_e),
      .p3_mc_req_last	(1'b0),
      .p3_mc_req_vadr	(p3_mc_req_vadr_e),
      .p3_mc_req_size	(p3_mc_req_size_e),
      .p3_mc_req_wrd_rdctl	(p3_mc_req_wrd_rdctl_e[63:0]),
      .mc3_rd_rq_stall	(mc3_rd_rq_stall_e),
      .mc3_wr_rq_stall	(mc3_wr_rq_stall_e),
      .p4_mc_req_idle	(p4_mc_req_idle_e),
      .p4_mc_req_stall	(p4_mc_req_stall_e),
      .mc4_req_ld		(mc4_req_ld_e),
      .mc4_req_st		(mc4_req_st_e),
      .mc4_req_vadr		(mc4_req_vadr_e),
      .mc4_req_size		(mc4_req_size_e),
      .mc4_req_wrd_rdctl	(mc4_req_wrd_rdctl_e[63:0]),
      .p4_mc_req_ld		(p4_mc_req_ld_e),
      .p4_mc_req_st		(p4_mc_req_st_e),
      //.p4_mc_req_last	(p4_mc_req_last_e),
      .p4_mc_req_last	(1'b0),
      .p4_mc_req_vadr	(p4_mc_req_vadr_e),
      .p4_mc_req_size	(p4_mc_req_size_e),
      .p4_mc_req_wrd_rdctl	(p4_mc_req_wrd_rdctl_e[63:0]),
      .mc4_rd_rq_stall	(mc4_rd_rq_stall_e),
      .mc4_wr_rq_stall	(mc4_wr_rq_stall_e),
      .p5_mc_req_idle	(p5_mc_req_idle_e),
      .p5_mc_req_stall	(p5_mc_req_stall_e),
      .mc5_req_ld		(mc5_req_ld_e),
      .mc5_req_st		(mc5_req_st_e),
      .mc5_req_vadr		(mc5_req_vadr_e),
      .mc5_req_size		(mc5_req_size_e),
      .mc5_req_wrd_rdctl	(mc5_req_wrd_rdctl_e[63:0]),
      .p5_mc_req_ld		(p5_mc_req_ld_e),
      .p5_mc_req_st		(p5_mc_req_st_e),
      //.p5_mc_req_last	(p5_mc_req_last_e),
      .p5_mc_req_last	(1'b0),
      .p5_mc_req_vadr	(p5_mc_req_vadr_e),
      .p5_mc_req_size	(p5_mc_req_size_e),
      .p5_mc_req_wrd_rdctl	(p5_mc_req_wrd_rdctl_e[63:0]),
      .mc5_rd_rq_stall	(mc5_rd_rq_stall_e),
      .mc5_wr_rq_stall	(mc5_wr_rq_stall_e),
      .p6_mc_req_idle	(p6_mc_req_idle_e),
      .p6_mc_req_stall	(p6_mc_req_stall_e),
      .mc6_req_ld		(mc6_req_ld_e),
      .mc6_req_st		(mc6_req_st_e),
      .mc6_req_vadr		(mc6_req_vadr_e),
      .mc6_req_size		(mc6_req_size_e),
      .mc6_req_wrd_rdctl	(mc6_req_wrd_rdctl_e[63:0]),
      .p6_mc_req_ld		(p6_mc_req_ld_e),
      .p6_mc_req_st		(p6_mc_req_st_e),
      //.p6_mc_req_last	(p6_mc_req_last_e),
      .p6_mc_req_last	(1'b0),
      .p6_mc_req_vadr	(p6_mc_req_vadr_e),
      .p6_mc_req_size	(p6_mc_req_size_e),
      .p6_mc_req_wrd_rdctl	(p6_mc_req_wrd_rdctl_e[63:0]),
      .mc6_rd_rq_stall	(mc6_rd_rq_stall_e),
      .mc6_wr_rq_stall	(mc6_wr_rq_stall_e),
      .p7_mc_req_idle	(p7_mc_req_idle_e),
      .p7_mc_req_stall	(p7_mc_req_stall_e),
      .mc7_req_ld		(mc7_req_ld_e),
      .mc7_req_st		(mc7_req_st_e),
      .mc7_req_vadr		(mc7_req_vadr_e),
      .mc7_req_size		(mc7_req_size_e),
      .mc7_req_wrd_rdctl	(mc7_req_wrd_rdctl_e[63:0]),
      .p7_mc_req_ld		(p7_mc_req_ld_e),
      .p7_mc_req_st		(p7_mc_req_st_e),
      //.p7_mc_req_last	(p7_mc_req_last_e),
      .p7_mc_req_last	(1'b0),
      .p7_mc_req_vadr	(p7_mc_req_vadr_e),
      .p7_mc_req_size	(p7_mc_req_size_e),
      .p7_mc_req_wrd_rdctl	(p7_mc_req_wrd_rdctl_e[63:0]),
      .mc7_rd_rq_stall	(mc7_rd_rq_stall_e),
      .mc7_wr_rq_stall	(mc7_wr_rq_stall_e),
      .r_ovrflow_alarm          (r_xbe_req_ovrflw_alarm[7:0]),
      .r_undflow_alarm          (r_xbe_req_undflw_alarm[7:0]),
      .clk			(clk),
      .clk_csr			(clk_csr),
      .i_reset			(i_reset)
      /*AUTOINST*/);

   pdk_req_mxb req_xbo (
      .p0_mc_req_idle	(p0_mc_req_idle_o),
      .p0_mc_req_stall	(p0_mc_req_stall_o),
      .mc0_req_ld		(mc0_req_ld_o),
      .mc0_req_st		(mc0_req_st_o),
      .mc0_req_vadr		(mc0_req_vadr_o),
      .mc0_req_size		(mc0_req_size_o),
      .mc0_req_wrd_rdctl	(mc0_req_wrd_rdctl_o[63:0]),
      .p0_mc_req_ld		(p0_mc_req_ld_o),
      .p0_mc_req_st		(p0_mc_req_st_o),
      //.p0_mc_req_last	(p0_mc_req_last_o),
      .p0_mc_req_last	(1'b0),
      .p0_mc_req_vadr	(p0_mc_req_vadr_o),
      .p0_mc_req_size	(p0_mc_req_size_o),
      .p0_mc_req_wrd_rdctl	(p0_mc_req_wrd_rdctl_o[63:0]),
      .mc0_rd_rq_stall	(mc0_rd_rq_stall_o),
      .mc0_wr_rq_stall	(mc0_wr_rq_stall_o),
      .p1_mc_req_idle	(p1_mc_req_idle_o),
      .p1_mc_req_stall	(p1_mc_req_stall_o),
      .mc1_req_ld		(mc1_req_ld_o),
      .mc1_req_st		(mc1_req_st_o),
      .mc1_req_vadr		(mc1_req_vadr_o),
      .mc1_req_size		(mc1_req_size_o),
      .mc1_req_wrd_rdctl	(mc1_req_wrd_rdctl_o[63:0]),
      .p1_mc_req_ld		(p1_mc_req_ld_o),
      .p1_mc_req_st		(p1_mc_req_st_o),
      //.p1_mc_req_last	(p1_mc_req_last_o),
      .p1_mc_req_last	(1'b0),
      .p1_mc_req_vadr	(p1_mc_req_vadr_o),
      .p1_mc_req_size	(p1_mc_req_size_o),
      .p1_mc_req_wrd_rdctl	(p1_mc_req_wrd_rdctl_o[63:0]),
      .mc1_rd_rq_stall	(mc1_rd_rq_stall_o),
      .mc1_wr_rq_stall	(mc1_wr_rq_stall_o),
      .p2_mc_req_idle	(p2_mc_req_idle_o),
      .p2_mc_req_stall	(p2_mc_req_stall_o),
      .mc2_req_ld		(mc2_req_ld_o),
      .mc2_req_st		(mc2_req_st_o),
      .mc2_req_vadr		(mc2_req_vadr_o),
      .mc2_req_size		(mc2_req_size_o),
      .mc2_req_wrd_rdctl	(mc2_req_wrd_rdctl_o[63:0]),
      .p2_mc_req_ld		(p2_mc_req_ld_o),
      .p2_mc_req_st		(p2_mc_req_st_o),
      //.p2_mc_req_last	(p2_mc_req_last_o),
      .p2_mc_req_last	(1'b0),
      .p2_mc_req_vadr	(p2_mc_req_vadr_o),
      .p2_mc_req_size	(p2_mc_req_size_o),
      .p2_mc_req_wrd_rdctl	(p2_mc_req_wrd_rdctl_o[63:0]),
      .mc2_rd_rq_stall	(mc2_rd_rq_stall_o),
      .mc2_wr_rq_stall	(mc2_wr_rq_stall_o),
      .p3_mc_req_idle	(p3_mc_req_idle_o),
      .p3_mc_req_stall	(p3_mc_req_stall_o),
      .mc3_req_ld		(mc3_req_ld_o),
      .mc3_req_st		(mc3_req_st_o),
      .mc3_req_vadr		(mc3_req_vadr_o),
      .mc3_req_size		(mc3_req_size_o),
      .mc3_req_wrd_rdctl	(mc3_req_wrd_rdctl_o[63:0]),
      .p3_mc_req_ld		(p3_mc_req_ld_o),
      .p3_mc_req_st		(p3_mc_req_st_o),
      //.p3_mc_req_last	(p3_mc_req_last_o),
      .p3_mc_req_last	(1'b0),
      .p3_mc_req_vadr	(p3_mc_req_vadr_o),
      .p3_mc_req_size	(p3_mc_req_size_o),
      .p3_mc_req_wrd_rdctl	(p3_mc_req_wrd_rdctl_o[63:0]),
      .mc3_rd_rq_stall	(mc3_rd_rq_stall_o),
      .mc3_wr_rq_stall	(mc3_wr_rq_stall_o),
      .p4_mc_req_idle	(p4_mc_req_idle_o),
      .p4_mc_req_stall	(p4_mc_req_stall_o),
      .mc4_req_ld		(mc4_req_ld_o),
      .mc4_req_st		(mc4_req_st_o),
      .mc4_req_vadr		(mc4_req_vadr_o),
      .mc4_req_size		(mc4_req_size_o),
      .mc4_req_wrd_rdctl	(mc4_req_wrd_rdctl_o[63:0]),
      .p4_mc_req_ld		(p4_mc_req_ld_o),
      .p4_mc_req_st		(p4_mc_req_st_o),
      //.p4_mc_req_last	(p4_mc_req_last_o),
      .p4_mc_req_last	(1'b0),
      .p4_mc_req_vadr	(p4_mc_req_vadr_o),
      .p4_mc_req_size	(p4_mc_req_size_o),
      .p4_mc_req_wrd_rdctl	(p4_mc_req_wrd_rdctl_o[63:0]),
      .mc4_rd_rq_stall	(mc4_rd_rq_stall_o),
      .mc4_wr_rq_stall	(mc4_wr_rq_stall_o),
      .p5_mc_req_idle	(p5_mc_req_idle_o),
      .p5_mc_req_stall	(p5_mc_req_stall_o),
      .mc5_req_ld		(mc5_req_ld_o),
      .mc5_req_st		(mc5_req_st_o),
      .mc5_req_vadr		(mc5_req_vadr_o),
      .mc5_req_size		(mc5_req_size_o),
      .mc5_req_wrd_rdctl	(mc5_req_wrd_rdctl_o[63:0]),
      .p5_mc_req_ld		(p5_mc_req_ld_o),
      .p5_mc_req_st		(p5_mc_req_st_o),
      //.p5_mc_req_last	(p5_mc_req_last_o),
      .p5_mc_req_last	(1'b0),
      .p5_mc_req_vadr	(p5_mc_req_vadr_o),
      .p5_mc_req_size	(p5_mc_req_size_o),
      .p5_mc_req_wrd_rdctl	(p5_mc_req_wrd_rdctl_o[63:0]),
      .mc5_rd_rq_stall	(mc5_rd_rq_stall_o),
      .mc5_wr_rq_stall	(mc5_wr_rq_stall_o),
      .p6_mc_req_idle	(p6_mc_req_idle_o),
      .p6_mc_req_stall	(p6_mc_req_stall_o),
      .mc6_req_ld		(mc6_req_ld_o),
      .mc6_req_st		(mc6_req_st_o),
      .mc6_req_vadr		(mc6_req_vadr_o),
      .mc6_req_size		(mc6_req_size_o),
      .mc6_req_wrd_rdctl	(mc6_req_wrd_rdctl_o[63:0]),
      .p6_mc_req_ld		(p6_mc_req_ld_o),
      .p6_mc_req_st		(p6_mc_req_st_o),
      //.p6_mc_req_last	(p6_mc_req_last_o),
      .p6_mc_req_last	(1'b0),
      .p6_mc_req_vadr	(p6_mc_req_vadr_o),
      .p6_mc_req_size	(p6_mc_req_size_o),
      .p6_mc_req_wrd_rdctl	(p6_mc_req_wrd_rdctl_o[63:0]),
      .mc6_rd_rq_stall	(mc6_rd_rq_stall_o),
      .mc6_wr_rq_stall	(mc6_wr_rq_stall_o),
      .p7_mc_req_idle	(p7_mc_req_idle_o),
      .p7_mc_req_stall	(p7_mc_req_stall_o),
      .mc7_req_ld		(mc7_req_ld_o),
      .mc7_req_st		(mc7_req_st_o),
      .mc7_req_vadr		(mc7_req_vadr_o),
      .mc7_req_size		(mc7_req_size_o),
      .mc7_req_wrd_rdctl	(mc7_req_wrd_rdctl_o[63:0]),
      .p7_mc_req_ld		(p7_mc_req_ld_o),
      .p7_mc_req_st		(p7_mc_req_st_o),
      //.p7_mc_req_last	(p7_mc_req_last_o),
      .p7_mc_req_last	(1'b0),
      .p7_mc_req_vadr	(p7_mc_req_vadr_o),
      .p7_mc_req_size	(p7_mc_req_size_o),
      .p7_mc_req_wrd_rdctl	(p7_mc_req_wrd_rdctl_o[63:0]),
      .mc7_rd_rq_stall	(mc7_rd_rq_stall_o),
      .mc7_wr_rq_stall	(mc7_wr_rq_stall_o),
      .r_ovrflow_alarm          (r_xbo_req_ovrflw_alarm[7:0]),
      .r_undflow_alarm          (r_xbo_req_undflw_alarm[7:0]),
      .clk			(clk),
      .clk_csr			(clk_csr),
      .i_reset			(i_reset)
      /*AUTOINST*/);

   pdk_rsp_mxb rsp_xbe (
      .mc0_rsp_stall		(mc0_rsp_stall_e),
      .p0_rsp_push		(p0_mc_rsp_push_e),
      .p0_rsp_rdctl		(p0_mc_rsp_rdctl_e[31:0]),
      .p0_rsp_data		(p0_mc_rsp_data_e[63:0]),
      .mc0_rsp_push		(mc0_rsp_push_e),
      .mc0_rsp_rdctl		(mc0_rsp_rdctl_e[31:0]),
      .mc0_rsp_data		(mc0_rsp_data_e[63:0]),
      .mc1_rsp_stall		(mc1_rsp_stall_e),
      .p1_rsp_push		(p1_mc_rsp_push_e),
      .p1_rsp_rdctl		(p1_mc_rsp_rdctl_e[31:0]),
      .p1_rsp_data		(p1_mc_rsp_data_e[63:0]),
      .mc1_rsp_push		(mc1_rsp_push_e),
      .mc1_rsp_rdctl		(mc1_rsp_rdctl_e[31:0]),
      .mc1_rsp_data		(mc1_rsp_data_e[63:0]),
      .mc2_rsp_stall		(mc2_rsp_stall_e),
      .p2_rsp_push		(p2_mc_rsp_push_e),
      .p2_rsp_rdctl		(p2_mc_rsp_rdctl_e[31:0]),
      .p2_rsp_data		(p2_mc_rsp_data_e[63:0]),
      .mc2_rsp_push		(mc2_rsp_push_e),
      .mc2_rsp_rdctl		(mc2_rsp_rdctl_e[31:0]),
      .mc2_rsp_data		(mc2_rsp_data_e[63:0]),
      .mc3_rsp_stall		(mc3_rsp_stall_e),
      .p3_rsp_push		(p3_mc_rsp_push_e),
      .p3_rsp_rdctl		(p3_mc_rsp_rdctl_e[31:0]),
      .p3_rsp_data		(p3_mc_rsp_data_e[63:0]),
      .mc3_rsp_push		(mc3_rsp_push_e),
      .mc3_rsp_rdctl		(mc3_rsp_rdctl_e[31:0]),
      .mc3_rsp_data		(mc3_rsp_data_e[63:0]),
      .mc4_rsp_stall		(mc4_rsp_stall_e),
      .p4_rsp_push		(p4_mc_rsp_push_e),
      .p4_rsp_rdctl		(p4_mc_rsp_rdctl_e[31:0]),
      .p4_rsp_data		(p4_mc_rsp_data_e[63:0]),
      .mc4_rsp_push		(mc4_rsp_push_e),
      .mc4_rsp_rdctl		(mc4_rsp_rdctl_e[31:0]),
      .mc4_rsp_data		(mc4_rsp_data_e[63:0]),
      .mc5_rsp_stall		(mc5_rsp_stall_e),
      .p5_rsp_push		(p5_mc_rsp_push_e),
      .p5_rsp_rdctl		(p5_mc_rsp_rdctl_e[31:0]),
      .p5_rsp_data		(p5_mc_rsp_data_e[63:0]),
      .mc5_rsp_push		(mc5_rsp_push_e),
      .mc5_rsp_rdctl		(mc5_rsp_rdctl_e[31:0]),
      .mc5_rsp_data		(mc5_rsp_data_e[63:0]),
      .mc6_rsp_stall		(mc6_rsp_stall_e),
      .p6_rsp_push		(p6_mc_rsp_push_e),
      .p6_rsp_rdctl		(p6_mc_rsp_rdctl_e[31:0]),
      .p6_rsp_data		(p6_mc_rsp_data_e[63:0]),
      .mc6_rsp_push		(mc6_rsp_push_e),
      .mc6_rsp_rdctl		(mc6_rsp_rdctl_e[31:0]),
      .mc6_rsp_data		(mc6_rsp_data_e[63:0]),
      .mc7_rsp_stall		(mc7_rsp_stall_e),
      .p7_rsp_push		(p7_mc_rsp_push_e),
      .p7_rsp_rdctl		(p7_mc_rsp_rdctl_e[31:0]),
      .p7_rsp_data		(p7_mc_rsp_data_e[63:0]),
      .mc7_rsp_push		(mc7_rsp_push_e),
      .mc7_rsp_rdctl		(mc7_rsp_rdctl_e[31:0]),
      .mc7_rsp_data		(mc7_rsp_data_e[63:0]),
      .r_ovrflow_alarm          (r_xbe_rsp_ovrflw_alarm[7:0]),
      .r_undflow_alarm          (r_xbe_rsp_undflw_alarm[7:0]),
      .clk			(clk),
      .clk_csr			(clk_csr),
      .i_reset			(i_reset)
      /*AUTOINST*/);

   pdk_rsp_mxb rsp_xbo (
      .mc0_rsp_stall		(mc0_rsp_stall_o),
      .p0_rsp_push		(p0_mc_rsp_push_o),
      .p0_rsp_rdctl		(p0_mc_rsp_rdctl_o[31:0]),
      .p0_rsp_data		(p0_mc_rsp_data_o[63:0]),
      .mc0_rsp_push		(mc0_rsp_push_o),
      .mc0_rsp_rdctl		(mc0_rsp_rdctl_o[31:0]),
      .mc0_rsp_data		(mc0_rsp_data_o[63:0]),
      .mc1_rsp_stall		(mc1_rsp_stall_o),
      .p1_rsp_push		(p1_mc_rsp_push_o),
      .p1_rsp_rdctl		(p1_mc_rsp_rdctl_o[31:0]),
      .p1_rsp_data		(p1_mc_rsp_data_o[63:0]),
      .mc1_rsp_push		(mc1_rsp_push_o),
      .mc1_rsp_rdctl		(mc1_rsp_rdctl_o[31:0]),
      .mc1_rsp_data		(mc1_rsp_data_o[63:0]),
      .mc2_rsp_stall		(mc2_rsp_stall_o),
      .p2_rsp_push		(p2_mc_rsp_push_o),
      .p2_rsp_rdctl		(p2_mc_rsp_rdctl_o[31:0]),
      .p2_rsp_data		(p2_mc_rsp_data_o[63:0]),
      .mc2_rsp_push		(mc2_rsp_push_o),
      .mc2_rsp_rdctl		(mc2_rsp_rdctl_o[31:0]),
      .mc2_rsp_data		(mc2_rsp_data_o[63:0]),
      .mc3_rsp_stall		(mc3_rsp_stall_o),
      .p3_rsp_push		(p3_mc_rsp_push_o),
      .p3_rsp_rdctl		(p3_mc_rsp_rdctl_o[31:0]),
      .p3_rsp_data		(p3_mc_rsp_data_o[63:0]),
      .mc3_rsp_push		(mc3_rsp_push_o),
      .mc3_rsp_rdctl		(mc3_rsp_rdctl_o[31:0]),
      .mc3_rsp_data		(mc3_rsp_data_o[63:0]),
      .mc4_rsp_stall		(mc4_rsp_stall_o),
      .p4_rsp_push		(p4_mc_rsp_push_o),
      .p4_rsp_rdctl		(p4_mc_rsp_rdctl_o[31:0]),
      .p4_rsp_data		(p4_mc_rsp_data_o[63:0]),
      .mc4_rsp_push		(mc4_rsp_push_o),
      .mc4_rsp_rdctl		(mc4_rsp_rdctl_o[31:0]),
      .mc4_rsp_data		(mc4_rsp_data_o[63:0]),
      .mc5_rsp_stall		(mc5_rsp_stall_o),
      .p5_rsp_push		(p5_mc_rsp_push_o),
      .p5_rsp_rdctl		(p5_mc_rsp_rdctl_o[31:0]),
      .p5_rsp_data		(p5_mc_rsp_data_o[63:0]),
      .mc5_rsp_push		(mc5_rsp_push_o),
      .mc5_rsp_rdctl		(mc5_rsp_rdctl_o[31:0]),
      .mc5_rsp_data		(mc5_rsp_data_o[63:0]),
      .mc6_rsp_stall		(mc6_rsp_stall_o),
      .p6_rsp_push		(p6_mc_rsp_push_o),
      .p6_rsp_rdctl		(p6_mc_rsp_rdctl_o[31:0]),
      .p6_rsp_data		(p6_mc_rsp_data_o[63:0]),
      .mc6_rsp_push		(mc6_rsp_push_o),
      .mc6_rsp_rdctl		(mc6_rsp_rdctl_o[31:0]),
      .mc6_rsp_data		(mc6_rsp_data_o[63:0]),
      .mc7_rsp_stall		(mc7_rsp_stall_o),
      .p7_rsp_push		(p7_mc_rsp_push_o),
      .p7_rsp_rdctl		(p7_mc_rsp_rdctl_o[31:0]),
      .p7_rsp_data		(p7_mc_rsp_data_o[63:0]),
      .mc7_rsp_push		(mc7_rsp_push_o),
      .mc7_rsp_rdctl		(mc7_rsp_rdctl_o[31:0]),
      .mc7_rsp_data		(mc7_rsp_data_o[63:0]),
      .r_ovrflow_alarm          (r_xbo_rsp_ovrflw_alarm[7:0]),
      .r_undflow_alarm          (r_xbo_rsp_undflw_alarm[7:0]),
      .clk			(clk),
      .clk_csr			(clk_csr),
      .i_reset			(i_reset)
   );

   /* ----------            registers         ---------- */

   always @(posedge clk) begin
    r_xbar_idle <= c_xbar_idle;
    r_cae_idle <= c_cae_idle;
   end

   /* ---------- debug & synopsys off blocks  ---------- */
   
   // synopsys translate_off

   // Parameters: 1-Severity: Don't Stop, 2-start check only after negedge of reset
   //assert_never #(1, 2, "***ERROR ASSERT: unimplemented instruction cracked") a0 (.clk(clk), .reset_n(~reset), .test_expr(r_unimplemented_inst));

    // synopsys translate_on

endmodule // cae_pers
// leda FM_2_23 on Non driven output ports or signals
