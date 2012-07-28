/* CVS info                         */
/* $Date: 2010/03/18 17:03:31 $     */
/* $Revision: 1.4 $                 */
/* $RCSfile: bench.h,v $            */
/* $State: Stab $:                   */

#include "common_inc.h"

#include "bm_timers.h"
#include "error_routines.h"
#include "brand.h"

#include <sys/utsname.h>
#include <pwd.h>
#include "iobmversion.h"

int64 bench_init (int* argc, char *argv[], brand_t *br,
			 timer *t, /*@null@*/const char* const more_args);

void bench_end (timer *t, const int64 iters, /*@null@*/const char* const work);

unsigned long e_strtoul(const char *nptr);
