# GA_ENABLE_PEIGS
# ---------------
# Whether to enable PeIGS routines.
AC_DEFUN([GA_ENABLE_PEIGS],
[AC_ARG_ENABLE([peigs],
    [AS_HELP_STRING([--enable-peigs],
        [enable Parallel Eigensystem Solver interface])],
    [enable_peigs=yes
    AC_DEFINE([ENABLE_PEIGS], [1], [Define if PeIGS is enabled])],
    [enable_peigs=no])
AM_CONDITIONAL([ENABLE_PEIGS], [test x$enable_peigs = xyes])
])dnl
