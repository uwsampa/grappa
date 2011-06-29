#ifndef _GA_VT_H_
#define _GA_VT_H_

/* This file is compiled only when a Vampirtrace instrumented version of the 
   GA is required.
*/

/* This file defines 3 routines that are used throughout the Vampirtrace
   instrumented version of the GAs: 

   - vampir_symdef is defined to associate numerical identifiers with user 
     defined states and activities.

   - vampir_begin is defined to register entering a user defined state.

   - vampir_end is defined to register leaving a user defined state.

   Also 2 functions are defined to ensure that MPI will be initialised and
   finalised when needed. MPI is required because Vampirtrace uses it 
   internally.

   Also a global counter <vampirtrace_level> is defined. This is used to keep
   track of GA library invocations from within the library itself.

   Examples:
   ---------

   vampir_symdef(32567,"pbeginf","tcgmsg",__FILE__,__LINE__);
   vampir_begin(32567,__FILE__,__LINE__);
   vampir_end(32567,__FILE__,__LINE__);

   cpp replaces the macros __FILE__ and __LINE__ with the actual file names
   and line numbers. This information is used to produce error messages that 
   state exactly where the error occurred. 
*/
extern int  vampirtrace_level;
extern void vampir_symdef(int id, char *state, char *activity, 
                          char *file, int line);
extern void vampir_begin(int id, char *file, int line);
extern void vampir_end(int id, char *file, int line);

extern void vampir_send(int me, int other, int length, int type);
extern void vampir_recv(int me, int other, int length, int type);

extern void vampir_start_comm(int source, int other, int length, int type);
extern void vampir_end_comm(int source, int other, int length, int type);

extern void vampir_begin_gop(int mynode, int nnodes, int length, int type);
extern void vampir_end_gop(int mynode, int nnodes, int length, int type);

extern void vampir_init(int argc, char **argv, char *file, int line);
extern void vampir_finalize(char *file, int line);

#endif
