.text
.globl set_ymm_reg0 
set_ymm_reg0:
  vmovdqu   (%rdi), %ymm0
  ret

.globl get_ymm_reg0 
get_ymm_reg0: 
  vmovdqu   %ymm0, (%rdi)
  ret

.globl set_ymm_reg1
set_ymm_reg1: 
  vmovdqu    (%rdi), %ymm1
  ret

.globl get_ymm_reg1
get_ymm_reg1: 
  vmovdqu   %ymm1, (%rdi)
  ret

.globl set_ymm_reg2
set_ymm_reg2: 
  vmovdqu    (%rdi), %ymm2
  ret

.globl get_ymm_reg2
get_ymm_reg2: 
  vmovdqu   %ymm2, (%rdi)
  ret

.globl set_ymm_reg3
set_ymm_reg3: 
  vmovdqu    (%rdi), %ymm3
  ret

.globl get_ymm_reg3
get_ymm_reg3: 
  vmovdqu   %ymm3, (%rdi)
  ret

.globl set_ymm_reg4
set_ymm_reg4: 
  vmovdqu    (%rdi), %ymm4
  ret

.globl get_ymm_reg4
get_ymm_reg4: 
  vmovdqu   %ymm4, (%rdi)
  ret

.globl set_ymm_reg5
set_ymm_reg5: 
  vmovdqu    (%rdi), %ymm5
  ret

.globl get_ymm_reg5
get_ymm_reg5: 
  vmovdqu   %ymm5, (%rdi)
  ret

.globl set_ymm_reg6
set_ymm_reg6: 
  vmovdqu    (%rdi), %ymm6
  ret

.globl get_ymm_reg6
get_ymm_reg6: 
  vmovdqu   %ymm6, (%rdi)
  ret

.globl set_ymm_reg7
set_ymm_reg7: 
  vmovdqu    (%rdi), %ymm7
  ret

.globl get_ymm_reg7
get_ymm_reg7: 
  vmovdqu   %ymm7, (%rdi)
  ret

.globl set_ymm_reg8
set_ymm_reg8: 
  vmovdqu    (%rdi), %ymm8
  ret

.globl get_ymm_reg8
get_ymm_reg8: 
  vmovdqu   %ymm8, (%rdi)
  ret

.globl set_ymm_reg9
set_ymm_reg9: 
  vmovdqu    (%rdi), %ymm9
  ret

.globl get_ymm_reg9
get_ymm_reg9: 
  vmovdqu   %ymm9, (%rdi)
  ret

.globl set_ymm_reg10
set_ymm_reg10: 
  vmovdqu    (%rdi), %ymm10
  ret

.globl get_ymm_reg10
get_ymm_reg10: 
  vmovdqu   %ymm10, (%rdi)
  ret

.globl set_ymm_reg11
set_ymm_reg11: 
  vmovdqu    (%rdi), %ymm11
  ret

.globl get_ymm_reg11
get_ymm_reg11: 
  vmovdqu   %ymm11, (%rdi)
  ret

.globl set_ymm_reg12
set_ymm_reg12: 
  vmovdqu    (%rdi), %ymm12
  ret

.globl get_ymm_reg12
get_ymm_reg12: 
  vmovdqu   %ymm12, (%rdi)
  ret

.globl set_ymm_reg13
set_ymm_reg13: 
  vmovdqu    (%rdi), %ymm13
  ret

.globl get_ymm_reg13
get_ymm_reg13: 
  vmovdqu   %ymm13, (%rdi)
  ret

.globl set_ymm_reg14
set_ymm_reg14: 
  vmovdqu    (%rdi), %ymm14
  ret

.globl get_ymm_reg14
get_ymm_reg14: 
  vmovdqu   %ymm14, (%rdi)
  ret

.globl set_ymm_reg15
set_ymm_reg15: 
  vmovdqu    (%rdi), %ymm15
  ret

.globl get_ymm_reg15
get_ymm_reg15: 
  vmovdqu   %ymm15, (%rdi)
  ret
