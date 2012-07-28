#include "common_inc.h"

#include "error_routines.h"

char u_tag[ 300 ] ;

void unique_tag(char* progname ){
  /*********************************************************************\
  * Create a unique string to label the the error output. The string    *
  * starts with the program name followed by the name of the host       *
  * where the process runs. If the process was launched by RES, the     *
  * next number is the RES session number followed by the RES command   *
  * number. The final value is the pid for the process followed by "::" *
  \*********************************************************************/

  char* s,* c;
  pid_t pid;
  char pid_buf[9];
  char hostname[4096];

  // The program name
  strncat(u_tag, progname, (size_t)30);

  // The host name
  if ( gethostname(hostname, (size_t)80) == -1 ){
    strncat(hostname,"unknown_host",(size_t)12);
  }
  strncat(u_tag, "_",(size_t)1);
  strncat(u_tag, hostname,(size_t)80);
  strncat(u_tag, "_",(size_t)1);

  // If using RES the session number and command number
  if( (s = getenv("RES_SESSION")) != NULL){
    strcat(u_tag, s);
    strncat(u_tag, "_",(size_t)1);
    c = getenv("RES_COMMAND");
    strcat(u_tag, c);
  }

  // Add the pid for multiple processes running on the same host
  pid = getpid();
  snprintf(pid_buf,8,"%08ld",(long)pid);
  strcat(u_tag, pid_buf);

  strncat(u_tag, "::",(size_t)2);
}

static void err_doit(const int errnoflag, const int err_nmb,
		     const char *fmt, va_list ap){
  /************************************************************\
  * Write out an error message prefixed with a unique string   *
  \************************************************************/
#define BFSZ (4096)

  char buf[BFSZ];

  snprintf(buf, BFSZ-1 , "%s", u_tag);
  vsnprintf(buf+strlen(buf), BFSZ-strlen(buf)-1, fmt, ap);

  // Write the error message to memory locations following previous write
  if (errnoflag)
    snprintf(buf+strlen(buf), BFSZ-strlen(buf)-1,": %s",
	     strerror(err_nmb));
  // Add the new line in memory
  strcat(buf, "\n");
  fflush(stdout);
  // Write it out
  fputs(buf, stderr);
  fflush(NULL);
}

#define USE_ERRNO  (1)
#define NO_ERRNO  (0)
void sys_err_exit(int error, const char *fmt, ...){
  va_list  ap;

  va_start(ap, fmt);
  err_doit(USE_ERRNO, error, fmt, ap);
  va_end(ap);

  exit( EXIT_FAILURE );
}

void sys_err_ret(int error, const char *fmt, ...){
  va_list  ap;

  va_start(ap, fmt);
  err_doit(USE_ERRNO, error, fmt, ap);
  va_end(ap);

}
void err_msg_exit(const char *fmt, ...){
  va_list  ap;

  va_start(ap, fmt);
  err_doit(NO_ERRNO, 0, fmt, ap);
  va_end(ap);

  exit( EXIT_FAILURE );
}

void err_msg_ret(const char *fmt, ...){
  va_list  ap;

  va_start(ap, fmt);
  err_doit(NO_ERRNO, 0, fmt, ap);
  va_end(ap);

}


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make error_routines.o "
End:
*/



