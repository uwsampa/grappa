.code



set_ymm_reg0 PROC ymm_reg:QWORD
  vmovdqu ymm0,  ymmword ptr [rcx]
  RET
set_ymm_reg0 ENDP

get_ymm_reg0 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm0
  RET
get_ymm_reg0 ENDP


set_ymm_reg1 PROC ymm_reg:QWORD
  vmovdqu ymm1,  ymmword ptr [rcx]
  RET
set_ymm_reg1 ENDP

get_ymm_reg1 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm1
  RET
get_ymm_reg1 ENDP

set_ymm_reg2 PROC ymm_reg:QWORD
  vmovdqu ymm2,  ymmword ptr [rcx]
  RET
set_ymm_reg2 ENDP

get_ymm_reg2 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm2
  RET
get_ymm_reg2 ENDP

set_ymm_reg3 PROC ymm_reg:QWORD
  vmovdqu ymm3,  ymmword ptr [rcx]
  RET
set_ymm_reg3 ENDP

get_ymm_reg3 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm3
  RET
get_ymm_reg3 ENDP

set_ymm_reg4 PROC ymm_reg:QWORD
  vmovdqu ymm4,  ymmword ptr [rcx]
  RET
set_ymm_reg4 ENDP

get_ymm_reg4 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm4
  RET
get_ymm_reg4 ENDP

set_ymm_reg5 PROC ymm_reg:QWORD
  vmovdqu ymm5,  ymmword ptr [rcx]
  RET
set_ymm_reg5 ENDP

get_ymm_reg5 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm5
  RET
get_ymm_reg5 ENDP

set_ymm_reg6 PROC ymm_reg:QWORD
  vmovdqu ymm6,  ymmword ptr [rcx]
  RET
set_ymm_reg6 ENDP

get_ymm_reg6 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm6
  RET
get_ymm_reg6 ENDP

set_ymm_reg7 PROC ymm_reg:QWORD
  vmovdqu ymm7,  ymmword ptr [rcx]
  RET
set_ymm_reg7 ENDP

get_ymm_reg7 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm7
  RET
get_ymm_reg7 ENDP

set_ymm_reg8 PROC ymm_reg:QWORD
  vmovdqu ymm8,  ymmword ptr [rcx]
  RET
set_ymm_reg8 ENDP

get_ymm_reg8 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm8
  RET
get_ymm_reg8 ENDP

set_ymm_reg9 PROC ymm_reg:QWORD
  vmovdqu ymm9,  ymmword ptr [rcx]
  RET
set_ymm_reg9 ENDP

get_ymm_reg9 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm9
  RET
get_ymm_reg9 ENDP

set_ymm_reg10 PROC ymm_reg:QWORD
  vmovdqu ymm10,  ymmword ptr [rcx]
  RET
set_ymm_reg10 ENDP

get_ymm_reg10 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm10
  RET
get_ymm_reg10 ENDP

set_ymm_reg11 PROC ymm_reg:QWORD
  vmovdqu ymm11,  ymmword ptr [rcx]
  RET
set_ymm_reg11 ENDP

get_ymm_reg11 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm11
  RET
get_ymm_reg11 ENDP

set_ymm_reg12 PROC ymm_reg:QWORD
  vmovdqu ymm12,  ymmword ptr [rcx]
  RET
set_ymm_reg12 ENDP

get_ymm_reg12 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm12
  RET
get_ymm_reg12 ENDP

set_ymm_reg13 PROC ymm_reg:QWORD
  vmovdqu ymm13,  ymmword ptr [rcx]
  RET
set_ymm_reg13 ENDP

get_ymm_reg13 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm13
  RET
get_ymm_reg13 ENDP

set_ymm_reg14 PROC ymm_reg:QWORD
  vmovdqu ymm14,  ymmword ptr [rcx]
  RET
set_ymm_reg14 ENDP

get_ymm_reg14 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm14
  RET
get_ymm_reg14 ENDP

set_ymm_reg15 PROC ymm_reg:QWORD
  vmovdqu ymm15,  ymmword ptr [rcx]
  RET
set_ymm_reg15 ENDP

get_ymm_reg15 PROC ymm_reg:QWORD
  vmovdqu ymmword ptr [rcx], ymm15
  RET
get_ymm_reg15 ENDP

end
