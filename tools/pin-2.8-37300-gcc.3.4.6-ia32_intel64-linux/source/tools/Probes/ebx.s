# ebx is the spill pointer, verify that probes
# that overwrite code that modifies ebx are handled correctly
			
.globl ebxmod
.type ebxmod, function
ebxmod:
	push %ebx
	add $1,%eax
	add $1,%eax
	add $1,%eax
	add $1,%eax
	mov $1, %ebx
	pop %ebx
	ret

.globl ebxmodcall
.type ebxmodcall, function
ebxmodcall:
	push %ebx
	mov $2,%ebx
	call ebxmod
	mov %ebx,%eax
	pop %ebx
	ret
