# GA_F77_SCALAPACK_TEST
# ---------------------
# Generate Fortran 77 conftest for SCALAPACK.
AC_DEFUN([GA_F77_SCALAPACK_TEST], [AC_LANG_CONFTEST([AC_LANG_PROGRAM([],
[[      implicit none
      external PDGETRS
      CALL PDGETRS ()]])])
])


# GA_C_SCALAPACK_TEST
# -------------------
# Generate C conftest for SCALAPACK.
AC_DEFUN([GA_C_SCALAPACK_TEST], [AC_LANG_CONFTEST([AC_LANG_PROGRAM(
[#ifdef __cplusplus
extern "C" {
#endif
char pdgetrs ();
#ifdef __cplusplus
}
#endif
],
[[char result = pdgetrs ();
]])])
])


# GA_SCALAPACK([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
# ---------------------------------------------------
# Modeled after GA_LAPACK and GA_BLAS.
# Tries to find ScaLAPACK library. The netlib implementation claims that
# ScaLAPACK depends on PBLAS, LAPACK, BLAS, and BLACS. However, some
# implementations such as Intel's combine ScaLAPACK, LAPACK, and BLAS together
# such that PBLAS and BLACS are not needed. This macro attempts to find the
# combination of libraries necessary to use ScaLAPACK.
AC_DEFUN([GA_SCALAPACK],
[AC_REQUIRE([GA_LAPACK])
AC_ARG_WITH([scalapack],
    [AS_HELP_STRING([--with-scalapack=[[ARG]]], [use ScaLAPACK library])])

ga_scalapack_ok=no
AS_IF([test "x$with_scalapack" = xno], [ga_scalapack_ok=skip])

# Parse --with-scalapack argument. Clear previous values first.
SCALAPACK_LIBS=
SCALAPACK_LDFLAGS=
SCALAPACK_CPPFLAGS=
GA_ARG_PARSE([with_scalapack],
    [SCALAPACK_LIBS], [SCALAPACK_LDFLAGS], [SCALAPACK_CPPFLAGS])

# Get fortran linker name of ScaLAPACK function to check for.
AC_F77_FUNC([pdgetrs])

ga_save_LIBS="$LIBS"
ga_save_LDFLAGS="$LDFLAGS"
ga_save_CPPFLAGS="$CPPFLAGS"

LDFLAGS="$LAPACK_LDFLAGS $BLAS_LDFLAGS $LDFLAGS"
CPPFLAGS="$LAPACK_CPPFLAGS $BLAS_CPPFLAGS $CPPFLAGS"

AC_MSG_NOTICE([Attempting to locate SCALAPACK library])

# First, check environment/command-line variables.
# If failed, erase SCALAPACK_LIBS but maintain SCALAPACK_LDFLAGS and
# SCALAPACK_CPPFLAGS.
AS_IF([test $ga_scalapack_ok = no],
    [LIBS="$SCALAPACK_LIBS $LAPACK_LIBS $BLAS_LIBS $LIBS"
     AS_IF([test "x$enable_f77" = xno],
        [AC_MSG_CHECKING([for C SCALAPACK with user-supplied flags])
         AC_LANG_PUSH([C])
         GA_C_SCALAPACK_TEST()
         AC_LINK_IFELSE([], [ga_scalapack_ok=yes], [SCALAPACK_LIBS=])
         AC_LANG_POP([C])],
        [AC_MSG_CHECKING([for Fortran 77 SCALAPACK with user-supplied flags])
         AC_LANG_PUSH([Fortran 77])
         GA_F77_SCALAPACK_TEST()
         AC_LINK_IFELSE([], [ga_scalapack_ok=yes], [SCALAPACK_LIBS=])
         AC_LANG_POP([Fortran 77])])
     AC_MSG_RESULT([$ga_scalapack_ok])
     LIBS="$ga_save_LIBS"])

# Generic ScaLAPACK library?
AS_IF([test $ga_scalapack_ok = no],
    [AC_CHECK_LIB([scalapack], [$pdgetrs],
        [ga_scalapack_ok=yes; SCALAPACK_LIBS="-lscalapack"], [], [$FLIBS])
     LIBS="$ga_save_LIBS"])

# TODO tests for PBLAS and BLACS to enable ScaLAPACK just in case all
# packages are separate...

CPPFLAGS="$ga_save_CPPFLAGS"
LDFLAGS="$ga_save_LDFLAGS"

AC_SUBST([SCALAPACK_LIBS])
AC_SUBST([SCALAPACK_LDFLAGS])
AC_SUBST([SCALAPACK_CPPFLAGS])

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
AS_IF([test $ga_scalapack_ok = yes],
    [have_scalapack=1
     $1],
    [AC_MSG_WARN([ScaLAPACK library not found, interfaces won't be defined])
     have_scalapack=0
     $2])
AC_DEFINE_UNQUOTED([HAVE_SCALAPACK], [$have_scalapack],
    [Define to 1 if you have ScaLAPACK library.])
AM_CONDITIONAL([HAVE_SCALAPACK], [test $ga_scalapack_ok = yes])
])dnl GA_SCALAPACK
