# ARMCI_F77_ENABLE()
#-------------------
# Whether to enable Fortran code within ARMCI.
AC_DEFUN([ARMCI_F77_ENABLE],
[AC_ARG_ENABLE([f77],
    [AS_HELP_STRING([--enable-f77], [enable Fortran code])],
    [],
    [enable_f77=no])
AS_IF([test "x$enable_f77" = xno], [armci_nofort=1], [armci_nofort=0])
AC_DEFINE_UNQUOTED([NOFORT], [$armci_nofort],
    [Define to 1 if not using Fortran])
AM_CONDITIONAL([NOFORT], [test "x$enable_f77" = xno])
])# ARMCI_F77_ENABLE
