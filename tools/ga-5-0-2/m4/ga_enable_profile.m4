# GA_ENABLE_PROFILE
# -----------------
# Whether to enable profiling. AC_DEFINEs ENABLE_PROFILE.
AC_DEFUN([GA_ENABLE_PROFILE],
[AC_ARG_ENABLE([profile],
    [AS_HELP_STRING([--enable-profile], [enable profiling])],
    [enable_profile=yes
    AC_DEFINE([ENABLE_PROFILE], [1], [Define if profiling is enabled])],
    [enable_profile=no])
AM_CONDITIONAL([ENABLE_PROFILE], [test x$enable_profile = xyes])
])dnl
