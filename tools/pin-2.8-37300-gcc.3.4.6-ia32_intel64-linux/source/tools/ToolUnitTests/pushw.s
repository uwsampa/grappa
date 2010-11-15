.global pushIW_
.type pushIW_, function
pushIW_:      
	mov     %esp, %ebx      # Save esp
	mov     4(%esp),%esp    # stack to play with
	pushw   $-5             # Do the op
	mov     %esp, %eax      # Result
	mov     %ebx, %esp      # Stack with return address
	ret
