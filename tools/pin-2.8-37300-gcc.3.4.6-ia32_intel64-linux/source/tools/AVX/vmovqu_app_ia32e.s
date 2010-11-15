 
.text
.globl CopyViaYmmRegs 
CopyViaYmmRegs:
    vmovdqu  (%rdi), %ymm0
    vmovdqu %ymm0,  (%rsi)

    vmovdqu  32(%rdi), %ymm1
    vmovdqu %ymm1,  32(%rsi)

    vmovdqu   64(%rdi), %ymm2
    vmovdqu  %ymm2,  64(%rsi)

    vmovdqu   96(%rdi), %ymm3
    vmovdqu  %ymm3,  96(%rsi)

    vmovdqu   128(%rdi), %ymm4
    vmovdqu  %ymm4,  128(%rsi)

    vmovdqu   160(%rdi), %ymm5
    vmovdqu  %ymm5,  160(%rsi)

    vmovdqu   192(%rdi), %ymm6
    vmovdqu  %ymm6,  192(%rsi)

    vmovdqu   224(%rdi), %ymm7
    vmovdqu  %ymm7,  224(%rsi)
    
    vmovdqu   256(%rdi), %ymm8
    vmovdqu  %ymm8,  256(%rsi)
    
    vmovdqu   288(%rdi), %ymm9
    vmovdqu  %ymm9,  288(%rsi)
    
    vmovdqu   320(%rdi), %ymm10
    vmovdqu  %ymm10,  320(%rsi)
    
    vmovdqu   352(%rdi), %ymm11
    vmovdqu  %ymm11,  352(%rsi)
    
    vmovdqu   384(%rdi), %ymm12
    vmovdqu  %ymm12,  384(%rsi)
    
    vmovdqu   416(%rdi), %ymm13
    vmovdqu  %ymm13,  416(%rsi)
    
    vmovdqu   448(%rdi), %ymm14
    vmovdqu  %ymm14,  448(%rsi)
    
    vmovdqu   480(%rdi), %ymm15
    vmovdqu  %ymm15,  480(%rsi)
    ret
