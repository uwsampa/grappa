/*****************************************************************************/
//
// Module          : cae_csr.v
// Revision        : $Revision: 1.4 $
// Last Modified On: $Date: 2010/10/11 22:56:50 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : Glen Edwards
// Created On      : Jun 24, 2009
//
//-----------------------------------------------------------------------------
//
// Description     : CAE csr block contains registers accessible by the 
//                   management processor
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2008 : Created by Convey Computer Corp. This model is the 
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: cae_csr.v,v 1.4 2010/10/11 22:56:50 gedwards Exp $ */

`timescale 1 ns / 1 ps

module cae_csr (
   // synthesis attribute keep_hierarchy cae_csr "true";

   input		clk_csr,
   input		i_csr_reset_n,

   input  [3:0]		ring_ctl_in,
   input  [15:0]	ring_data_in,

   output [3:0]		ring_ctl_out,
   output [15:0]	ring_data_out,

   //
   // CSRs
   //
   input  [63:0]         cae_csr_status,
   input  [63:0]         cae_csr_vis,
   input  [63:0]         cae_csr_sum,
   output [63:0]         cae_csr_scratch
   );

   /* ----------         include files        ---------- */

   parameter CAE_CSR_SEL	= 16'h8000;
   parameter CAE_CSR_ADR_MASK	= 16'h8000;

   parameter CAE_CSR_STATUS	= 16'h0001;
   parameter CAE_CSR_VIS	= 16'h0002;
   parameter CAE_CSR_SCRATCH	= 16'h0003;
   parameter CAE_CSR_SUM	= 16'h0004;

   /* ----------          wires & regs        ---------- */

   reg                  csr_reset;

   wire   [15:0]	func_address;
   wire			func_wr_vld;
   wire   [63:0] 	rig_func_wr_data;
   wire			func_rd_vld;
   reg    [63:0]	c_func_rd_data;
   reg                  c_func_ack;
   
   reg    [63:0]	c_csr_scratch;
   reg    [63:0]	r_csr_scratch;
   reg    [63:0]	c_csr_status;
   reg    [63:0]	r_csr_status;
   reg    [63:0]	c_csr_vis;
   reg    [63:0]	r_csr_vis;

   /* ----------      combinatorial blocks    ---------- */

   assign cae_csr_scratch = r_csr_scratch;

   // read data mux
   always @*
    begin
      c_func_ack = func_rd_vld;
      c_func_rd_data = 64'h0;
      c_csr_status = r_csr_status;
      c_csr_vis = r_csr_vis;
      c_csr_scratch = r_csr_scratch;
      casex(func_address)
       CAE_CSR_STATUS: begin
         c_func_rd_data = cae_csr_status;
         c_csr_status = func_wr_vld ? rig_func_wr_data : r_csr_status;
        end
       CAE_CSR_VIS: begin
         c_func_rd_data = cae_csr_vis;
         c_csr_vis = func_wr_vld ? rig_func_wr_data : r_csr_vis;
        end
       CAE_CSR_SCRATCH: begin
         c_func_rd_data = cae_csr_scratch;
         c_csr_scratch = func_wr_vld ? rig_func_wr_data : r_csr_scratch;
        end
       CAE_CSR_SUM: begin
         c_func_rd_data = cae_csr_sum;
        end
       default: begin
         c_func_rd_data = 64'h0;
        end
      endcase
    end

   /* ----------      external module calls   ---------- */

   defparam agent.ADDRESS_MASK = CAE_CSR_ADR_MASK;
   defparam agent.ADDRESS_SELECT = CAE_CSR_SEL;
   csr_agent agent (
     // Outputs
     .ring_ctl_out                    (ring_ctl_out[3:0]),
     .ring_data_out                   (ring_data_out[15:0]),
     .func_wr_valid                   (func_wr_vld),
     .func_rd_valid                   (func_rd_vld),
     .func_address                    (func_address[15:0]),
     .func_wr_data                    (rig_func_wr_data[63:0]),
     // Inputs
     .core_clk83                      (clk_csr),
     .reset_n                         (~csr_reset),
     .ring_ctl_in                     (ring_ctl_in[3:0]),
     .ring_data_in                    (ring_data_in[15:0]),
     .func_ack                        (c_func_ack),
     .func_rd_data                    (c_func_rd_data[63:0])
   );


   /* ----------            registers         ---------- */

   always @(posedge clk_csr)
    if (~i_csr_reset_n)
      csr_reset <= 1'b1;
    else
      csr_reset <= 1'b0;

   always @(posedge clk_csr) begin
    r_csr_scratch <= c_csr_scratch;
    r_csr_status <= c_csr_status;
    r_csr_vis <= c_csr_vis;
   end
    
   /* ---------- debug & synopsys off blocks  ---------- */

endmodule // cae_csr
