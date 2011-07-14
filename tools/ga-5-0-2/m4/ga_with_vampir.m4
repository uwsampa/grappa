# GA_WITH_VAMPIR
# --------------
# vampir-related stuff.
AC_DEFUN([GA_WITH_VAMPIR],
[AC_ARG_WITH([vampir],
    [AS_HELP_STRING([--with-vampir[[=ARG]]],[optionally located at DIR])],
    [with_vampir=yes
    AC_DEFINE([USE_VAMPIR], [1], [define when using vampir])],
    [with_vampir=no])
AM_CONDITIONAL([USE_VAMPIR], [test x$with_vampir = xyes])
])dnl
