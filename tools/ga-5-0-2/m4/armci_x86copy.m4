# ARMCI_X86COPY()
# ---------------
# Attempt to compile the inline assembly x86copy code.  If we fail,
# search for a list of compilers known to work and try again.
#
AC_DEFUN([ARMCI_X86COPY], [
AC_CACHE_CHECK([whether we can compile x86copy inline assembly],
    [armci_cv_as_x86copy],
    [ga_compile="$CC -c $CFLAGS -I$ARMCI_TOP_SRCDIR/src $CPPFLAGS $ARMCI_TOP_SRCDIR/src/x86copy.c"
     AS_ECHO(["$ga_compile"])>&AS_MESSAGE_LOG_FD
     AS_IF([$ga_compile 1>&AS_MESSAGE_LOG_FD 2>&1],
        [armci_cv_as_x86copy=yes],
        [armci_cv_as_x86copy=no])
     rm -f x86copy.o])
AC_CACHE_CHECK([whether we can compile x86copy inline assembly with help],
    [armci_cv_as_x86copy_with],
    [AC_PATH_PROGS_FEATURE_CHECK([ARMCI_X86COPY_AS], [bgxlc xlc gcc],
        [ga_compile="$ac_path_ARMCI_X86COPY_AS -c $CFLAGS -I$ARMCI_TOP_SRCDIR/src $CPPFLAGS $ARMCI_TOP_SRCDIR/src/x86copy.c"
         AS_ECHO(["$ga_compile"])>&AS_MESSAGE_LOG_FD
         AS_IF([$ga_compile 1>&AS_MESSAGE_LOG_FD 2>&1],
            [armci_cv_as_x86copy_with=$ac_path_ARMCI_X86COPY_AS
             ac_cv_path_ARMCI_X86COPY_AS=$ac_path_ARMCI_X86COPY_AS
             ac_path_ARMCI_X86COPY_AS_found=:])
         rm -f x86copy.o],
        [armci_cv_as_x86copy_with=no])])
AC_SUBST([ARMCI_X86COPY_AS], [$ac_cv_path_ARMCI_X86COPY_AS])
AS_CASE([$ga_cv_target:$host_cpu],
[LINUX:*86*],
    [armci_cv_as_x86copy_need=yes
     AC_DEFINE([COPY686], [1], [Defined when TARGET=LINUX and cpu is x86])])
AM_CONDITIONAL([INLINE_X86COPY_NEEDED],
    [test "x$armci_cv_as_x86copy_need" = xyes])
AM_CONDITIONAL([INLINE_X86COPY_OKAY],
    [test "x$armci_cv_as_x86copy" = xyes])
AM_CONDITIONAL([INLINE_X86COPY_WITH],
    [test "x$armci_cv_as_x86copy_with" != xno])
])dnl
