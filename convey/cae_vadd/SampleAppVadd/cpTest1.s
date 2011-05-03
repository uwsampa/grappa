
	.file	"cpTest.s"
	.ctext

	.globl	cpTestEx1
	.type	cpTestEx1. @function
	.signature	4
cpTestEx1:
  # a8 contains address of a1
  # a9 contains address of a2
  # a10 contains address of a3
  # a11 contains length l

  mov %a8, $0, %aeg
  mov %a9, $1, %aeg
  mov %a10, $2, %aeg
  mov %a11, $3, %aeg
  caep00 $0
  mov.ae0 %aeg, $30, %a16
  mov.ae1 %aeg, $31, %a17
  mov.ae2 %aeg, $32, %a18
  mov.ae3 %aeg, $33, %a19
  st.uq %a16, $0(%a12)
  st.uq %a17, $8(%a12)
  st.uq %a18, $16(%a12)
  st.uq %a19, $24(%a12)
  mov.ae0 %aeg, $30, %a8
  rtn 

	.globl	cpTestEx2
	.type	cpTestEx2. @function
	.signature	4
cpTestEx2:
  # a8 contains address of a1
  # a9 contains address of a2
  # a10 contains address of a3
  # a11 contains length l

  add.uq %a8, $24, %a12
  add.uq %a8, $48, %a13
  add.uq %a8, $72, %a14
  mov.ae0 %a8, $0, %aeg
  mov.ae1 %a12, $0, %aeg
  mov.ae2 %a13, $0, %aeg
  mov.ae3 %a14, $0, %aeg

  add.uq %a9, $24, %a15
  add.uq %a9, $48, %a16
  add.uq %a9, $72, %a17
  mov.ae0 %a9, $1, %aeg
  mov.ae1 %a15, $1, %aeg
  mov.ae2 %a16, $1, %aeg
  mov.ae3 %a17, $1, %aeg

  add.uq %a10, $24, %a18
  add.uq %a10, $48, %a19
  add.uq %a10, $72, %a20
  mov.ae0 %a10, $2, %aeg
  mov.ae1 %a18, $2, %aeg
  mov.ae2 %a19, $2, %aeg
  mov.ae3 %a20, $2, %aeg

  div.uq %a11, $4, %a21
  mov %a21, $3, %aeg
  caep00 $0
  caep01 $0
  caep02 $0
  mov.ae0 %aeg, $40, %a8

	rtn

cpTest1End:	
	.globl	cpTest1End
	.type	cpTest1End. @function

	.cend
	
	
