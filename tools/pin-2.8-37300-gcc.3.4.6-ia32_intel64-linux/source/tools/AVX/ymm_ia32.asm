.686
.XMM
.model flat, c

.code

set_ymm_reg0 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm0,  ymmword ptr [eax]
  RET
set_ymm_reg0 ENDP

get_ymm_reg0 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm0
  RET
get_ymm_reg0 ENDP

set_ymm_reg1 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm1,  ymmword ptr [eax]
  RET
set_ymm_reg1 ENDP

get_ymm_reg1 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm1
  RET
get_ymm_reg1 ENDP

set_ymm_reg2 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm2,  ymmword ptr [eax]
  RET
set_ymm_reg2 ENDP

get_ymm_reg2 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm2
  RET
get_ymm_reg2 ENDP

set_ymm_reg3 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm3,  ymmword ptr [eax]
  RET
set_ymm_reg3 ENDP

get_ymm_reg3 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm3
  RET
get_ymm_reg3 ENDP

set_ymm_reg4 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm4,  ymmword ptr [eax]
  RET
set_ymm_reg4 ENDP

get_ymm_reg4 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm4
  RET
get_ymm_reg4 ENDP

set_ymm_reg5 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm5,  ymmword ptr [eax]
  RET
set_ymm_reg5 ENDP

get_ymm_reg5 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm5
  RET
get_ymm_reg5 ENDP

set_ymm_reg6 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm6,  ymmword ptr [eax]
  RET
set_ymm_reg6 ENDP

get_ymm_reg6 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm6
  RET
get_ymm_reg6 ENDP

set_ymm_reg7 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymm7,  ymmword ptr [eax]
  RET
set_ymm_reg7 ENDP

get_ymm_reg7 PROC ymm_reg:DWORD
  mov    eax,   ymm_reg
  vmovdqu ymmword ptr [eax], ymm7
  RET
get_ymm_reg7 ENDP

end
