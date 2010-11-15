.text
    .align 4
.globl SetAppFlagsAndSegv_asm
SetAppFlagsAndSegv_asm:
    pushf
    pop %eax
    or $0xcd5, %eax
    push %eax
    popf
    mov  $7, %ecx
    mov  %eax, 0(%ecx)
    ret

    .align 4
.globl ClearAppFlagsAndSegv_asm
ClearAppFlagsAndSegv_asm:
    pushf
    pop %eax
    and $0xfffff000, %eax
    push %eax
    popf
    mov  $7, %ecx
    mov  %eax, 0(%ecx)
    ret
    
    .align 4
.globl GetFlags_asm
GetFlags_asm:
    pushf
    pop %eax
    ret
