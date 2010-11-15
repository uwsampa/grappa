    /*
     * This routine is modified by the main part of the application.
     */
    .data
    .align 16
    .global Call
    .proc Call
Call:
    alloc r34 = ar.pfs, 0, 3, 0, 0
    mov r33 = b0;;

    /*
     * The application overwrites this constant with the address of a real subroutine.
     */
    movl r32 = 0x12345678abcdef12;;
    mov b6 = r32;;
    br.call.sptk.many b0 = b6

    mov ar.pfs = r34
    mov b0 = r33
    br.ret.sptk.many b0;;
    .endp Call


    /*
     * A utility routine to flush the I-cache (required for SMC on ipf).
     */
    .text
    .align 16
    .global FlushICache
    .proc FlushICache
FlushICache:

#if __GNUC__ == 3 && __GNUC_MINOR__ == 2
    // The 3.2 assembler does not understand the fc.i instruction.
    // This 16-byte value is the encoding for the entire instruction
    // bundle, which includes the fc.i and the sync.i (and also a nop).
    //
    data16 0x20000000000066000000063040000018
#else
    fc.i r32
    sync.i
#endif

    srlz.i
    br.ret.sptk.many b0;;

    .endp FlushICache
