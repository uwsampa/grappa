#define ASM

// Prototype of StackUnswitch
//    	 VOID* StackUnswitch(AFUNPTR fun, VOID* arg1, VOID* arg2, VOID* arg3, VOID* arg4, ADDRINT appSp)
//
				
.globl StackUnswitch
.globl _StackUnswitch
StackUnswitch:
_StackUnswitch:
#if defined(TARGET_IA32)
	//	             stack layout
	//		-------------------------------------------------------
	//			6th arg (appSp)	
	//			5th arg (arg4)
	//			4th arg (arg3)
	//			3rd arg (arg2)
	//			2nd arg (arg1)
	//			1st arg (fun)
	//			return address
	// ebp->		saved ebp
	
	push %ebp
	mov %esp, %ebp

	# Switch stacks
	cmp $0, 28(%ebp)
	cmovne 28(%ebp),%esp

	# Push arguments
	push 24(%ebp)
	push 20(%ebp)
	push 16(%ebp)
	push 12(%ebp)
	push 8(%ebp)

	# call dispatcher
	call MyDispatch

	# Restore stack will unswitch stack
	mov %ebp,%esp
	pop %ebp
	ret
#else
#if defined(TARGET_IA32E)
	//	             stack layout
	//		-------------------------------------------------------
	//			return address
	// rbp->		saved rbp

	push %rbp
	mov %rsp, %rbp

	# Switch stacks
	cmpq $0, %r9
	cmovne %r9, %rsp

	#arguments already in registers

	# call dispatcher
	call MyDispatch

	# Restore stack will unswitch stack
	mov %rbp, %rsp
	pop %rbp
	ret
#endif			
#endif
