/*
 * This test verifies Pin's handling of two weird instructions:
 * "call *%rsp" and "jmp *%rsp".
 */

	.text
.global SetAppFlags_asm
.type SetAppFlags_asm, @function


SetAppFlags_asm:

    
    pushfq
    pop rax
    or rax, rcx
    push rax
    popfq
    leave
    ret


.global ClearAcFlag_asm
.type ClearAcFlag_asm, @function
ClearAcFlag_asm: 

    
    pushfq
    pop rcx
    and rcx, $0xfffbffff
    push rcx
    popfq
    ret

    .align 4
.globl GetFlags_asm
.type GetFlags_asm, @function
GetFlags_asm:
    pushfq
    pop rax
    ret
