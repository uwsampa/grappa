    .text
.globl DoILLOnBadStack
DoILLOnBadStack:
    movl    %esp, %eax
    movl    $0, %esp
    ud2
    movl    %eax, %esp
    ret

.globl DoSigreturnOnBadStack
DoSigreturnOnBadStack:
    push    %ebp
    movl    %esp, %ebp
    movl    $0, %esp
    movl    $119, %eax    /* __NR_sigreturn */
    int     $128
    movl    %ebp, %esp
    pop     %ebp
    ret
