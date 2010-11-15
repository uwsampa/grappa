
// void SetYmmRegs(const YMM_REG *v1, const YMM_REG *v2, const YMM_REG *v3);
// void GetYmmRegs(YMM_REG *v1, YMM_REG *v2, YMM_REG *v3);


.global SetYmmRegs
.type SetYmmRegs, @function
SetYmmRegs:
  vmovdqu (%rdi), %ymm1
  vmovdqu (%rsi), %ymm2
  vmovdqu (%rdx), %ymm3
  ret
 
.global GetYmmRegs
.type GetYmmRegs, @function
GetYmmRegs:
  vmovdqu %ymm1, (%rdi)
  vmovdqu %ymm2, (%rsi)
  vmovdqu %ymm3, (%rdx)
  ret
