 
PUBLIC CopyViaYmmRegs

.code
CopyViaYmmRegs PROC

    vmovdqu ymm0, ymmword ptr [rcx]
    vmovdqu ymmword ptr [rdx], ymm0

    vmovdqu ymm1, ymmword ptr [rcx]+32
    vmovdqu ymmword ptr [rdx]+32, ymm1

    vmovdqu ymm2, ymmword ptr [rcx]+64
    vmovdqu ymmword ptr [rdx]+64, ymm2

    vmovdqu ymm3, ymmword ptr [rcx]+96
    vmovdqu ymmword ptr [rdx]+96, ymm3

    vmovdqu ymm4, ymmword ptr [rcx]+128
    vmovdqu ymmword ptr [rdx]+128, ymm4

    vmovdqu ymm5, ymmword ptr [rcx]+160
    vmovdqu ymmword ptr [rdx]+160, ymm5

    vmovdqu ymm6, ymmword ptr [rcx]+192
    vmovdqu ymmword ptr [rdx]+192, ymm6

    vmovdqu ymm7, ymmword ptr [rcx]+224
    vmovdqu ymmword ptr [rdx]+224, ymm7
    
    vmovdqu ymm8, ymmword ptr [rcx]+256
    vmovdqu ymmword ptr [rdx]+256, ymm8
    
    vmovdqu ymm9, ymmword ptr [rcx]+288
    vmovdqu ymmword ptr [rdx]+288, ymm9
    
    vmovdqu ymm10, ymmword ptr [rcx]+320
    vmovdqu ymmword ptr [rdx]+320, ymm10
    
    vmovdqu ymm11, ymmword ptr [rcx]+352
    vmovdqu ymmword ptr [rdx]+352, ymm11
    
    vmovdqu ymm12, ymmword ptr [rcx]+384
    vmovdqu ymmword ptr [rdx]+384, ymm12
    
    vmovdqu ymm13, ymmword ptr [rcx]+416
    vmovdqu ymmword ptr [rdx]+416, ymm13
    
    vmovdqu ymm14, ymmword ptr [rcx]+448
    vmovdqu ymmword ptr [rdx]+448, ymm14
    
    vmovdqu ymm15, ymmword ptr [rcx]+480
    vmovdqu ymmword ptr [rdx]+480, ymm15
    ret
CopyViaYmmRegs ENDP

end