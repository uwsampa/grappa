// $Date: 2008/12/08 23:20:00 $
// $Revision: 1.2 $
// $RCSFile$
// $Name:  $
// $State: Stab $:
//

// This program prints the number of seconds since
// the epoch to stdout

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char cvs_info[] = "$Date: 2008/12/08 23:20:00 $ $Revision: 1.2 $ $Name:  $ $RCSFile$";

int main(void){
  time_t now;

  if ( (now = time( NULL ) ) == ((time_t)-1) ){
    exit( EXIT_FAILURE );
  }
  printf("%ld\n", (long) now);

  return 0;
} // End Main


/*
Emacs reads the stuff in this comment to set local variables
Local Variables:
compile-command: "make sec_frm_epoch"
End:
*/



