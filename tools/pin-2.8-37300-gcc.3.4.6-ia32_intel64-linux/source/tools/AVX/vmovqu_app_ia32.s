.text
.globl CopyViaYmmRegs 
CopyViaYmmRegs:


    push      %ebp
    mov       %esp, %ebp
    mov       8(%ebp), %eax
    mov       12(%ebp), %ecx
    vmovdqu   (%eax), %ymm0
    vmovdqu   %ymm0, (%ecx)

    vmovdqu   32(%eax), %ymm1
    vmovdqu   %ymm1, 32(%ecx)

    vmovdqu   64(%eax), %ymm2
    vmovdqu   %ymm2, 64(%ecx)

    vmovdqu   96(%eax), %ymm3
    vmovdqu   %ymm3, 96(%ecx)

    vmovdqu   128(%eax), %ymm4
    vmovdqu   %ymm4, 128(%ecx)

    vmovdqu   160(%eax), %ymm5
    vmovdqu   %ymm5, 160(%ecx)

    vmovdqu   192(%eax), %ymm6
    vmovdqu   %ymm6, 192(%ecx)

    vmovdqu   224(%eax), %ymm7
    vmovdqu   %ymm7, 224(%ecx)
    
    vmovdqu   256(%eax), %ymm0
    vmovdqu   %ymm0, 256(%ecx)
    
    vmovdqu   288(%eax), %ymm1
    vmovdqu   %ymm1, 288(%ecx)
    
    vmovdqu   320(%eax), %ymm2
    vmovdqu   %ymm2, 320(%ecx)
    
    vmovdqu   352(%eax), %ymm3
    vmovdqu   %ymm3, 352(%ecx)
    
    vmovdqu   384(%eax), %ymm4
    vmovdqu   %ymm4, 384(%ecx)
    
    vmovdqu   416(%eax), %ymm5
    vmovdqu   %ymm5, 416(%ecx)
    
    vmovdqu   448(%eax), %ymm6
    vmovdqu   %ymm6, 448(%ecx)
    
    vmovdqu   480(%eax), %ymm7
    vmovdqu   %ymm7, 480(%ecx)
    pop       %ebp
    ret
