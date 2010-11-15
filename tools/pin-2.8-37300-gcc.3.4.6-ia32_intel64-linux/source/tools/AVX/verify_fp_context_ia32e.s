.text
.globl Do_Xsave 
Do_Xsave:
    xor %rdx, %rdx
    mov $7, %rax
    xsave (%rdi)
    ret

.globl Do_Fxsave 
Do_Fxsave:
    fxsave (%rdi)
    ret
    
.globl Do_Xrstor
Do_Xrstor:
    xor %rdx, %rdx
    mov $7, %rax
    xrstor (%rdi)
    ret
    
.globl Do_Fxrstor
Do_Fxrstor:
    fxrstor (%rdi)
    ret
