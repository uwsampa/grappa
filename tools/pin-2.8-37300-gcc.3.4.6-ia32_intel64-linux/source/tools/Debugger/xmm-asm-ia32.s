/*NO LEGAL*/

    .data
    .align 16
xmmval:
    .quad   0x123456789abcdef0
    .quad   0xff00ff000a550a55

    .text
    .align 4
.globl DoXmm
DoXmm:
    movl        $1, %eax
    cvtsi2ss    %eax, %xmm0     /* %xmm0 = 1.0 (in lower 32-bits) */
    movl        $2, %eax
    cvtsi2ss    %eax, %xmm1     /* %xmm1 = 2.0 (in lower 32-bits) */
    lea         xmmval, %eax
    movdqa      (%eax), %xmm2
    ret
