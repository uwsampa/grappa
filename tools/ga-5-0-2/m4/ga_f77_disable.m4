# GA_F77_DISABLE()
# ----------------
# Whether to disable all Fortran code.
AC_DEFUN([GA_F77_DISABLE],
[AC_ARG_ENABLE([f77],
    [AS_HELP_STRING([--disable-f77], [disable Fortran code])],
    [],
    [enable_f77=yes])
AS_IF([test x$enable_f77 = xno], [ga_nofort=1], [ga_nofort=0])
AC_DEFINE_UNQUOTED([NOFORT], [$ga_nofort], [Define to 1 if not using Fortran])
AM_CONDITIONAL([NOFORT], [test x$enable_f77 = xno])
])# GA_F77_DISABLE
