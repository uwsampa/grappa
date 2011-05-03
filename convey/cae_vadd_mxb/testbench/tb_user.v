`timescale 1 ns / 1 ps

module tb_user();

  initial begin
     $wlfdumpvars(0, testbench);
     
    // Insert user code here, such as signal dumping
    // set CNY_PDK_TB_USER_VLOG variable in makefile
  end

endmodule
