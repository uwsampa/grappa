
.text
.globl Do_Xsave 
Do_Xsave:
    push   %ebp
    mov    %esp, %ebp
    mov    8(%ebp), %ecx
    xor    %edx,%edx
    mov    7, %eax,
    xsave  (%ecx)
    pop    %ebp
    ret


.globl Do_Fxsave 
Do_Fxsave:
    push   %ebp
    mov    %esp, %ebp
    mov    8(%ebp), %ecx
    fxsave (%ecx)
    pop    %ebp
    ret
    
    
.globl Do_Fxrstor 
Do_Fxrstor:
    push   %ebp
    mov    %esp, %ebp
    mov    8(%ebp), %ecx
    fxrstor (%ecx)
    pop    %ebp
    ret
    
    
.globl Do_Fxrstor 
Do_Xrstor:
    push   %ebp
    mov    %esp, %ebp
    mov    8(%ebp), %ecx
    xor    %edx,%edx
    mov    7, %eax,
    xrstor (%ecx)
    pop    %ebp
    ret
