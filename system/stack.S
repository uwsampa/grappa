
/* for now, only support amd64 */
/* inspired by:
 * http://charm.cs.uiuc.edu/papers/migThreads.www/node24.html#fig:fastswitch
 * and hacked up substantially
 */

// define this only when we're using the minimal save context switcher
// if you switch this off, you must update the swapstacks_inline settings in stack.h
//#define SAVE_REGISTERS LITE_SAVE_REGISTERS
//#define RESTORE_REGISTERS LITE_RESTORE_REGISTERS

// heavyweight context swtiching
// if you switch this off, you probably want to update the swapstacks_inline settings in stack.h
 #define SAVE_REGISTERS PUSHPOP_SAVE_REGISTERS
 #define RESTORE_REGISTERS PUSHPOP_RESTORE_REGISTERS

// save and restore callee-saved registers, including FP
#define PUSHPOP_SAVE_REGISTERS \
        push %rdi; \
        push %rbp; \
        push %rbx; \
        push %r12; \
        push %r13; \
        push %r14; \
        push %r15; \
        sub $8, %rsp; \
        stmxcsr 4(%rsp); \
        fnstcw (%rsp);
        
#define PUSHPOP_RESTORE_REGISTERS \
        fldcw (%rsp); \
        ldmxcsr 4(%rsp); \
        add $8, %rsp; \
        pop %r15; \
        pop %r14; \
        pop %r13; \
        pop %r12; \
        pop %rbx; \
        pop %rbp; \
        pop %rdi;



// save and restore callee-saved registers, without FP
#define PUSHPOP_NOFP_SAVE_REGISTERS \
        push %rdi; \
        push %rbp; \
        push %rbx; \
        push %r12; \
        push %r13; \
        push %r14; \
        push %r15;
        
#define PUSHPOP_NOFP_RESTORE_REGISTERS \
        pop %r15; \
        pop %r14; \
        pop %r13; \
        pop %r12; \
        pop %rbx; \
        pop %rbp; \
        pop %rdi;

// save and restore only register we can't tell the compiler to save
// fails with -fno-omit-frame-pointer
#define LITE_SAVE_REGISTERS \
        sub $4, %rsp; \
        stmxcsr 0(%rsp);
        
#define LITE_RESTORE_REGISTERS \
        ldmxcsr 0(%rsp); \
        add $4, %rsp;

// save and restore only register we can't tell the compiler to save        
#define LITE_FRAME_SAVE_REGISTERS \
        push %rbp; \
        sub $4, %rsp; \
        stmxcsr 0(%rsp);
        
#define LITE_FRAME_RESTORE_REGISTERS \
        ldmxcsr 0(%rsp); \
        add $4, %rsp; \
        pop %rbp;

// compiler saves and restores all registers
#define LITE_NOFP_SAVE_REGISTERS \
        ;
        
#define LITE_NOFP_RESTORE_REGISTERS \
        ;



// save and restore callee-saved registers, including FP        
#define BULK_SAVE_REGISTERS \
        sub $64, %rsp; \
        mov %rdi, 56(%rsp); \
        mov %rbp, 48(%rsp); \
        mov %rbx, 40(%rsp); \
        mov %r12, 32(%rsp); \
        mov %r13, 24(%rsp); \
        mov %r14, 16(%rsp); \
        mov %r15,  8(%rsp); \
        stmxcsr 4(%rsp); \
        fnstcw (%rsp);

#define BULK_RESTORE_REGISTERS \
        mov 56(%rsp), %rdi; \
        mov 48(%rsp), %rbp; \
        mov 40(%rsp), %rbx; \
        mov 32(%rsp), %r12; \
        mov 24(%rsp), %r13; \
        mov 16(%rsp), %r14; \
        mov  8(%rsp), %r15; \
        ldmxcsr 4(%rsp); \
        fldcw (%rsp); \
        add $64, %rsp;

// save and restore callee-saved registers, without FP
#define BULK_NOFP_SAVE_REGISTERS \
        sub $56, %rsp; \
        mov %rdi, 48(%rsp); \
        mov %rbp, 40(%rsp); \
        mov %rbx, 32(%rsp); \
        mov %r12, 24(%rsp); \
        mov %r13, 16(%rsp); \
        mov %r14,  8(%rsp); \
        mov %r15,  0(%rsp);

#define BULK_NOFP_RESTORE_REGISTERS \
        mov 48(%rsp), %rdi; \
        mov 40(%rsp), %rbp; \
        mov 32(%rsp), %rbx; \
        mov 24(%rsp), %r12; \
        mov 16(%rsp), %r13; \
        mov  8(%rsp), %r14; \
        mov  0(%rsp), %r15; \
        add $56, %rsp;

.globl _swapstacks
_swapstacks:
        SAVE_REGISTERS
        mov %rsp, (%rdi)
        mov (%rsi), %rsp
        RESTORE_REGISTERS
        // pass one value from previous to next coroutine
        mov %rdx, %rax
        ret


.globl _makestack
_makestack:
        /* save the needed arguments */
         mov %rdx, %r12 /* <f> */
        mov %rcx, %r13 /* <me> */
        
        /* "save" the current stack so it can be popped off in swapstacks */
        SAVE_REGISTERS
        
        mov %rsp, (%rdi)
        mov (%rsi), %rsp

        // save data required for task start
        push %rdx /* <f> */
        push %rcx /* <me> */
        
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
#ifndef DYNAMIC_LIB
	call _swapstacks
#else
        call _swapstacks@PLT
#endif
	
        // now restore arguments and run function
        pop %rdi /* <me> */
        pop %r12 /* <f> */
        mov %rax, %rsi
        call *%r12
        
        /* it is an error for a coro to return */
        hlt
