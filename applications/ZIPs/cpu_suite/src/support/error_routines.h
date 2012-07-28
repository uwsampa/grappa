/******************************************************************************\
*                                                                              *
*                                                                              *
*                                                                              *
*                                                                              *
\******************************************************************************/
#ifndef ERR_RT_INCLUDED
#define ERR_RT_INCLUDED

#include "common_inc.h"

void unique_tag(char* progname );

void sys_err_exit(int error, const char *fmt, ...);

void sys_err_ret(int error, const char *fmt, ...);

void err_msg_exit(const char *fmt, ...);

void err_msg_ret(const char *fmt, ...);

#endif


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make error_routines.o"
End:
*/



