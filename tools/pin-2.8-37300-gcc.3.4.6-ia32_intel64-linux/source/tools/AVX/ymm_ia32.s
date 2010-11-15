
.globl set_ymm_reg0 
.type set_ymm_reg0, @function
set_ymm_reg0:
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   (%ecx), %ymm0
  leave
  ret

.globl get_ymm_reg0
.type get_ymm_reg0, @function 
get_ymm_reg0: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm0, (%ecx)
  leave
  ret

.globl set_ymm_reg1
.type set_ymm_reg1, @function
set_ymm_reg1: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm1
  leave
  ret

.globl get_ymm_reg1
.type get_ymm_reg1, @function
get_ymm_reg1:
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm1, (%ecx)
  leave
  ret

.globl set_ymm_reg2
.type set_ymm_reg2, @function
set_ymm_reg2: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm2
  leave
  ret

.globl get_ymm_reg2
.type get_ymm_reg2, @function
get_ymm_reg2: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm2, (%ecx)
  leave
  ret

.globl set_ymm_reg3
.type set_ymm_reg3, @function
set_ymm_reg3: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm3
  leave
  ret

.globl get_ymm_reg3
.type get_ymm_reg3, @function
get_ymm_reg3: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm3, (%ecx)
  leave
  ret

.globl set_ymm_reg4
.type set_ymm_reg4, @function
set_ymm_reg4: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm4
  leave
  ret

.globl get_ymm_reg4
.type get_ymm_reg4, @function
get_ymm_reg4: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm4, (%ecx)
  leave
  ret

.globl set_ymm_reg5
.type set_ymm_reg5, @function
set_ymm_reg5: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm5
  leave
  ret

.globl get_ymm_reg5
.type get_ymm_reg5, @function
get_ymm_reg5: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm5, (%ecx)
  leave
  ret

.globl set_ymm_reg6
.type set_ymm_reg6, @function
set_ymm_reg6: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm6
  leave
  ret

.globl get_ymm_reg6
.type get_ymm_reg6, @function
get_ymm_reg6: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm6, (%ecx)
  leave
  ret

.globl set_ymm_reg7
.type set_ymm_reg7, @function
set_ymm_reg7: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu    (%ecx), %ymm7
  leave
  ret

.globl get_ymm_reg7
.type get_ymm_reg7, @function
get_ymm_reg7: 
  push %ebp
  mov %esp, %ebp
  mov 8(%ebp), %ecx
  vmovdqu   %ymm7, (%ecx)
  leave
  ret

