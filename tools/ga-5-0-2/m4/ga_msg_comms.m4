# GA_MSG_COMMS
# ------------
# Establishes all things related to messageing libraries.
# This includes the compilers to use (either standard or MPI wrappers)
# or the proper linker flags (-L), libs (-l) or preprocessor directives (-I).
# Yes, it's a beefy AC macro, but because when MPI is desired it replaces the
# usual compiler the order here is necessary and it is all interdependent.
AC_DEFUN([GA_MSG_COMMS], [
# GA_MP_* vars might exist in environment, but they are really internal.
# Reset them.
GA_MP_LIBS=
GA_MP_LDFLAGS=
GA_MP_CPPFLAGS=
# First of all, which messaging library do we want?
AC_ARG_WITH([mpi],
    [AS_HELP_STRING([--with-mpi[[=ARG]]],
        [select MPI as the messaging library (default); leave ARG blank to use MPI compiler wrappers])],
    [],
    [with_mpi=maybe])
AC_ARG_WITH([tcgmsg],
    [AS_HELP_STRING([--with-tcgmsg],
        [select TCGMSG as the messaging library; if --with-mpi is also specified then TCGMSG over MPI is used])],
    [],
    [with_tcgmsg=no])
need_parse=no
AS_CASE([$with_mpi:$with_tcgmsg],
[maybe:yes],[ga_msg_comms=TCGMSG; with_mpi=no],
[maybe:no], [ga_msg_comms=MPI; with_mpi_wrappers=yes; with_mpi=yes],
[yes:yes],  [ga_msg_comms=TCGMSGMPI; with_mpi_wrappers=yes],
[yes:no],   [ga_msg_comms=MPI; with_mpi_wrappers=yes],
[no:yes],   [ga_msg_comms=TCGMSG],
[no:no],    [AC_MSG_ERROR([select at least one messaging library])],
[*:yes],    [ga_msg_comms=TCGMSGMPI; need_parse=yes],
[*:no],     [ga_msg_comms=MPI; need_parse=yes],
[*:*],      [AC_MSG_ERROR([unknown messaging library settings])])
# Hack. If TARGET=MACX and MSG_COMMS=TCGMSG, we really want TCGMSG5.
AS_CASE([$ga_cv_target_base:$ga_msg_comms],
    [MACX:TCGMSG], [ga_msg_comms=TCGMSG5])
# Sanity check for --with-mpi and environment var interplay.
AS_IF([test "x$with_mpi_wrappers" = xyes],
    [AS_IF([test "x$CC" != x],
      [AS_IF([test "x$CC" != "x$MPICC"],
        [AC_MSG_ERROR([MPI compilers wanted (--with-mpi was specified without
        arguments (or was assumed by default)), but CC was also specified and
        CC != MPICC.  Perhaps you meant to set MPICC instead?  Or perhaps you
        meant to specify the path to an MPI installation such as
        --with-mpi=/path/to/mpi?  The easiest thing to try is to unset CC
        (CC=) as part of the configure invocation.])])])
     AS_IF([test "x$F77" != x],
      [AS_IF([test "x$F77" != "x$MPIF77"],
        [AC_MSG_ERROR([MPI compilers wanted (--with-mpi was specified without
        arguments (or was assumed by default)), but F77 was also specified and
        F77 != MPIF77.  Perhaps you meant to set MPIF77 instead?  Or perhaps you
        meant to specify the path to an MPI installation such as
        --with-mpi=/path/to/mpi?  The easiest thing to try is to unset F77
        (F77=) as part of the configure invocation.])])])
     AS_IF([test "x$enable_cxx" = xyes], [
     AS_IF([test "x$CXX" != x],
      [AS_IF([test "x$CXX" != "x$MPICXX"],
        [AC_MSG_ERROR([MPI compilers wanted (--with-mpi was specified without
        arguments (or was assumed by default)), but CXX was also specified and
        CXX != MPICXX.  Perhaps you meant to set MPICXX instead?  Or perhaps you
        meant to specify the path to an MPI installation such as
        --with-mpi=/path/to/mpi?  The easiest thing to try is to unset CXX
        (CXX=) as part of the configure invocation.])])])])
    ])
AS_IF([test x$need_parse = xyes],
    [GA_ARG_PARSE([with_mpi], [GA_MP_LIBS], [GA_MP_LDFLAGS], [GA_MP_CPPFLAGS])])
# TCGMSG is no longer supported for ARMCI development.
AS_IF([test "x$ARMCI_TOP_SRCDIR" != x],
    [AS_IF([test ! -d "$ARMCI_TOP_SRCDIR/../global"],
        [err=no
         AS_CASE([$ga_msg_comms],
            [TCGMSG],   [err=yes],
            [TCGMSG5],  [err=yes],
            [TCGMSGMPI],[err=yes])
         AS_IF([test "x$err" = xyes],
            [AC_MSG_ERROR([ARMCI no longer supports TCGMSG outside of the Global Arrays distribution])])])])
# PVM is no longer supported, but there is still some code around
# referring to it.
AM_CONDITIONAL([MSG_COMMS_MPI],
    [test x$ga_msg_comms = xMPI || test x$ga_msg_comms = xTCGMSGMPI])
AM_CONDITIONAL([MSG_COMMS_PVM],       [test 1 = 0])
AM_CONDITIONAL([MSG_COMMS_TCGMSG4],   [test x$ga_msg_comms = xTCGMSG])
AM_CONDITIONAL([MSG_COMMS_TCGMSG5],   [test x$ga_msg_comms = xTCGMSG5])
AM_CONDITIONAL([MSG_COMMS_TCGMSGMPI], [test x$ga_msg_comms = xTCGMSGMPI])
AM_CONDITIONAL([MSG_COMMS_TCGMSG],
    [test x$ga_msg_comms = xTCGMSG || test x$ga_msg_comms = xTCGMSG5 || test x$ga_msg_comms = xTCGMSGMPI])
AS_CASE([$ga_msg_comms],
    [MPI],      [AC_DEFINE([MSG_COMMS_MPI], [1],
                    [Use MPI for messaging])],
    [PVM],      [],
    [TCGMSG],   [AC_DEFINE([MSG_COMMS_TCGMSG4], [1],
                    [Use TCGMSG (ipcv4.0) for messaging])
                 AC_DEFINE([MSG_COMMS_TCGMSG], [1],
                    [Use TCGMSG for messaging])
                 AC_DEFINE([TCGMSG], [1],
                    [deprecated, use MSG_COMMS_TCGMSG])],
    [TCGMSG5],  [AC_DEFINE([MSG_COMMS_TCGMSG5], [1],
                    [Use TCGMSG (ipcv5.0) for messaing])
                 AC_DEFINE([MSG_COMMS_TCGMSG], [1],
                    [Use TCGMSG for messaging])
                 AC_DEFINE([TCGMSG], [1],
                    [deprecated, use MSG_COMMS_TCGMSG])],
    [TCGMSGMPI],[AC_DEFINE([MSG_COMMS_TCGMSGMPI], [1],
                    [Use TCGMSG over MPI for messaging])
                 AC_DEFINE([MSG_COMMS_MPI], [1],
                    [Use MPI for messaging])])
AC_SUBST([GA_MP_LIBS])
AC_SUBST([GA_MP_LDFLAGS])
AC_SUBST([GA_MP_CPPFLAGS])
])dnl
