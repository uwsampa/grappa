// void SetYmmRegs(const YMM_REG *v1, const YMM_REG *v2, const YMM_REG *v3);
// void GetYmmRegs(YMM_REG *v1, YMM_REG *v2, YMM_REG *v3);

.global SetYmmRegs
.type SetYmmRegs, @function
SetYmmRegs:
  mov 0x4(%esp), %eax      
  vmovdqu (%eax), %ymm1
  mov 0x8(%esp), %eax      
          vmovdqu (%eax), %ymm2
  mov 0xc(%esp), %eax      
          vmovdqu (%eax), %ymm3
  ret
 
.global GetYmmRegs
.type GetYmmRegs, @function
GetYmmRegs:
  mov 0x4(%esp), %eax
        vmovdqu %ymm1, (%eax)
  mov 0x8(%esp), %eax
          vmovdqu %ymm2, (%eax)
  mov 0xc(%esp), %eax
          vmovdqu %ymm3, (%eax)
  ret

