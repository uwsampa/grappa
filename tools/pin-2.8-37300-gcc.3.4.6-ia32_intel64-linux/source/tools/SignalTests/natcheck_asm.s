    .text
    .align 16
    .global DoNatTest
    .proc DoNatTest
DoNatTest:
    alloc r32 = ar.pfs, 0, 25, 0, 0

    // Save the "preserved" registers.
    //
    mov r33 = r4
    mov r34 = r5
    mov r35 = r6
    mov r36 = r7
    mov r37 = r14
    mov r38 = r15
    mov r39 = r16
    mov r40 = r17
    mov r41 = r18
    mov r42 = r19
    mov r43 = r20
    mov r44 = r21
    mov r45 = r22
    mov r46 = r23
    mov r47 = r24
    mov r48 = r25
    mov r49 = r26
    mov r50 = r27
    mov r51 = r28
    mov r52 = r29
    mov r53 = r30
    mov r54 = r31;;

    // Clear the NaT bits for all registers (except special registers r1 (gp),
    // r12 (sp), and r13 (tp).
    //
    mov r2 = r0
    mov r3 = r0
    mov r4 = r0
    mov r5 = r0
    mov r6 = r0
    mov r7 = r0
    mov r8 = r0
    mov r9 = r0
    mov r10 = r0
    mov r11 = r0
    mov r14 = r0
    mov r15 = r0
    mov r16 = r0
    mov r17 = r0
    mov r18 = r0
    mov r19 = r0
    mov r20 = r0
    mov r21 = r0
    mov r22 = r0
    mov r23 = r0
    mov r24 = r0
    mov r25 = r0
    mov r26 = r0
    mov r27 = r0
    mov r28 = r0
    mov r29 = r0
    mov r30 = r0
    mov r31 = r0;;

    // Set the NaT bits in a couple registers.
    //
    ld8.s r7 = [r0]     // This is the Pin spill pointer
    ld8.s r9 = [r0]     // A scratch register

    // Wait for the signal handler to execute.
    //
    addl r2 = @ltoff(SignalFlag#), r1;;
    ld8 r3 = [r2];;
.L1:
    ld4 r2 = [r3];;
    cmp4.eq p6,p7 = 0, r2
    (p6) br.cond.dptk .L1

    // Compute a bit mask of all the registers whose NaT bit is set.
    //
    mov r56 = 1
    mov r55 = 0;;

    tnat.nz p6,p7 = r1
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r2
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r3
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r4
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r5
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r6
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r7
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r8
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r9
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r10
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r11
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r12
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r13
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r14
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r15
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r16
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r17
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r18
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r19
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r20
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r21
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r22
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r23
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r24
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r25
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r26
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r27
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r28
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r29
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r30
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    tnat.nz p6,p7 = r31
    shl r56 = r56, 1;;
    (p6) or r55 = r55, r56;;

    mov r8 = r55    // Return the bit mask

    // Restore all the "preserved" registers.
    //
    mov r4 = r33
    mov r5 = r34
    mov r6 = r35
    mov r7 = r36
    mov r14 = r37
    mov r15 = r38
    mov r16 = r39
    mov r17 = r40
    mov r18 = r41
    mov r19 = r42
    mov r20 = r43
    mov r21 = r44
    mov r22 = r45
    mov r23 = r46
    mov r24 = r47
    mov r25 = r48
    mov r26 = r49
    mov r27 = r50
    mov r28 = r51
    mov r29 = r52
    mov r30 = r53
    mov r31 = r54

    mov ar.pfs = r32
    br.ret.sptk.many b0;;
    .endp DoNatTest
