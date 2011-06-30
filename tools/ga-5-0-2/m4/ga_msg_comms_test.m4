# _GA_MSG_COMMS_TEST_MPI(RESULT)
# ------------------------------
# Test link an MPI program. Add -lmpi if it fails without.
AC_DEFUN([_GA_MSG_COMMS_TEST_MPI],
[ga_save_LIBS="$LIBS"
ga_save_LDFLAGS="$LDFLAGS"
ga_save_CPPFLAGS="$CPPFLAGS"
LDFLAGS="$LDFLAGS $GA_MP_LDFLAGS"
CPPFLAGS="$CPPFLAGS $GA_MP_CPPFLAGS"
AS_TR_SH([$1])="mpi failed"
for mpilib in none mpi mpich ; do
    LIBS="$LIBS $GA_MP_LIBS"
    AS_IF([test x$mpilib != xnone], [LIBS="$LIBS -l$mpilib"])
    AC_LINK_IFELSE(
        [AC_LANG_PROGRAM(
[[#include <mpi.h>]],
[[int myargc; char **myargv; MPI_Init(&myargc,&myargv); MPI_Finalize();]])],
            [AS_TR_SH([$1])="mpi okay"
             AS_IF([test x$mpilib != xnone],
                [GA_MP_LIBS="$GA_MP_LIBS -l$mpilib"])
             break])
    LIBS="$ga_save_LIBS"
done
LIBS="$ga_save_LIBS"
LDFLAGS="$ga_save_LDFLAGS"
CPPFLAGS="$ga_save_CPPFLAGS"
]) # _GA_MSG_COMMS_TEST_MPI


# _GA_MSG_COMMS_TEST_TCGMSG(RESULT)
# ---------------------------------
# Test compile/link a TCGMSG program. Add -ltcgmsg if it fails without.
AC_DEFUN([_GA_MSG_COMMS_TEST_TCGMSG],
[ga_save_LIBS="$LIBS"
ga_save_LDFLAGS="$LDFLAGS"
ga_save_CPPFLAGS="$CPPFLAGS"
LIBS="$LIBS $GA_MP_LIBS"
LDFLAGS="$LDFLAGS $GA_MP_LDFLAGS"
CPPFLAGS="$CPPFLAGS $GA_MP_CPPFLAGS"
AS_TR_SH([$1])="tcgmsg failed"
AS_IF([test x$with_tcgmsg_internal = xyes],
    [AC_COMPILE_IFELSE([AC_LANG_SOURCE([[#include <tcgmsg.h>]])],
        [AS_TR_SH([$1])="tcgmsg okay"],
        [AS_TR_SH([$1])="tcgmsg failed"])],
    [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <tcgmsg.h>]],
        [[long t = tcg_time();]])],
        [AS_TR_SH([$1])="tcgmsg okay"],
        [AS_TR_SH([$1])="tcgmsg failed"])])
LIBS="$ga_save_LIBS"
LDFLAGS="$ga_save_LDFLAGS"
CPPFLAGS="$ga_save_CPPFLAGS"
]) # _GA_MSG_COMMS_TEST_TCGMSG


# GA_MSG_COMMS_TEST
# -----------------
# Test compile/link an appropriate messaging program.
AC_DEFUN([GA_MSG_COMMS_TEST], [
AC_CACHE_CHECK([whether a simple messaging program works], [ga_cv_test_msg], [
AS_CASE([$ga_msg_comms],
[MPI],                      [_GA_MSG_COMMS_TEST_MPI([ga_cv_test_msg])],
[TCGMSG|TCGMSG5|TCGMSGMPI], [_GA_MSG_COMMS_TEST_TCGMSG([ga_cv_test_msg])],
                            [ga_cv_test_msg=missing])
])
AS_CASE([$ga_cv_test_msg],
[*failed],
    [AC_MSG_WARN([Test compile/link of message program failed.])
     AC_MSG_WARN([If using the MPI compiler wrappers (the default),])
     AC_MSG_WARN([did you mean to set MPIF77/CC/CXX instead of F77/CC/CXX?])
     AC_MSG_WARN([Or did you specify the correct CPPFLAGS and LDFLAGS?])
     AC_MSG_WARN([e.g. --with-mpi="-I/path/to/mpi/include -L/path/to/mpi/lib"])
     AC_MSG_WARN([*OR* --with-mpi="/path/to/mpi/base -lmpi -lanother_lib"])
     AC_MSG_WARN([When using TCGMSG, --with-tcgmsg should be sufficient.])
     AC_MSG_WARN([When using TCGMSG over MPI, specify both])
     AC_MSG_WARN([--with-mpi (see above examples) and --with-tcgmsg])
     AC_MSG_ERROR([See config.log for details.])],
[missing],
    [AC_MSG_ERROR([message library not selected? see config.log for details])])
])dnl
