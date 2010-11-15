



.text
.globl SetYmmScratchesFun 
SetYmmScratchesFun: 
    vmovdqu   (%rdi), %ymm0
    vmovdqu   32(%rdi), %ymm1
    vmovdqu   64(%rdi), %ymm2
    vmovdqu   96(%rdi), %ymm3
    vmovdqu   128(%rdi), %ymm4
    vmovdqu   160(%rdi), %ymm5
    vmovdqu   192(%rdi), %ymm6
    vmovdqu   224(%rdi), %ymm7
    vmovdqu   256(%rdi), %ymm8
    vmovdqu   288(%rdi), %ymm9
    vmovdqu   320(%rdi), %ymm10
    vmovdqu   352(%rdi), %ymm11
    vmovdqu   384(%rdi), %ymm12
    vmovdqu   416(%rdi), %ymm13
    vmovdqu   448(%rdi), %ymm14
    vmovdqu   480(%rdi), %ymm15
    
    ret
