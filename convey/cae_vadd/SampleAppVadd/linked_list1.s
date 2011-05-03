
	.ctext

	.globl	cny_walk
	.type	cny_walk. @function
	.signature	4
cny_walk:
  # a8 contains base address
  # a9 contains thread count
  # a10 contains edge count
  mov $1, %aeem
  mov %a8, $0, %aeg
  mov %a9, $1, %aeg
  mov %a10, $2, %aeg
  mov %a11, $3, %aeg
  caep00 $0
  rtn 

	.cend
