
/* for now, only support amd64 */
/* code stolen from:
 http://charm.cs.uiuc.edu/papers/migThreads.www/node24.html#fig:fastswitch
        */

.global swapstacks
swapstacks:
        push %rdi
        push %rbp
        push %rbx
        push %r12
        push %r13
        push %r14
        push %r15
        mov %rsp, (%rdi)
        mov (%rsi), %rsp
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %rbx
        pop %rbp
        pop %rdi
        mov %rdx, %rax
        ret

.global makestack
makestack:
        /* save the needed arguments */
        mov %rdx, %r12 /* <f> */
        mov %rcx, %r13 /* <me> */
        /* "save" the current stack so it can be popped off in swapstacks */
        push %rdi
        push %rbp
        push %rbx
        push %r12
        push %r13
        push %r14
        push %r15
        mov %rsp, (%rdi)
        mov (%rsi), %rsp
        mov %rdi, %rax
        mov %rsi, %rdi
        mov %rax, %rsi
        /* OK, assuming I did this right, here's how it works:
        NEW stack is in %rsp.  We call swapstacks and push a return address
        of our trampoline into f.
        Swapstacks saves a bunch of reg, and switches to the ORIGINAL stack.
        It then pops the registers I manually saved, and returns.
	Return address should return to whoever called makestack.
        */
        call swapstacks
        /* only thing I can actually trust is the swapstack-pushed regs */
        mov %r13, %rdi
        mov %rax, %rsi
        call *%r12
        /* it is an error for a coro to return */
        hlt
