
	.file	"walk.s"
	.ctext

	.globl	walk
	.type	walk. @function
	.signature	4
walk:
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


cpTest1End:	
	.globl	cpTest1End
	.type	cpTest1End. @function

	.cend
	
	
