.686
.XMM
.model flat, c

PUBLIC CopyViaYmmRegs

.code
CopyViaYmmRegs PROC
    push   ebp
    mov    ebp, esp
    mov eax,[ebp]+8
    mov ecx,[ebp]+12

    vmovdqu ymm0, ymmword ptr [eax]
    vmovdqu ymmword ptr [ecx], ymm0

    vmovdqu ymm1, ymmword ptr [eax]+32
    vmovdqu ymmword ptr [ecx]+32, ymm1

    vmovdqu ymm2, ymmword ptr [eax]+64
    vmovdqu ymmword ptr [ecx]+64, ymm2

    vmovdqu ymm3, ymmword ptr [eax]+96
    vmovdqu ymmword ptr [ecx]+96, ymm3

    vmovdqu ymm4, ymmword ptr [eax]+128
    vmovdqu ymmword ptr [ecx]+128, ymm4

    vmovdqu ymm5, ymmword ptr [eax]+160
    vmovdqu ymmword ptr [ecx]+160, ymm5

    vmovdqu ymm6, ymmword ptr [eax]+192
    vmovdqu ymmword ptr [ecx]+192, ymm6

    vmovdqu ymm7, ymmword ptr [eax]+224
    vmovdqu ymmword ptr [ecx]+224, ymm7
    
    vmovdqu ymm0, ymmword ptr [eax]+256
    vmovdqu ymmword ptr [ecx]+256, ymm0
    
    vmovdqu ymm1, ymmword ptr [eax]+288
    vmovdqu ymmword ptr [ecx]+288, ymm1
    
    vmovdqu ymm2, ymmword ptr [eax]+320
    vmovdqu ymmword ptr [ecx]+320, ymm2
    
    vmovdqu ymm3, ymmword ptr [eax]+352
    vmovdqu ymmword ptr [ecx]+352, ymm3
    
    vmovdqu ymm4, ymmword ptr [eax]+384
    vmovdqu ymmword ptr [ecx]+384, ymm4
    
    vmovdqu ymm5, ymmword ptr [eax]+416
    vmovdqu ymmword ptr [ecx]+416, ymm5
    
    vmovdqu ymm6, ymmword ptr [eax]+448
    vmovdqu ymmword ptr [ecx]+448, ymm6
    
    vmovdqu ymm7, ymmword ptr [eax]+480
    vmovdqu ymmword ptr [ecx]+480, ymm7
    pop    ebp
    ret
CopyViaYmmRegs ENDP

end