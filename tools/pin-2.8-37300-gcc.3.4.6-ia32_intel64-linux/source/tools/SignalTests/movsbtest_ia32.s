/*
 * void CopyWithMovsb(void *src, void *dst, size_t size)
 */
.text
    .align 4
.globl CopyWithMovsb
CopyWithMovsb:
    movl    4(%esp), %esi   /* src */
    movl    8(%esp), %edi   /* dst */
    movl    12(%esp), %ecx  /* size */
    rep movsb
    ret
