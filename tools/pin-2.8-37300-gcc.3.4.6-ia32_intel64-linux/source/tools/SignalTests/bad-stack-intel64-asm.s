    .text
.globl DoILLOnBadStack
DoILLOnBadStack:
    movq    %rsp, %rax
    movq    $0, %rsp
    ud2
    movq    %rax, %rsp
    ret

.globl DoSigreturnOnBadStack
DoSigreturnOnBadStack:
    push    %rbp
    movq    %rsp, %rbp
    movq    $0, %rsp
#if defined(TARGET_LINUX)
    movq    $15, %rax    /* __NR_rt_sigreturn */
#elif defined(TARGET_BSD)
    movq    $417, %rax    /* SYS_sigreturn */
#else
#error "Code not defined"
#endif
    syscall
    movq    %rbp, %rsp
    pop     %rbp
    ret
