    .text
    .align 16
    .global NatCheck
    .proc NatCheck
NatCheck:
    alloc r32 = ar.pfs, 0, 1, 0, 0

    ld8.s r14 = [r0];;  // Force the NaT bit to be set

    // Delay for a while, so Pin can attach.
    //
    movl r10 = 3000000000
    mov r11 = r0;;
.L1:
    adds r11 = 1, r11;;
    cmp.geu p6,p7 = r10, r11;;
    (p6) br.cond.dptk .L1

    // Test whether the NaT bit is still set.  Return TRUE if it is, FALSE if not.
    //
    tnat.nz p6,p7 = r14;;
    (p7) mov r8 = r0
    (p6) mov r8 = 1

    mov ar.pfs = r32
    br.ret.sptk.many b0;;
    .endp NatCheck
