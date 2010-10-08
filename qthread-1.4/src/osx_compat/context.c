/* Portions of this file are Copyright (c) 2005-2006 Russ Cox, MIT; see COPYRIGHT */
/* Portions of this file are Copyright Sandia National Laboratories */

#include <osx_compat/taskimpl.h>

#ifdef NEEDPOWERMAKECONTEXT
void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	unsigned long *sp, *tos;
	va_list arg;

	tos = (unsigned long*)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size / sizeof(unsigned long);
	sp = tos - 16;
	ucp->mc.pc = (long)func;
	ucp->mc.sp = (long)sp;
	va_start(arg, argc);
	ucp->mc.r3 = va_arg(arg, long);
	va_end(arg);
}
#elif defined(NEEDX86MAKECONTEXT)
void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	uintptr_t *sp;
#ifdef NEEDX86REGISTERARGS
	int i;
	va_list argp;

	va_start(argp, argc);
#endif

	sp = (uintptr_t *)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size / sizeof(void *); /* sp = top of stack */
	sp -= argc; /* count down to where 8(%rsp) should be */
	sp = (void*)((uintptr_t)sp - (uintptr_t)sp % 16);	/* 16-align for OS X */
	/* now copy from my arg list to the function's arglist */
	memmove(sp, &argc + 1, argc * sizeof(uintptr_t));

#ifdef NEEDX86REGISTERARGS
	/* HOWEVER, the function may not be expecting to pull from the stack,
	 * several 64-bit architectures expect that args will be in the correct
	 * registers! */
	for (i=0;i<argc;i++) {
	    switch (i) {
		case 0: ucp->uc_mcontext.mc_edi = va_arg(argp, uintptr_t); break;
		case 1: ucp->uc_mcontext.mc_esi = va_arg(argp, uintptr_t); break;
		case 2: ucp->uc_mcontext.mc_edx = va_arg(argp, uintptr_t); break;
		case 3: ucp->uc_mcontext.mc_ecx = va_arg(argp, uintptr_t); break;
		/*case 4: ucp->uc_mcontext.mc_r8 = va_arg(argp, uintptr_t); break;
		case 5: ucp->uc_mcontext.mc_r9 = va_arg(argp, uintptr_t); break;*/
	    }
	}
#endif

	*--sp = 0;		/* return address */
	ucp->uc_mcontext.mc_eip = (long)func;
	ucp->uc_mcontext.mc_esp = (long)sp;
#ifdef NEEDX86REGISTERARGS
	va_end(argp);
#endif
}
#elif defined(NEEDTILEMAKECONTEXT)
/* This function is entirely copyright Sandia National Laboratories */
void makecontext(ucontext_t *ucp, void (*func)(void), int argc, ...)
{
	unsigned long *sp;
	unsigned long *tos = ucp->uc_stack.ss_sp;
	int i;
	va_list arg;

	tos += ucp->uc_stack.ss_size / sizeof(unsigned long *);
	tos -= 1; // allow space for an incoming lr
	sp = tos - argc; // allow space for arguments
	sp = (void*)((unsigned long)sp - (unsigned long)sp % 64); /* 64-align for Tilera */
	/* now copy from my arg list to the function's arglist (yes, I know this is voodoo) */
	//memmove(sp, &argc + 1, argc * sizeof(void*));
	/* The function may also expect to pull args from up to nine registers */
	va_start(arg, argc);
	for (i=0; i<argc; i++) {
	    if (i == 0) {
		ucp->mc.arg0 = va_arg(arg, unsigned long);
	    }
	}
	ucp->mc.pc = (unsigned long)func;
	ucp->mc.sp = (unsigned long)sp;
	ucp->mc.first = 1;
	va_end(arg);
}
#endif

#ifdef NEEDSWAPCONTEXT
int swapcontext(ucontext_t *oucp, ucontext_t *ucp)
{
    /* note that my getcontext implementation has only two possible return
     * values: 1 and 0. If it's 0, then I successfully got the context. If it's
     * 1, then I've just swapped back into a previously fetched context (i.e. I
     * do NOT want to swap again, because that'll put me into a nasty loop). */
	if(getcontext(oucp) == 0) {
		setcontext(ucp);
	}
	return 0;
}
#endif

