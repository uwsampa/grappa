
.686
.XMM
.model flat, c
 
PUBLIC Do_Xsave
PUBLIC Do_Fxsave
PUBLIC Do_Xrstor
PUBLIC Do_Fxrstor


.code
Do_Xsave PROC
    push   ebp
    mov    ebp, esp
    mov    ecx, [8+ebp]
    xor edx,edx
    mov eax,7
    xsave ymmword ptr[ecx]
    pop    ebp
    ret
Do_Xsave ENDP

.code
Do_Fxsave PROC
    push   ebp
    mov    ebp, esp
    mov    ecx, [8+ebp]
    fxsave xmmword ptr[ecx]
    pop    ebp
    ret
Do_Fxsave ENDP

.code
Do_Xrstor PROC
    push   ebp
    mov    ebp, esp
    mov    ecx, [8+ebp]
    xor edx,edx
    mov eax,7
    xrstor ymmword ptr[ecx]
    pop    ebp
    ret
Do_Xrstor ENDP

.code
Do_Fxrstor PROC
    push   ebp
    mov    ebp, esp
    mov    ecx, [8+ebp]
    fxrstor xmmword ptr[ecx]
    pop    ebp
    ret
Do_Fxrstor ENDP

end