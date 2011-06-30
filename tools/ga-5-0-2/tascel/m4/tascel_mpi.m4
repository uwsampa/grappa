# TASCEL_MPI
# ----------
# Establishes all things related to the Message Passing Interface (MPI).
# This includes the compilers to use (either standard or MPI wrappers)
# or the proper linker flags (-L), libs (-l) or preprocessor directives (-I).
# When MPI is desired (the default) it replaces the
# usual compiler the order here is necessary and it is all interdependent.
AC_DEFUN([TASCEL_MPI], [
# GA_MP_* vars might exist in environment, but they are really internal.
# Reset them.
GA_MP_LIBS=
GA_MP_LDFLAGS=
GA_MP_CPPFLAGS=
AC_ARG_WITH([mpi],
    [AS_HELP_STRING([--with-mpi[[=ARG]]],
        [select MPI as the messaging library (default); leave ARG blank to use MPI compiler wrappers])],
    [],
    [with_mpi=yes])
need_parse=no
AS_CASE([$with_mpi],
    [yes],  [with_mpi_wrappers=yes],
    [no],   [],
    [*],    [need_parse=yes])
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
     AS_IF([test "x$CXX" != x],
      [AS_IF([test "x$CXX" != "x$MPICXX"],
        [AC_MSG_ERROR([MPI compilers wanted (--with-mpi was specified without
        arguments (or was assumed by default)), but CXX was also specified and
        CXX != MPICXX.  Perhaps you meant to set MPICXX instead?  Or perhaps you
        meant to specify the path to an MPI installation such as
        --with-mpi=/path/to/mpi?  The easiest thing to try is to unset CXX
        (CXX=) as part of the configure invocation.])])])
    ])
AS_IF([test x$need_parse = xyes],
    [GA_ARG_PARSE([with_mpi],
        [GA_MP_LIBS], [GA_MP_LDFLAGS], [GA_MP_CPPFLAGS])])
AC_SUBST([GA_MP_LIBS])
AC_SUBST([GA_MP_LDFLAGS])
AC_SUBST([GA_MP_CPPFLAGS])
])dnl
