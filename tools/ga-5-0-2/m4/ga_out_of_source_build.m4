# GA_OUT_OF_SOURCE_BUILD
# ----------------------
# Verify that an out-of-source build is being performed and error otherwise.
AC_DEFUN([GA_OUT_OF_SOURCE_BUILD],
[AC_CACHE_CHECK([for out of source build], [ga_cv_vpath],
[AS_IF([test -f $ac_unique_file],
    [ga_cv_vpath=no],
    [ga_cv_vpath=yes])
])
AS_IF([test x$ga_cv_vpath = xno],
    [AC_MSG_ERROR([You must perform an out of source build in order to preserve the old GNUmakefile environment. Please create a subdirectory, cd into it, and run configure from there.])])
])# GA_OUT_OF_SOURCE_BUILD
