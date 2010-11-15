 
PUBLIC Do_Xsave
PUBLIC Do_Fxsave
PUBLIC Do_Xrstor
PUBLIC Do_Fxrstor


.code
Do_Xsave PROC
    xor rdx,rdx
    mov rax,7
    xsave ymmword ptr[rcx]
    ret
Do_Xsave ENDP

.code
Do_Fxsave PROC
    fxsave xmmword ptr[rcx]
    ret
Do_Fxsave ENDP

.code
Do_Xrstor PROC
    xor rdx,rdx
    mov rax,7
    xrstor ymmword ptr[rcx]
    ret
Do_Xrstor ENDP

.code
Do_Fxrstor PROC
    fxrstor xmmword ptr[rcx]
    ret
Do_Fxrstor ENDP

end