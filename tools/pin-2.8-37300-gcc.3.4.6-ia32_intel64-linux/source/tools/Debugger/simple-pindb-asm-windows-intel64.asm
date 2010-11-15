PUBLIC Breakpoint
PUBLIC Breakpoint2
PUBLIC MemTestData
PUBLIC DoRegMemTest


.code
Breakpoint PROC
    nop
Breakpoint2::
    ret
Breakpoint ENDP


.data
MemTestData DWORD 012345678h, 0deadbeefh


.code
DoRegMemTest PROC
    lea     rax, MemTestData
    mov     ecx, DWORD PTR [rax]
    mov     DWORD PTR [rax], ecx
    ret
DoRegMemTest ENDP

END
