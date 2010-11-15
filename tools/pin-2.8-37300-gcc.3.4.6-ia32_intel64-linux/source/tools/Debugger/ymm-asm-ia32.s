    .text
    .align 4
.globl LoadYmm0
LoadYmm0:
    mov     4(%esp), %ecx

    /*
     * This is "VMOVDQU (%ecx), %ymm0".  We directly specify the machine code,
     * so this test runs even when the compiler doesn't support AVX.
     */
    .byte   0xc5, 0xfe, 0x6f, 0x01

.globl LoadYmm0Breakpoint
LoadYmm0Breakpoint:         /* Debugger puts a breakpoint here */
    ret
