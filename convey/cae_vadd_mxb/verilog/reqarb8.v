/*****************************************************************************/
//
// Module          : reqarb8.vpp
// Revision        : $Revision: 1.4 $
// Last Modified On: $Date: 2010/10/25 22:47:55 $
// Last Modified By: $Author: gedwards $
//
//-----------------------------------------------------------------------------
//
// Original Author : mbarr
// Created On      : Fri Apr 30 17:01:24 CDT 2010
//
//-----------------------------------------------------------------------------
//
// Description     : MC request 8-way round robin arbiter with advance control
//
// This arbiter leaves grant asserted unless stalled, even if no request is
// asserted. A data transfer occurs whenever request (!empty) and grant are
// both true for any requestor. This eliminates re-arbitration when a single
// requestor is active, allowing full rate operation
//
// Push and select outputs are provided to control an external data path. The
// assumption is that requestor data is valid along with request (!empty) so
// the external data path can be implemented as:
//
//     data0 ---|\
//     data1 ---|  \
//     data2 ---|    \     +---+
//     data3 ---| 8:1 |----|D Q|--- r_data
//     data4 ---|     |    |   |
//     data5 ---|    /     |>  |
//     data6 ---|  /       +---+
//     data7 ---|/ |
//                 |
//    o_gsel ------+
//
// The arbiter's push output is valid on the same cycle as r_data.
// The i_lastreq input can be used to control arbiter operation. The normal
// mode of operation, simple round-robin, implies a crossbar designed to
// avoid head-aligned blocking problems associated with simple, lightweight
// crossbar designs.
//
// The i_lastreqN inputs provide some control over when the arbiter advances
// to the next requestor. This is intended to mitigate head-aligned blocking
// issues, allowing a lightweight crossbar design to be used with requestors
// that tend to issue relatively long streams of sequential requests.
//
// There are three ways to control arbiter operation using i_lastreqN:
//
// 1. Normal Mode: all i_lastreqN=1
//
//    With i_lastreq tied high, the arbiter follows an ordinary round-robin
//    sequence, advancing to a different reuestor each cycle when multipe
//    requestors are active.
//
// 2. Simple Sequential Mode: all i_lastreqN=0
//
//    With i_lastreq tied low, the arbiter attempts to issue 8 consecutive
//    grants to a particular reqestor before advancing to the next requestor.
//
//    In this mode, if a requestor has fewer than 8 consecutive requests, an
//    idle cycle (no grant asserted, no push) occurs when switching to the
//    next requestor.
// 
//    If requestors tend to issue streams of sequential quadword requests
//    spanning multiple cache lines, the idle overhead should be small. The
//    ideal requestors would always issue sequential quadword requests in
//    multiples of 8 that are always aligned on a cache lint boundary.
//
//    Short, random request streams could easily wind up with a significant
//    percentage of idle cycles, approaching 50% worst case.
//
// 3. Controlled Sequential Mode: requestor-generated i_lastreqN
//
//    Each requestor sends, along with virtual address and write data/read
//    control, a single control bit indicating this is the last request in
//    the cache line, and the next request will be to a different MC.
//   
//    The arbiter advances to the next requestor when the i_lastreqN input
//    the currently selected requestor is asserted, or when 8 consecutive
//    grants have been issued. The arbiter can avoid an idle cycle when
//    switching to the next requestor before 8 consecutive requests have
//    been issued.
//
//-----------------------------------------------------------------------------
//
// Copyright (c) 2007 : created by Convey Computer Corp. This model is the
// confidential and proprietary property of Convey Computer Corp.
//
/*****************************************************************************/
/* $Id: reqarb8.vpp,v 1.4 2010/10/25 22:47:55 gedwards Exp $ */

module reqarb8 (/*AUTOARG*/
   // Outputs
   o_grant0, o_grant1, o_grant2, o_grant3, o_grant4, o_grant5,
   o_grant6, o_grant7, o_gsel, o_push_ld, o_push_st,
   // Inputs
   clk, reset, i_empty0, i_empty1, i_empty2, i_empty3, i_empty4,
   i_empty5, i_empty6, i_empty7, i_lastreq0, i_lastreq1, i_lastreq2,
   i_lastreq3, i_lastreq4, i_lastreq5, i_lastreq6, i_lastreq7,
   i_ld_stall, i_st_stall, i_ld_st
   ) ;

   // synthesis attribute keep_hierarchy reqarb8 "true";

   /* ----------        port declarations     ---------- */
   input                   clk;
   input                   reset;

   input                   i_empty0;
   input                   i_empty1;
   input                   i_empty2;
   input                   i_empty3;
   input                   i_empty4;
   input                   i_empty5;
   input                   i_empty6;
   input                   i_empty7;

   input                   i_lastreq0;
   input                   i_lastreq1;
   input                   i_lastreq2;
   input                   i_lastreq3;
   input                   i_lastreq4;
   input                   i_lastreq5;
   input                   i_lastreq6;
   input                   i_lastreq7;

   input                   i_ld_stall;
   input                   i_st_stall;

   input                   i_ld_st;

   output                  o_grant0;
   output                  o_grant1;
   output                  o_grant2;
   output                  o_grant3;
   output                  o_grant4;
   output                  o_grant5;
   output                  o_grant6;
   output                  o_grant7;

   output [2:0]            o_gsel;
   output                  o_push_ld;
   output                  o_push_st;


   /* ----------         include files        ---------- */

   /* ----------          wires & regs        ---------- */

   wire                    c_grant0;
   wire                    c_grant1;
   wire                    c_grant2;
   wire                    c_grant3;
   wire                    c_grant4;
   wire                    c_grant5;
   wire                    c_grant6;
   wire                    c_grant7;
   reg  [2:0]              c_gsel;
   wire                    c_push;
   wire                    c_push_ld;
   wire                    c_push_st;

   wire                    r_grant0;
   wire                    r_grant1;
   wire                    r_grant2;
   wire                    r_grant3;
   wire                    r_grant4;
   wire                    r_grant5;
   wire                    r_grant6;
   wire                    r_grant7;
   wire [2:0]              r_gsel;
   wire                    r_push_ld;
   wire                    r_push_st;

   wire                    c_consec;
   reg                     c_gcmax0;
   reg                     c_gcmax1;
   reg                     c_gcmax2;
   reg                     c_gcmax3;
   reg                     c_gcmax4;
   reg                     c_gcmax5;
   reg                     c_gcmax6;
   reg                     c_gcmax7;
   wire [2:0]              c_gcnt;

   wire                    r_consec;
   wire [2:0]              r_gcnt;

   wire                    r_ld_stall;
   wire                    r_st_stall;

   /* ----------      combinatorial blocks    ---------- */

   // Output assignments.
   assign o_grant0 = r_grant0;
   assign o_grant1 = r_grant1;
   assign o_grant2 = r_grant2;
   assign o_grant3 = r_grant3;
   assign o_grant4 = r_grant4;
   assign o_grant5 = r_grant5;
   assign o_grant6 = r_grant6;
   assign o_grant7 = r_grant7;
   assign o_gsel = r_gsel;
   assign o_push_ld = r_push_ld;
   assign o_push_st = r_push_st;

   // Next grant per current grant and requests.
   always @* begin
     c_gsel = r_gsel;
     c_gcmax0 = i_lastreq0 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax1 = i_lastreq1 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax2 = i_lastreq2 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax3 = i_lastreq3 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax4 = i_lastreq4 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax5 = i_lastreq5 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax6 = i_lastreq6 | (r_gcnt == 3'b110); // 8 consec
     c_gcmax7 = i_lastreq7 | (r_gcnt == 3'b110); // 8 consec
     case (r_gsel)
     3'd0: begin
       casex ({c_gcmax0, i_empty1, i_empty2, i_empty3, i_empty4,
                            i_empty5, i_empty6, i_empty7, i_empty0})
       9'b1_0xxxxxxx: c_gsel = 3'd1;
       9'b1_10xxxxxx: c_gsel = 3'd2;
       9'b1_110xxxxx: c_gsel = 3'd3;
       9'b1_1110xxxx: c_gsel = 3'd4;
       9'b1_11110xxx: c_gsel = 3'd5;
       9'b1_111110xx: c_gsel = 3'd6;
       9'b1_1111110x: c_gsel = 3'd7;
       9'b1_11111110: c_gsel = 3'd0;
       9'b0_xxxxxxx0: c_gsel = 3'd0;
       9'b0_0xxxxxx1: c_gsel = 3'd1;
       9'b0_10xxxxx1: c_gsel = 3'd2;
       9'b0_110xxxx1: c_gsel = 3'd3;
       9'b0_1110xxx1: c_gsel = 3'd4;
       9'b0_11110xx1: c_gsel = 3'd5;
       9'b0_111110x1: c_gsel = 3'd6;
       9'b0_11111101: c_gsel = 3'd7;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd1: begin
       casex ({c_gcmax1, i_empty2, i_empty3, i_empty4, i_empty5,
                            i_empty6, i_empty7, i_empty0, i_empty1})
       9'b1_0xxxxxxx: c_gsel = 3'd2;
       9'b1_10xxxxxx: c_gsel = 3'd3;
       9'b1_110xxxxx: c_gsel = 3'd4;
       9'b1_1110xxxx: c_gsel = 3'd5;
       9'b1_11110xxx: c_gsel = 3'd6;
       9'b1_111110xx: c_gsel = 3'd7;
       9'b1_1111110x: c_gsel = 3'd0;
       9'b1_11111110: c_gsel = 3'd1;
       9'b0_xxxxxxx0: c_gsel = 3'd1;
       9'b0_0xxxxxx1: c_gsel = 3'd2;
       9'b0_10xxxxx1: c_gsel = 3'd3;
       9'b0_110xxxx1: c_gsel = 3'd4;
       9'b0_1110xxx1: c_gsel = 3'd5;
       9'b0_11110xx1: c_gsel = 3'd6;
       9'b0_111110x1: c_gsel = 3'd7;
       9'b0_11111101: c_gsel = 3'd0;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd2: begin
       casex ({c_gcmax2, i_empty3, i_empty4, i_empty5, i_empty6,
                            i_empty7, i_empty0, i_empty1, i_empty2})
       9'b1_0xxxxxxx: c_gsel = 3'd3;
       9'b1_10xxxxxx: c_gsel = 3'd4;
       9'b1_110xxxxx: c_gsel = 3'd5;
       9'b1_1110xxxx: c_gsel = 3'd6;
       9'b1_11110xxx: c_gsel = 3'd7;
       9'b1_111110xx: c_gsel = 3'd0;
       9'b1_1111110x: c_gsel = 3'd1;
       9'b1_11111110: c_gsel = 3'd2;
       9'b0_xxxxxxx0: c_gsel = 3'd2;
       9'b0_0xxxxxx1: c_gsel = 3'd3;
       9'b0_10xxxxx1: c_gsel = 3'd4;
       9'b0_110xxxx1: c_gsel = 3'd5;
       9'b0_1110xxx1: c_gsel = 3'd6;
       9'b0_11110xx1: c_gsel = 3'd7;
       9'b0_111110x1: c_gsel = 3'd0;
       9'b0_11111101: c_gsel = 3'd1;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd3: begin
       casex ({c_gcmax3, i_empty4, i_empty5, i_empty6, i_empty7,
                            i_empty0, i_empty1, i_empty2, i_empty3})
       9'b1_0xxxxxxx: c_gsel = 3'd4;
       9'b1_10xxxxxx: c_gsel = 3'd5;
       9'b1_110xxxxx: c_gsel = 3'd6;
       9'b1_1110xxxx: c_gsel = 3'd7;
       9'b1_11110xxx: c_gsel = 3'd0;
       9'b1_111110xx: c_gsel = 3'd1;
       9'b1_1111110x: c_gsel = 3'd2;
       9'b1_11111110: c_gsel = 3'd3;
       9'b0_xxxxxxx0: c_gsel = 3'd3;
       9'b0_0xxxxxx1: c_gsel = 3'd4;
       9'b0_10xxxxx1: c_gsel = 3'd5;
       9'b0_110xxxx1: c_gsel = 3'd6;
       9'b0_1110xxx1: c_gsel = 3'd7;
       9'b0_11110xx1: c_gsel = 3'd0;
       9'b0_111110x1: c_gsel = 3'd1;
       9'b0_11111101: c_gsel = 3'd2;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd4: begin
       casex ({c_gcmax4, i_empty5, i_empty6, i_empty7, i_empty0,
                            i_empty1, i_empty2, i_empty3, i_empty4})
       9'b1_0xxxxxxx: c_gsel = 3'd5;
       9'b1_10xxxxxx: c_gsel = 3'd6;
       9'b1_110xxxxx: c_gsel = 3'd7;
       9'b1_1110xxxx: c_gsel = 3'd0;
       9'b1_11110xxx: c_gsel = 3'd1;
       9'b1_111110xx: c_gsel = 3'd2;
       9'b1_1111110x: c_gsel = 3'd3;
       9'b1_11111110: c_gsel = 3'd4;
       9'b0_xxxxxxx0: c_gsel = 3'd4;
       9'b0_0xxxxxx1: c_gsel = 3'd5;
       9'b0_10xxxxx1: c_gsel = 3'd6;
       9'b0_110xxxx1: c_gsel = 3'd7;
       9'b0_1110xxx1: c_gsel = 3'd0;
       9'b0_11110xx1: c_gsel = 3'd1;
       9'b0_111110x1: c_gsel = 3'd2;
       9'b0_11111101: c_gsel = 3'd3;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd5: begin
       casex ({c_gcmax5, i_empty6, i_empty7, i_empty0, i_empty1,
                            i_empty2, i_empty3, i_empty4, i_empty5})
       9'b1_0xxxxxxx: c_gsel = 3'd6;
       9'b1_10xxxxxx: c_gsel = 3'd7;
       9'b1_110xxxxx: c_gsel = 3'd0;
       9'b1_1110xxxx: c_gsel = 3'd1;
       9'b1_11110xxx: c_gsel = 3'd2;
       9'b1_111110xx: c_gsel = 3'd3;
       9'b1_1111110x: c_gsel = 3'd4;
       9'b1_11111110: c_gsel = 3'd5;
       9'b0_xxxxxxx0: c_gsel = 3'd5;
       9'b0_0xxxxxx1: c_gsel = 3'd6;
       9'b0_10xxxxx1: c_gsel = 3'd7;
       9'b0_110xxxx1: c_gsel = 3'd0;
       9'b0_1110xxx1: c_gsel = 3'd1;
       9'b0_11110xx1: c_gsel = 3'd2;
       9'b0_111110x1: c_gsel = 3'd3;
       9'b0_11111101: c_gsel = 3'd4;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd6: begin
       casex ({c_gcmax6, i_empty7, i_empty0, i_empty1, i_empty2,
                            i_empty3, i_empty4, i_empty5, i_empty6})
       9'b1_0xxxxxxx: c_gsel = 3'd7;
       9'b1_10xxxxxx: c_gsel = 3'd0;
       9'b1_110xxxxx: c_gsel = 3'd1;
       9'b1_1110xxxx: c_gsel = 3'd2;
       9'b1_11110xxx: c_gsel = 3'd3;
       9'b1_111110xx: c_gsel = 3'd4;
       9'b1_1111110x: c_gsel = 3'd5;
       9'b1_11111110: c_gsel = 3'd6;
       9'b0_xxxxxxx0: c_gsel = 3'd6;
       9'b0_0xxxxxx1: c_gsel = 3'd7;
       9'b0_10xxxxx1: c_gsel = 3'd0;
       9'b0_110xxxx1: c_gsel = 3'd1;
       9'b0_1110xxx1: c_gsel = 3'd2;
       9'b0_11110xx1: c_gsel = 3'd3;
       9'b0_111110x1: c_gsel = 3'd4;
       9'b0_11111101: c_gsel = 3'd5;
       default:       c_gsel = r_gsel;
       endcase
     end
     3'd7: begin
       casex ({c_gcmax7, i_empty0, i_empty1, i_empty2, i_empty3,
                            i_empty4, i_empty5, i_empty6, i_empty7})
       9'b1_0xxxxxxx: c_gsel = 3'd0;
       9'b1_10xxxxxx: c_gsel = 3'd1;
       9'b1_110xxxxx: c_gsel = 3'd2;
       9'b1_1110xxxx: c_gsel = 3'd3;
       9'b1_11110xxx: c_gsel = 3'd4;
       9'b1_111110xx: c_gsel = 3'd5;
       9'b1_1111110x: c_gsel = 3'd6;
       9'b1_11111110: c_gsel = 3'd7;
       9'b0_xxxxxxx0: c_gsel = 3'd7;
       9'b0_0xxxxxx1: c_gsel = 3'd0;
       9'b0_10xxxxx1: c_gsel = 3'd1;
       9'b0_110xxxx1: c_gsel = 3'd2;
       9'b0_1110xxx1: c_gsel = 3'd3;
       9'b0_11110xx1: c_gsel = 3'd4;
       9'b0_111110x1: c_gsel = 3'd5;
       9'b0_11111101: c_gsel = 3'd6;
       default:       c_gsel = r_gsel;
       endcase
     end
     default: begin
       c_gsel = r_gsel;
     end
     endcase
   end

   // Any request and grant, advance.
   assign c_push = |(~{i_empty0, i_empty1, i_empty2, i_empty3,
                       i_empty4, i_empty5, i_empty6, i_empty7} &
                      {r_grant0, r_grant1, r_grant2, r_grant3,
                       r_grant4, r_grant5, r_grant6, r_grant7});

   // Load or store advance.
   assign c_push_ld = c_push & i_ld_st;
   assign c_push_st = c_push & ~i_ld_st;


   // Arbitration advance control.
   assign c_consec = (c_gsel == r_gsel);
   assign c_gcnt = (r_consec & c_push) ?
                   ((r_gcnt == 3'b111) ? r_gcnt : {r_gcnt + 3'd1}) : 3'd0;

   // Next grant qualified with stall.
   assign c_grant0 = (c_gsel == 3'd0) & ~(r_ld_stall | r_st_stall);
   assign c_grant1 = (c_gsel == 3'd1) & ~(r_ld_stall | r_st_stall);
   assign c_grant2 = (c_gsel == 3'd2) & ~(r_ld_stall | r_st_stall);
   assign c_grant3 = (c_gsel == 3'd3) & ~(r_ld_stall | r_st_stall);
   assign c_grant4 = (c_gsel == 3'd4) & ~(r_ld_stall | r_st_stall);
   assign c_grant5 = (c_gsel == 3'd5) & ~(r_ld_stall | r_st_stall);
   assign c_grant6 = (c_gsel == 3'd6) & ~(r_ld_stall | r_st_stall);
   assign c_grant7 = (c_gsel == 3'd7) & ~(r_ld_stall | r_st_stall);


   /* ----------            registers         ---------- */

   dffr_3 reg_gs ( .clk(clk), .reset(reset), .d(c_gsel), .q(r_gsel) );
   dff_1 reg_g0 ( .clk(clk), .d(c_grant0), .q(r_grant0) );
   dff_1 reg_g1 ( .clk(clk), .d(c_grant1), .q(r_grant1) );
   dff_1 reg_g2 ( .clk(clk), .d(c_grant2), .q(r_grant2) );
   dff_1 reg_g3 ( .clk(clk), .d(c_grant3), .q(r_grant3) );
   dff_1 reg_g4 ( .clk(clk), .d(c_grant4), .q(r_grant4) );
   dff_1 reg_g5 ( .clk(clk), .d(c_grant5), .q(r_grant5) );
   dff_1 reg_g6 ( .clk(clk), .d(c_grant6), .q(r_grant6) );
   dff_1 reg_g7 ( .clk(clk), .d(c_grant7), .q(r_grant7) );
   dff_1 reg_pushld ( .clk(clk), .d(c_push_ld), .q(r_push_ld) );
   dff_1 reg_pushst ( .clk(clk), .d(c_push_st), .q(r_push_st) );
   dff_1 reg_consec ( .clk(clk), .d(c_consec), .q(r_consec) );
   dff_3 reg_gcnt ( .clk(clk), .d(c_gcnt), .q(r_gcnt) );

   dff_1 reg_ld_stall ( .clk(clk), .d(i_ld_stall), .q(r_ld_stall) );
   dff_1 reg_st_stall ( .clk(clk), .d(i_st_stall), .q(r_st_stall) );
endmodule // reqarb8
