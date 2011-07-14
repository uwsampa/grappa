# GA_PROG_MPICXX
# --------------
# If desired, replace CXX with MPICXX while searching for a C++ compiler.
#
# Known C++ compilers:
#  aCC      HP-UX C++ compiler much better than `CC', so test before.
#  c++
#  cc++
#  CC
#  cl.exe
#  cxx
#  FCC      Fujitsu C++ compiler
#  g++
#  gpp
#  icpc     Intel C++ compiler
#  KCC      KAI C++ compiler
#  RCC      Rational C++
#  bgxlC    Intel
#  bgxlC_r  Intel, thread safe
#  xlC      AIX C Set++
#  xlC_r    AIX C Set++, thread safe
#  pgCC     Portland Group
#  pathCC   PathScale
#  sxc++    NEC SX
#  openCC   AMD's x86 open64
#  sunCC    Sun's Studio
#
# Known MPI C++ compilers
#  mpic++
#  mpicxx
#  mpiCC
#  sxmpic++     NEC SX
#  hcp
#  mpxlC_r
#  mpxlC
#  mpixlcxx_r
#  mpixlcxx
#  mpg++
#  mpc++
#  mpCC
#  cmpic++
#  mpiFCC       Fujitsu
#  CC
#
AC_DEFUN([GA_PROG_MPICXX],
[AC_ARG_VAR([MPICXX], [MPI C++ compiler])
AS_CASE([$ga_cv_target_base],
[BGP],  [ga_mpicxx_pref=mpixlcxx_r; ga_cxx_pref=bgxlC_r],
[])
# In the case of using MPI wrappers, set CXX=MPICXX since CXX will override
# absolutely everything in our list of compilers.
AS_IF([test x$with_mpi_wrappers = xyes],
    [ga_save_CXX="$CXX"
     CXX="$MPICXX"
     AS_IF([test "x$MPICXX" != x],
        [AS_IF([test "x$ga_save_CXX" != x],
            [AC_MSG_WARN([MPI compilers desired, MPICXX is set and CXX is set])
             AC_MSG_WARN([Choosing MPICXX over CXX])])],
        [AS_IF([test "x$ga_save_CXX" != x],
            [AC_MSG_WARN([MPI compilers desired but CXX is set, ignoring])
             AC_MSG_WARN([Perhaps you mean to set MPICXX instead?])])])])
ga_cxx="icpc pgCC pathCC sxc++ xlC_r xlC bgxlC_r bgxlC openCC sunCC g++ c++ gpp aCC CC cxx cc++ cl.exe FCC KCC RCC"
ga_mpicxx="mpic++ mpicxx mpiCC sxmpic++ hcp mpxlC_r mpxlC mpixlcxx_r mpixlcxx mpg++ mpc++ mpCC cmpic++ mpiFCC CC"
AS_IF([test x$with_mpi_wrappers = xyes],
    [CXX_TO_TEST="$ga_mpicxx_pref $ga_mpicxx"],
    [CXX_TO_TEST="$ga_cxx_pref $ga_cxx"])
AC_PROG_CXX([$CXX_TO_TEST])
])dnl
