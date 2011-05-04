# Makefile $Header: /cvs/cae_vadd_mxb/testbench/Makefile,v 1.3 2010/11/02 19:43:52 gedwards Exp $

LIBRARIES += -y ../verilog
LIBRARIES += -v ../verilog/macro.v

# To compile additional verilog modules for simulation:
CNY_PDK_TB_USER_VLOG += tb_user.v
CNY_PDK_ADDON_TOP_MODULES += tb_user

# to disable wave dumps
# VERILOG_SIM_OPTIONS += +DUMPOFF
# VERILOG_SIM_OPTIONS += +DUMPSTART=15000

# To fix the seed for simulation:
# CNY_PDK_SIM_SEED = 11223344

####################################################################
# Include Convey Makefile Template
####################################################################
#include $(CNY_PDK)/$(CNY_PDK_REV)/lib/MakefileInclude.cnypdk
include ../Makefile.include

