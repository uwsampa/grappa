.686
.XMM
.model flat, c


PUBLIC SetYmmScratchesFun



.code
SetYmmScratchesFun PROC
    push   ebp
    mov    ebp, esp
    mov eax,[ebp]+8
    vmovdqu ymm0, ymmword ptr [eax]
    vmovdqu ymm1, ymmword ptr [eax]+32
    vmovdqu ymm2, ymmword ptr [eax]+64
    vmovdqu ymm3, ymmword ptr [eax]+96
    vmovdqu ymm4, ymmword ptr [eax]+128
    vmovdqu ymm5, ymmword ptr [eax]+160
    vmovdqu ymm6, ymmword ptr [eax]+192
    vmovdqu ymm7, ymmword ptr [eax]+224    
    pop    ebp
    ret
SetYmmScratchesFun ENDP

end