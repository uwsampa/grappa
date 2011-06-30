###############################################################################
# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
###############################################################################
AC_PREREQ([2.63])
AC_INIT([Task Scheduling Library (tascel)],
        [0.0.1],
        [hpctools@pnl.gov],
        [tascel])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/Comm.h])

###############################################################################
# Init automake
###############################################################################
AM_INIT_AUTOMAKE([color-tests foreign parallel-tests silent-rules subdir-objects])

# Don't emit "rebuild rules" for configure, Makefile.ins, etc.
AM_MAINTAINER_MODE

###############################################################################
# Misc. information and package setup.
###############################################################################
TASCEL_TOP_BUILDDIR="`pwd`"
cd "$srcdir"
TASCEL_TOP_SRCDIR="`pwd`"
cd "$TASCEL_TOP_BUILDDIR"
AS_IF([test "$TASCEL_TOP_BUILDDIR" != "TASCEL_TOP_SRCDIR"],
    [AC_MSG_NOTICE([Detected VPATH build])])
AC_CACHE_CHECK([whether tascel is built outside of GA], [tascel_cv_standalone],
    [AS_IF([test -f "$TASCEL_TOP_BUILDDIR/../armci/config.status"],
        [tascel_cv_standalone=no],
        [tascel_cv_standalone=yes])])
AM_CONDITIONAL([TASCEL_STANDALONE], [test x$tascel_cv_standalone = xyes])

# If MPI is desired we use the MPI compiler wrappers instead of the standard
# compilers.
TASCEL_MPI

GA_DISABLE_MPI_TESTS

GA_F77_DISABLE
# C++ is always enabled for tascel
enable_cxx=yes

###############################################################################
# C compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C compiler])
AC_MSG_NOTICE

tascel_save_CFLAGS="$CFLAGS"
GA_PROG_MPICC
CFLAGS="$tascel_save_CFLAGS"
GA_MPICC_TEST_COMPILE

# Checks for C header files.

# Checks for C typedefs, structures, and compiler characteristics.

# Checks for C library functions.

# Hack to append .x to executables.
AC_SUBST([EXEEXT], [.x$EXEEXT])

###############################################################################
# C++ compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C++ compiler])
AC_MSG_NOTICE
AC_LANG_PUSH([C++])

tascel_save_CXXFLAGS="$CXXFLAGS"
GA_PROG_MPICXX
CXXFLAGS="$tascel_save_CXXFLAGS"
GA_MPICXX_TEST

# Checks for C++ header files.

# Checks for C++ typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T
AC_TYPE_LONG_DOUBLE
AC_TYPE_LONG_LONG_INT
AC_TYPE_SIZE_T
AC_TYPE_UNSIGNED_LONG_LONG_INT

# Checks for C++ type sizes.
AC_CHECK_SIZEOF([void*])
AC_CHECK_SIZEOF([char])
AC_CHECK_SIZEOF([short])
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])
AS_IF([test x$ac_cv_type_long_long_int = xyes], [AC_CHECK_SIZEOF([long long])])
AC_CHECK_SIZEOF([float])
AC_CHECK_SIZEOF([double])
AS_IF([test x$ac_cv_type_long_double = xyes], [AC_CHECK_SIZEOF([long double])])

# Checks for C++ library functions.

AC_LANG_POP([C++])

###############################################################################
# Fortran compiler
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Fortran compiler])
AC_MSG_NOTICE
AC_LANG_PUSH([Fortran 77])

tascel_save_FFLAGS="$FFLAGS"
GA_PROG_MPIF77
FFLAGS="$tascel_save_FFLAGS"

# If Fortran 77 compiler was not found, we disable support for it.
AS_IF([test "x$F77" = x], [enable_f77=no])

# Perform Fortran 77 tests only if enabled.
if test "x$enable_f77" = xyes; then :

GA_MPIF77_TEST_COMPILE
GA_F77_LIBRARY_LDFLAGS
GA_F77_INTEGER_SIZE
AC_F77_WRAPPERS

#Checks for Fortran typedefs, structures, and compiler characteristics.

#Checks for Fortran type sizes.
tascel_save_FFLAGS="$FFLAGS"; FFLAGS="$FFLAGS $FFLAG_INT"
GA_F77_CHECK_SIZEOF([INTEGER], [$ga_cv_f77_integer_size])
FFLAGS="$tascel_save_FFLAGS"
GA_F77_CHECK_SIZEOF([REAL], [4])
GA_F77_CHECK_SIZEOF([DOUBLE PRECISION], [8])

# The rest of the Fortran 77 to C compatibility checks.
GA_F2C_NOMAIN
GA_F2C_CMDARGS
GA_F2C_HIDDEN_STRING_LENGTH_CONVENTION
GA_F2C_MATCH_TYPES([INTEGER], [short, int, long, long long])
GA_F2C_MATCH_TYPES([REAL], [float, double, long double])
GA_F2C_MATCH_TYPES([DOUBLE PRECISION], [float, double, long double])

else

# enable_f77=no version of AC_F77_WRAPPERS
AC_DEFINE([F77_FUNC(name,NAME)], [name], [F77 name mangling])
AC_DEFINE([F77_FUNC_(name,NAME)],[name @%:@@%:@ _], [F77 name mangling])
# enable_f77=no version of GA_F77_CHECK_SIZEOF
AC_DEFINE_UNQUOTED(AS_TR_CPP(sizeof_f77_INTEGER), [$ac_cv_sizeof_voidp],
    [The size of 'INTEGER' as computed by C's sizeof.])
AC_DEFINE_UNQUOTED(AS_TR_CPP(sizeof_f77_REAL), [4],
    [The size of 'REAL' as computed by C's sizeof.])
AC_DEFINE_UNQUOTED(AS_TR_CPP(sizeof_f77_DOUBLE PRECISION), [8],
    [The size of 'DOUBLE PRECISION' as computed by C's sizeof.])
# enable_f77=no version of GA_F2C_NOMAIN
AC_SUBST([FLD_NOMAIN], [])
# enable_f77=no version of GA_F2C_CMDARGS
F2C_GETARG="F2C_GETARG"
F2C_IARGC="F2C_IARGC"
AC_SUBST([F2C_GETARG])
AC_SUBST([F2C_IARGC])
# enable_f77=no version of GA_F2C_HIDDEN_STRING_LENGTH_CONVENTION
AC_DEFINE([F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS], [1],
    [whether the hidden string length comes after all other args])
# enable_f77=no version of GA_F2C_MATCH_TYPES
AC_SUBST([F2C_INTEGER_C_TYPE], [long])
AC_SUBST([F2C_REAL_C_TYPE], [float])
AC_SUBST([F2C_DOUBLE_PRECISION_C_TYPE], [double])

fi

AC_LANG_POP([Fortran 77])

###############################################################################
# Look for Global Arrays
###############################################################################
AS_IF([test "x$tascel_cv_standalone" = xno],
    [GA_LIBS="../libga.la"
     GA_CPPFLAGS="-I$srcdir/../global/src -I../global/src -I$srcdir/../ma -I../ma -I../gaf2c -I$srcdir/../armci/src"],
    [TASCEL_CHECK_PACKAGE([ga], [ga.h], [ga], [GA_Initialize], [$FLIBS],
        [GA_LIBS="-lga"],
        [AS_UNSET([ac_cv_search_GA_Initialize])
         TASCEL_CHECK_PACKAGE([ga], [ga.h], [global], [GA_Initialize],
            [-lma -larmci -llinalg $FLIBS],
            [GA_LIBS="-lglobal -lma -larmci -llinalg"],
            [AC_MSG_FAILURE([Could not locate Global Arrays 5.x nor 4.x])])])])
AC_SUBST([GA_CPPFLAGS])
AC_SUBST([GA_LDFLAGS])
AC_SUBST([GA_LIBS])

###############################################################################
# Checks for BLAS/LAPACK/SCALAPACK require both C and Fortran features to
# have been detected. Further, they assume the current AC_LANG is C.
# NOTE: We only look for BLAS and LAPACK because GA does.  We will get linker
# errors if GA was built with an external BLAS and LAPACK.
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Checks for BLAS,LAPACK,ScaLAPACK])
AC_MSG_NOTICE

GA_BLAS
GA_LAPACK
GA_SCALAPACK

###############################################################################
# Libtool setup -- no compiler/linker tests after this
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Libtool setup])
AC_MSG_NOTICE

GA_AR
LT_INIT([disable-shared])

###############################################################################
# The End
###############################################################################
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Report on what we found.
AC_MSG_NOTICE([])
AC_MSG_NOTICE([**************************************************************])
AC_MSG_NOTICE([ $PACKAGE_NAME configured as follows:])
AC_MSG_NOTICE([**************************************************************])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([GA_MP_CPPFLAGS=$GA_MP_CPPFLAGS])
AC_MSG_NOTICE([ GA_MP_LDFLAGS=$GA_MP_LDFLAGS])
AC_MSG_NOTICE([    GA_MP_LIBS=$GA_MP_LIBS])
AC_MSG_NOTICE([   GA_CPPFLAGS=$GA_CPPFLAGS])
AC_MSG_NOTICE([    GA_LDFLAGS=$GA_LDFLAGS])
AC_MSG_NOTICE([       GA_LIBS=$GA_LIBS])
AC_MSG_NOTICE([           F77=$F77])
AC_MSG_NOTICE([            CC=$CC])
AC_MSG_NOTICE([           CXX=$CXX])
AC_MSG_NOTICE([           CPP=$CPP])
AC_MSG_NOTICE([        FFLAGS=$FFLAGS])
AC_MSG_NOTICE([        CFLAGS=$CFLAGS])
AC_MSG_NOTICE([      CXXFLAGS=$CXXFLAGS])
AC_MSG_NOTICE([      CPPFLAGS=$CPPFLAGS])
AC_MSG_NOTICE([       LDFLAGS=$LDFLAGS])
AC_MSG_NOTICE([          LIBS=$LIBS])
AC_MSG_NOTICE([         FLIBS=$FLIBS])
AC_MSG_NOTICE([          DEFS=$DEFS])
AC_MSG_NOTICE([            AR=$AR])
AC_MSG_NOTICE([      AR_FLAGS=$AR_FLAGS])
AC_MSG_NOTICE([         SHELL=$SHELL])
AC_MSG_NOTICE([])
