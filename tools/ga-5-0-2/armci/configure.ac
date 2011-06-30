# Process this file with autoconf to produce a configure script.

###############################################################################
# Init autoconf
###############################################################################
AC_PREREQ([2.64])
AC_INIT([Aggregate Remote Memory Copy Interface (ARMCI)], [1.4],
        [hpctools@pnl.gov], [armci],
        [http://www.emsl.pnl.gov/docs/parsoft/armci/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_LIBOBJ_DIR([compat])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/armci.c])

###############################################################################
# Init automake
###############################################################################
AM_INIT_AUTOMAKE([color-tests foreign parallel-tests silent-rules subdir-objects])

# Don't emit "rebuild rules" for configure, Makefile.ins, etc.
AM_MAINTAINER_MODE

###############################################################################
# Misc. information and package setup.
###############################################################################
GA_TARGET

GA_WITH_HELP

ARMCI_TOP_BUILDDIR="`pwd`"
cd "$srcdir"
ARMCI_TOP_SRCDIR="`pwd`"
cd "$ARMCI_TOP_BUILDDIR"
AS_IF([test "$ARMCI_TOP_BUILDDIR" != "$ARMCI_TOP_SRCDIR"],
      [AC_MSG_NOTICE([Detected VPATH build])])
AS_IF([test -d "$ARMCI_TOP_BUILDDIR/../global"],
      [AC_MSG_NOTICE([Detected Global Arrays distribution])])
AM_CONDITIONAL([ARMCI_STANDALONE], [test ! -d "$ARMCI_TOP_BUILDDIR/../global"])

# Determine messaging library up front because if MPI is desired we use the
# MPI compiler wrappers instead of the standard compilers.
GA_MSG_COMMS

# Whether to enable F77 source dependency.
ARMCI_F77_ENABLE

# Hack to enable NEW_MALLOC feature
AC_ARG_ENABLE([portals-new-malloc],
    [AS_HELP_STRING([--enable-portals-new-malloc],
        [add -DNEW_MALLOC to CPPFLAGS])])
AS_IF([test "x$enable_portals_new_malloc" = xyes],
    [AC_DEFINE([NEW_MALLOC], [1], [for portals, enable new malloc])])
AM_CONDITIONAL([PORTALS_ENABLE_NEW_MALLOC],
    [test "x$enable_portals_new_malloc" = xyes])

ARMCI_ENABLE_GPC
ARMCI_ENABLE_GROUP
ARMCI_ENABLE_NB_NONCONT
ARMCI_ENABLE_PROFILING
GA_DISABLE_MPI_TESTS
GA_ENABLE_CHECKPOINT
GA_ENABLE_OPT
GA_ENABLE_TRACE
GA_THREAD_SAFE
GA_WITH_VAMPIR

# Unless we rename all preprocessor references to MPI and MPIPP
# in the source code, this hack is necessary.
AH_BOTTOM([#ifdef MSG_COMMS_MPI
#   ifdef __cplusplus
#       define MPIPP
#   else
#       define MPI
#   endif /* __cplusplus */
#endif /* MSG_COMMS_MPI */])

#########################################
# C compiler
#########################################
AC_MSG_NOTICE
AC_MSG_NOTICE([C compiler])
AC_MSG_NOTICE

ga_save_CFLAGS="$CFLAGS"
GA_PROG_MPICC
CFLAGS="$ga_save_CFLAGS"
AS_IF([test x$with_mpi_wrappers = xyes], [GA_MPI_UNWRAP])
AC_USE_SYSTEM_EXTENSIONS
AX_COMPILER_VENDOR
# If F77 is enabled, only perform a simple compilation test.
AS_IF([test "x$with_mpi" != xno], [
    AS_CASE([$enable_f77:$enable_mpi_tests],
        [yes:*], [GA_MPICC_TEST_COMPILE],
        [no:yes],[GA_MPICC_TEST_LINK],
        [no:no], [GA_MPICC_TEST_COMPILE])])
# Hack to append .x to executables.
AC_SUBST([EXEEXT], [.x$EXEEXT])

GA_TARGET64

# Establish the underlying network infrastructure (SOCKETS, OPENIB, etc)
GA_ARMCI_NETWORK

# Checks for C header files.
AC_HEADER_ASSERT
AC_HEADER_DIRENT
AC_HEADER_SYS_WAIT
GA_CHECK_HEADERS([assert.h c_asm.h errno.h fcntl.h float.h ia64/sys/inline.h limits.h malloc.h math.h memory.h mpp/shmem.h netdb.h netinet/in.h netinet/tcp.h process.h setjmp.h signal.h stdarg.h stdint.h stdio.h stdlib.h string.h strings.h sys/types.h sys/atomic_op.h sys/file.h sys/ipc.h sys/mman.h sys/param.h sys/sem.h sys/shm.h sys/socket.h sys/stat.h sys/syscall.h sys/time.h sys/uio.h sys/wait.h time.h unistd.h windows.h winsock.h],
[], [],
[@%:@ifdef HAVE_RPC_TYPES_H
@%:@include <rpc/types.h>
@%:@endif
@%:@ifdef HAVE_SYS_TYPES_H
@%:@include <sys/types.h>
@%:@endif
@%:@ifdef HAVE_SYS_IPC_H
@%:@include <sys/ipc.h>
@%:@endif])

# Checks for C typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_RESTRICT
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIGNAL
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])
GA_FUNCTION

# Checks for C type sizes.
AC_CHECK_SIZEOF([void*])
AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])

# Checks for C library functions.
AC_FUNC_FORK
AC_FUNC_MMAP
GA_CHECK_FUNCS([bzero fastbcopy ftruncate gethostbyname getpagesize gettimeofday memset munmap select socket strchr strdup strerror strstr strtol _lock_try _acquire_lock])
AC_SEARCH_LIBS([floor], [m],
    [AC_DEFINE([HAVE_FLOOR], [1],
               [Define to 1 if you have the `floor' function.])],
    [AC_DEFINE([HAVE_FLOOR], [0],
               [Define to 1 if you have the `floor' function.])])
AC_SEARCH_LIBS([pow], [m],
    [AC_DEFINE([HAVE_POW], [1],
               [Define to 1 if you have the `pow' function.])],
    [AC_DEFINE([HAVE_POW], [0],
               [Define to 1 if you have the `pow' function.])])
AC_SEARCH_LIBS([sqrt], [m],
    [AC_DEFINE([HAVE_SQRT], [1],
               [Define to 1 if you have the `sqrt' function.])],
    [AC_DEFINE([HAVE_SQRT], [0],
               [Define to 1 if you have the `sqrt' function.])])

ARMCI_C_OPT

#########################################
# C++ compiler
#########################################
AC_MSG_NOTICE
AS_IF([test "x$enable_cxx" = xyes],
    [AC_MSG_NOTICE([C++ compiler])],
    [AC_MSG_NOTICE([C++ compiler (disabled, but some tests still required)])])
AC_MSG_NOTICE
AC_LANG_PUSH([C++])

# GA_PROG_MPICXX is required to silence complaints that C++ source exists
# even if C++ support has been disabled.
ga_save_CXXFLAGS="$CXXFLAGS"
GA_PROG_MPICXX
CXXFLAGS="$ga_save_CXXFLAGS"

AS_IF([test "x$enable_cxx" = xyes], [

AS_IF([test x$with_mpi_wrappers = xyes], [GA_MPI_UNWRAP])

AX_COMPILER_VENDOR
AS_IF([test "x$with_mpi" != xno], [GA_MPICXX_TEST])

# Checks for C++ header files.

# Checks for C++ typedefs, structures, and compiler characteristics.

# Checks for C++ type sizes.

# Checks for C++ library functions.

ARMCI_CXX_OPT

])

AC_LANG_POP([C++])

#########################################
# Assembler
#########################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Assembler])
AC_MSG_NOTICE

GA_AS
AM_PROG_AS

ARMCI_TAS
ARMCI_X86COPY
ARMCI_AS_NO_I386ASM

#########################################
# Fortran compiler
#########################################
AC_MSG_NOTICE
AS_IF([test "x$enable_f77" = xyes],
    [AC_MSG_NOTICE([Fortran compiler])],
    [AC_MSG_NOTICE([Fortran compiler (disabled, but some tests still required)])])
AC_MSG_NOTICE
AC_LANG_PUSH([Fortran 77])

ga_save_FFLAGS="$FFLAGS"
GA_PROG_MPIF77
FFLAGS="$ga_save_FFLAGS"

# If Fortran 77 compiler was not found, we disable support for it.
AS_IF([test "x$F77" = x], [enable_f77=no])

# Perform Fortran 77 tests only if enabled.
if test "x$enable_f77" = xyes; then :

AS_IF([test x$with_mpi_wrappers = xyes], [GA_MPI_UNWRAP])

AX_COMPILER_VENDOR
# If F77 is enabled, perform a simple link test.
AS_IF([test "x$with_mpi" != xno],
    [AS_IF([test "x$enable_mpi_tests" = xyes],
        [GA_MPIF77_TEST_LINK],
        [GA_MPIF77_TEST_COMPILE])])

GA_SUPPRESS_MESSAGE
GA_F77_LIBRARY_LDFLAGS
AC_F77_DUMMY_MAIN
AC_F77_WRAPPERS
GA_F77_CPP_SYMBOL
GA_F77_FIXED

# Checks for Fortran typedefs, structures, and compiler characteristics.

# Checks for Fortran type sizes.

# Checks for Fortran library functions.

# Fortran to C compatibility.
ARMCI_F77_I4
GA_F2C_NOMAIN
ARMCI_F77_OPT

else

# enable_f77=no version of AC_F77_WRAPPERS
AC_DEFINE([F77_FUNC(name,NAME)], [name @%:@@%:@ _])
AC_DEFINE([F77_FUNC_(name,NAME)],[name @%:@@%:@ _])
# enable_f77=no version of GA_F2C_NOMAIN
AC_SUBST([FLD_NOMAIN], [])

fi

# This is required to override what automake does, if needed.
GA_F77_LD_OVERRIDE

AC_LANG_POP([Fortran 77])

###############################################################################
# Checks for programs.
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Checks for additional programs])
AC_MSG_NOTICE

AC_PROG_GREP
AC_PROG_SED

###############################################################################
# Libtool setup -- no compiler/linker tests after this
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Libtool setup])
AC_MSG_NOTICE
# temporarily restore unwrapped compilers
# this works around a bug where libtool sadly relies on matching compiler
# names in order to determine features (Fortran only, I think)
# libtool doesn't recognize MPI compiler names, nor should it
AS_IF([test x$with_mpi_wrappers = xyes],
    [AS_IF([test "x$ga_cv_mpif77_naked" != x],
           [ga_save_F77="$F77"; F77="$ga_cv_mpif77_naked"])])
GA_AR
LT_INIT([disable-shared])
# and now that that's over, put the MPI compilers back
# also, the above hack incorrectly sets the base compiler as the linker
AS_IF([test x$with_mpi_wrappers = xyes],
    [AS_IF([test "x$ga_cv_mpif77_naked" != x],
           [F77="$ga_save_F77"; compiler_F77="$ga_save_F77"])])

###############################################################################
# Remaining setup -- some tests, some individual components
###############################################################################
AC_MSG_NOTICE
AC_MSG_NOTICE([Miscellaneous remaining setup])
AC_MSG_NOTICE

# Establish some preprocessor symbols.
ARMCI_SETUP

# TODO not sure, but thus far required for 'dist' target at least
AM_CONDITIONAL([RANDOM_ACCESS], [test x != x]) # always false

# Whether to use PIC for the inline GCC code.
AM_CONDITIONAL([ENABLE_SHARED], [test x$enable_shared = xyes])

AS_IF([test "x$ga_cv_target" = xLINUX64],
    [AS_CASE([$host_cpu],
        [x86_64|ppc64|ia64],
            [AC_DEFINE([NEED_MEM_SYNC], [1], [Creates memfenc macro])])])
AS_IF([test "x$host_cpu" = xPWR4],
    [AC_DEFINE([NEED_MEM_SYNC], [1], [Creates memfenc macro])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

# Report on what we found.
AC_MSG_NOTICE([])
AC_MSG_NOTICE([**************************************************************])
AC_MSG_NOTICE([ $PACKAGE_NAME configured as follows:])
AC_MSG_NOTICE([**************************************************************])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([                TARGET=$ga_cv_target])
AC_MSG_NOTICE([             MSG_COMMS=$ga_msg_comms])
AC_MSG_NOTICE([            GA_MP_LIBS=$GA_MP_LIBS])
AC_MSG_NOTICE([         GA_MP_LDFLAGS=$GA_MP_LDFLAGS])
AC_MSG_NOTICE([        GA_MP_CPPFLAGS=$GA_MP_CPPFLAGS])
AC_MSG_NOTICE([         ARMCI_NETWORK=$ga_armci_network])
AC_MSG_NOTICE([ ARMCI_NETWORK_LDFLAGS=$ARMCI_NETWORK_LDFLAGS])
AC_MSG_NOTICE([    ARMCI_NETWORK_LIBS=$ARMCI_NETWORK_LIBS])
AC_MSG_NOTICE([ARMCI_NETWORK_CPPFLAGS=$ARMCI_NETWORK_CPPFLAGS])
AS_IF([test "x$enable_f77" = xyes], [
AC_MSG_NOTICE([                   F77=$F77])
AC_MSG_NOTICE([                FFLAGS=$FFLAGS])
AC_MSG_NOTICE([           FFLAG_INT_4=$FFLAG_INT_4])
AC_MSG_NOTICE([            ARMCI_FOPT=$ARMCI_FOPT])
])
AC_MSG_NOTICE([                    CC=$CC])
AC_MSG_NOTICE([                CFLAGS=$CFLAGS])
AC_MSG_NOTICE([            ARMCI_COPT=$ARMCI_COPT])
AS_IF([test "x$enable_cxx" = xyes], [
AC_MSG_NOTICE([                   CXX=$CXX])
AC_MSG_NOTICE([              CXXFLAGS=$CXXFLAGS])
AC_MSG_NOTICE([          ARMCI_CXXOPT=$ARMCI_CXXOPT])
])
AC_MSG_NOTICE([                   CPP=$CPP])
AC_MSG_NOTICE([              CPPFLAGS=$CPPFLAGS])
AC_MSG_NOTICE([               LDFLAGS=$LDFLAGS])
AC_MSG_NOTICE([                  LIBS=$LIBS])
AC_MSG_NOTICE([                 FLIBS=$FLIBS])
AC_MSG_NOTICE([                    AR=$AR])
AC_MSG_NOTICE([              AR_FLAGS=$AR_FLAGS])
AC_MSG_NOTICE([                  CCAS=$CCAS])
AC_MSG_NOTICE([            CCAS_FLAGS=$CCAS_FLAGS])
AC_MSG_NOTICE([                  DEFS=$DEFS])
AC_MSG_NOTICE([                 SHELL=$SHELL])
AC_MSG_NOTICE([])
