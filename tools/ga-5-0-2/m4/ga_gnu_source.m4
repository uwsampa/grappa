# GA_GNU_SOURCE
# -------
# Define _GNU_SOURCE if using glibc.
AC_DEFUN([GA_GNU_SOURCE],
[AC_CACHE_CHECK([for GNU C library], [ga_cv_glibc],
[AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([[#include <features.h>]],
        [[#if ! (defined __GLIBC__ || defined __GNU_LIBRARY__)
#error Not a GNU C library system
#endif]])],
    [ga_cv_glibc=yes],
    [ga_cv_glibc=no])])
AS_IF([test x$ga_cv_glibc = xyes],
    [AC_DEFINE([_GNU_SOURCE], [1],
        [Always define this when using the GNU C library])])
])# GA_GNU_SOURCE
