


PUBLIC SetYmmScratchesFun


.code
SetYmmScratchesFun PROC
    vmovdqu ymm0, ymmword ptr [rcx]
    vmovdqu ymm1, ymmword ptr [rcx]+32
    vmovdqu ymm2, ymmword ptr [rcx]+64
    vmovdqu ymm3, ymmword ptr [rcx]+96
    vmovdqu ymm4, ymmword ptr [rcx]+128
    vmovdqu ymm5, ymmword ptr [rcx]+160
    
    movdqu  xmmword ptr [rdx], xmm6
    vmovdqu ymm6, ymmword ptr [rcx]+192
    movdqu  xmm6, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm7
    vmovdqu ymm7, ymmword ptr [rcx]+224
    movdqu  xmm7, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm8
    vmovdqu ymm8, ymmword ptr [rcx]+256
    movdqu  xmm8, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm9
    vmovdqu ymm9, ymmword ptr [rcx]+288
    movdqu  xmm9, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm10
    vmovdqu ymm10, ymmword ptr [rcx]+320
    movdqu  xmm10, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm11
    vmovdqu ymm11, ymmword ptr [rcx]+352
    movdqu  xmm11, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm12
    vmovdqu ymm12, ymmword ptr [rcx]+384
    movdqu  xmm12, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm13
    vmovdqu ymm13, ymmword ptr [rcx]+416
    movdqu  xmm13, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm14
    vmovdqu ymm14, ymmword ptr [rcx]+448
    movdqu  xmm14, xmmword ptr [rdx]
    
    movdqu  xmmword ptr [rdx], xmm15
    vmovdqu ymm15, ymmword ptr [rcx]+480
    movdqu  xmm15, xmmword ptr [rdx]
    
    
    ret
SetYmmScratchesFun ENDP

end