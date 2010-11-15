.386
.XMM
.model flat, c

.code

mmemcpy_ia32 PROC 
push        esi
push        edi
mov         eax,dword ptr [esp+14h] 
mov         ecx,eax
shr         ecx,2                  
mov         eax,dword ptr [esp+14h]
mov         edi,dword ptr [esp+0Ch]  
mov         esi,dword ptr [esp+10h] 
cld
rep movs    dword ptr [edi],dword ptr [esi]
test        al,2
je          l1
movs        word ptr [edi],word ptr [esi]
l1:
test        al,1
je          l2
movs        byte ptr [edi],byte ptr [esi]
l2:
mov         eax, dword ptr [esp+18h] 
mov         dword ptr [eax],edi
mov         eax, dword ptr [esp+1Ch] 
mov         dword ptr [eax],esi
pop         edi
pop         esi
ret
mmemcpy_ia32 ENDP


memindex_ia32 PROC
mov         dword ptr [esp + 8],0
mov         edx,dword ptr [esp + 8]
mov         eax,dword ptr [esp+4]
mov         dword ptr [edx+eax],0
ret
memindex_ia32 ENDP 



end
