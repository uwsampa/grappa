# ARMCI_TAS()
# -----------
# Attempt to compile the inline assembly test-and-set code.  If we fail,
# search for a list of compilers known to work and try again.
#
AC_DEFUN([ARMCI_TAS], [
AC_CACHE_CHECK([whether we can compile test-and-set inline assembly],
    [armci_cv_as_tas],
    [ga_compile="$CC -c $CFLAGS -I$ARMCI_TOP_SRCDIR/src $CPPFLAGS $ARMCI_TOP_SRCDIR/src/tas.c"
     AS_ECHO(["$ga_compile"])>&AS_MESSAGE_LOG_FD
     AS_IF([$ga_compile 1>&AS_MESSAGE_LOG_FD 2>&1],
        [armci_cv_as_tas=yes],
        [armci_cv_as_tas=no])
     rm -f tas.o])
AC_CACHE_CHECK([whether we can compile test-and-set inline assembly with help],
    [armci_cv_as_tas_with],
    [AC_PATH_PROGS_FEATURE_CHECK([ARMCI_TAS_AS], [bgxlc xlc gcc],
        [ga_compile="$ac_path_ARMCI_TAS_AS -c $CFLAGS -I$ARMCI_TOP_SRCDIR/src $CPPFLAGS $ARMCI_TOP_SRCDIR/src/tas.c"
         AS_ECHO(["$ga_compile"])>&AS_MESSAGE_LOG_FD
         AS_IF([$ga_compile 1>&AS_MESSAGE_LOG_FD 2>&1],
            [armci_cv_as_tas_with=$ac_path_ARMCI_TAS_AS
             ac_cv_path_ARMCI_TAS_AS=$ac_path_ARMCI_TAS_AS
             ac_path_ARMCI_TAS_AS_found=:])
         rm -f tas.o],
        [armci_cv_as_tas_with=no])])
AC_SUBST([ARMCI_TAS_AS], [$ac_cv_path_ARMCI_TAS_AS])
AM_CONDITIONAL([INLINE_TAS_OKAY], [test "x$armci_cv_as_tas" = xyes])
AM_CONDITIONAL([INLINE_TAS_WITH], [test "x$armci_cv_as_tas_with" != xno])
])dnl
