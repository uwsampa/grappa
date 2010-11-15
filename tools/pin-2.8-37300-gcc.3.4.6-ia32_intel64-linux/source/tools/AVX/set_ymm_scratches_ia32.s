.text
.globl SetYmmScratchesFun
SetYmmScratchesFun:
    push      %ebp
    mov       %esp, %ebp
    mov       8(%ebp), %eax
    vmovdqu   (%eax), %ymm0
    vmovdqu   32(%eax), %ymm1
    vmovdqu   64(%eax), %ymm2
    vmovdqu   96(%eax), %ymm3
    vmovdqu   128(%eax), %ymm4
    vmovdqu   160(%eax), %ymm5
    vmovdqu   192(%eax), %ymm6
    vmovdqu   224(%eax), %ymm7
    pop       %ebp
    ret
