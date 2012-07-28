/***************************************************************************\
* print_cmplr_flags prints the compiler flags used to build the benchmark.  *
* The method used is to have the makefile create a header file that         *
* consists of an array containing the flags. The makefile does              *
*    echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h             *
\***************************************************************************/

#include "common_inc.h"

#if HAS_CFLAGS
#include "cflags.h"
#endif

#if HAS_FFLAGS
#include "fflags.h"
#endif

void print_cmplr_flags(char* label){
#if HAS_CFLAGS
  printf( "%s %s\n", label, cflags);
#endif
#if HAS_FFLAGS
  printf( "%s %s\n", label, fflags);
#endif
}


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make print_cmplr_flags.o"
End:
*/



