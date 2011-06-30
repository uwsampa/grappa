# _GA_ARMCI_NETWORK_WITH(KEY, DESCRIPTION)
# --------------------------------------------------
# A helper macro for generating all of the AC_ARG_WITHs.
# Also may establish value of ga_armci_network.
# Counts how many armci networks were specified by user.
AC_DEFUN([_GA_ARMCI_NETWORK_WITH], [
AC_ARG_WITH([$1],
    [AS_HELP_STRING([--with-$1[[=ARG]]], [select armci network as $2])])
AS_VAR_PUSHDEF([KEY],      m4_toupper(m4_translit([$1],      [-.], [__])))
AS_VAR_PUSHDEF([with_key],            m4_translit([with_$1], [-.], [__]))
dnl Can't have AM_CONDITIONAL here in case configure must find armci network
dnl without user intervention.
dnl AM_CONDITIONAL([ARMCI_NETWORK_]KEY, [test "x$with_key" != x])
AS_IF([test "x$with_key" != x],
    [GA_ARG_PARSE([with_key], [ARMCI_NETWORK_LIBS], [ARMCI_NETWORK_LDFLAGS],
                  [ARMCI_NETWORK_CPPFLAGS])])
AS_IF([test "x$with_key" != xno && test "x$with_key" != x],
    [ga_armci_network=KEY
     AS_VAR_ARITH([armci_network_count], [$armci_network_count + 1])])
AS_VAR_POPDEF([KEY])
AS_VAR_POPDEF([with_key])
])dnl

# _GA_ARMCI_NETWORK_WARN(KEY)
# ---------------------------
# Helper macro for idicating value of armci network arguments.
AC_DEFUN([_GA_ARMCI_NETWORK_WARN], [
AS_VAR_PUSHDEF([with_key],            m4_translit([with_$1], [-.], [__]))
AS_IF([test "x$with_key" != x && test "x$with_key" != xno],
    [AC_MSG_WARN([--with-$1=$with_key])])
AS_VAR_POPDEF([with_key])
])dnl

# _GA_ARMCI_NETWORK_AM_CONDITIONAL(KEY)
#--------------------------------------
# Helper macro for generating all AM_CONDITIONALs.
AC_DEFUN([_GA_ARMCI_NETWORK_AM_CONDITIONAL], [
AS_VAR_PUSHDEF([KEY],      m4_toupper(m4_translit([$1],      [-.], [__])))
AS_VAR_PUSHDEF([with_key],            m4_translit([with_$1], [-.], [__]))
AM_CONDITIONAL([ARMCI_NETWORK_]KEY,
    [test "x$with_key" != x && test "x$with_key" != xno])
AS_VAR_POPDEF([KEY])
AS_VAR_POPDEF([with_key])
])dnl

# _GA_ARMCI_NETWORK_BGML([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_BGML], [
AC_MSG_NOTICE([searching for BGML...])
happy=yes
dnl AS_IF([test "x$happy" = xyes],
dnl     [AS_IF([test -d /bgl/BlueLight/ppcfloor/bglsys], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([BGLML_memcpy], [msglayer.rts], [], [happy=no],
        [-lrts.rts -ldevices.rts])
     AS_CASE([$ac_cv_search_BGLML_memcpy],
        ["none required"], [],
        [no], [],
        [# add msglayer.rts to ARMCI_NETWORK_LIBS if not there
         AS_CASE([$ARMCI_NETWORK_LIBS],
                 [*msglayer.rts*], [],
                 [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -lmsglayer.rts"])
         # add extra lib rts.rts to ARMCI_NETWORK_LIBS if not there
         AS_CASE([$ARMCI_NETWORK_LIBS],
                 [*rts.rts*], [],
                 [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -lrts.rts"])
         # add extra lib devices.rts to ARMCI_NETWORK_LIBS if not there
         AS_CASE([$ARMCI_NETWORK_LIBS],
                 [*devices.rts*], [],
                 [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -ldevices.rts"])])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=BGML; with_bgml=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_CRAY_SHMEM([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_CRAY_SHMEM], [
AC_MSG_NOTICE([searching for CRAY_SHMEM...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([mpp/shmem.h], [],
        [AC_CHECK_HEADER([shmem.h], [], [happy=no])])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([shmem_init], [sma], [], [happy=no])
     AS_CASE([$ac_cv_search_shmem_init],
        ["none required"], [],
        [no], [],
        [# add sma to ARMCI_NETWORK_LIBS if not there
         AS_CASE([$ARMCI_NETWORK_LIBS],
                 [*sma*], [],
                 [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -lsma"])])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=CRAY_SHMEM; with_cray_shmem=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_DCMF([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_DCMF], [
AC_MSG_NOTICE([searching for DCMF...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([dcmf.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([DCMF_Messager_initialize], [dcmf.cnk],
        [], [happy=no], [-ldcmfcoll.cnk -lSPI.cna -lrt])
     AS_CASE([$ac_cv_search_DCMF_Messager_initialize],
            ["none required"], [],
            [no], [],
            [# add dcmf.cnk to ARMCI_NETWORK_LIBS if not there
             AS_CASE([$ARMCI_NETWORK_LIBS],
                     [*dcmf.cnk*], [],
                     [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -ldcmf.cnk"])
             # add extra lib dcmfcoll.cnk if not there
             AS_CASE([$ARMCI_NETWORK_LIBS],
                     [*dcmfcoll.cnk*], [],
                     [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -ldcmfcoll.cnk"])
             # add extra lib SPI.cna if not there
             AS_CASE([$ARMCI_NETWORK_LIBS],
                     [*SPI.cna*], [],
                     [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -lSPI.cna"])
             # add extra lib rt if not there
             AS_CASE([$ARMCI_NETWORK_LIBS],
                     [*rt*], [],
                     [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS -lrt"])])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=DCMF; with_dcmf=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_LAPI([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_LAPI], [
AC_MSG_NOTICE([searching for LAPI...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([lapi.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([LAPI_Init], [lapi_r lapi], [], [happy=no])
     AS_CASE([$ac_cv_search_LAPI_Init],
            ["none required"], [],
            [no], [],
            [# add missing lib to ARMCI_NETWORK_LIBS if not there
             AS_CASE([$ARMCI_NETWORK_LIBS],
                     [*$ac_cv_search_LAPI_Init*], [],
                     [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS $ac_cv_search_LAPI_Init"])])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=LAPI; with_lapi=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_MPI_SPAWN([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_MPI_SPAWN], [
AC_MSG_NOTICE([searching for MPI_SPAWN...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([mpi.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([MPI_Comm_spawn_multiple], [mpi mpich.cnk mpich.rts],
        [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=MPI_SPAWN; with_mpi_spawn=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_OPENIB([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ------------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_OPENIB], [
AC_MSG_NOTICE([searching for OPENIB...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([infiniband/verbs.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([ibv_open_device], [ibverbs], [], [happy=no])
     AS_CASE([$ac_cv_search_ibv_open_device],
        ["none required"], [],
        [no], [],
        [ARMCI_NETWORK_LIBS="$ARMCI_NETWORK_LIBS $ac_cv_search_ibv_open_device"])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=OPENIB; with_openib=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_PORTALS([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -------------------------------------------------------------------
AC_DEFUN([_GA_ARMCI_NETWORK_PORTALS], [
AC_MSG_NOTICE([searching for PORTALS...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([portals/portals3.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([portals/nal.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([PtlInit], [portals], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=PORTALS; with_portals=yes; $1],
    [$2])
])dnl

# _GA_ARMCI_NETWORK_GEMINI([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ------------------------------------------------------------------
# TODO when gemini headers and libraries become available, fix this
AC_DEFUN([_GA_ARMCI_NETWORK_GEMINI], [
AC_MSG_NOTICE([searching for GEMINI...])
happy=yes
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([numatoolkit.h], [], [happy=no], [
AC_INCLUDES_DEFAULT
#include <mpi.h>])])
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([NTK_Init], [numatoolkit], [], [happy=no])])
# CPPFLAGS must have CRAY_UGNI before looking for the next headers.
gemini_save_CPPFLAGS="$CPPFLAGS"; CPPFLAGS="$CPPFLAGS -DCRAY_UGNI"
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([onesided.h], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [AC_CHECK_HEADER([gni.h], [], [happy=no])])
CPPFLAGS="$gemini_save_CPPFLAGS"
AS_IF([test "x$happy" = xyes],
    [AC_SEARCH_LIBS([gniInit], [onesided], [], [happy=no])])
AS_IF([test "x$happy" = xyes],
    [ga_armci_network=GEMINI; with_gemini=yes; $1],
    [$2])
])dnl

# GA_ARMCI_NETWORK
# ----------------
# This macro allows user to choose the armci network but also allows the
# network to be tested for automatically.
AC_DEFUN([GA_ARMCI_NETWORK], [
# Clear the variables we will be using, just in case.
ga_armci_network=
ARMCI_NETWORK_LIBS=
ARMCI_NETWORK_LDFLAGS=
ARMCI_NETWORK_CPPFLAGS=
AC_ARG_ENABLE([autodetect],
    [AS_HELP_STRING([--enable-autodetect],
        [attempt to locate ARMCI_NETWORK besides SOCKETS])])
# First, all of the "--with" stuff is taken care of.
armci_network_count=0
_GA_ARMCI_NETWORK_WITH([bgml],      [IBM BG/L])
_GA_ARMCI_NETWORK_WITH([cray-shmem],[Cray XT shmem])
_GA_ARMCI_NETWORK_WITH([dcmf],      [IBM BG/P Deep Computing Message Framework])
_GA_ARMCI_NETWORK_WITH([lapi],      [IBM LAPI])
_GA_ARMCI_NETWORK_WITH([mpi-spawn], [MPI-2 dynamic process mgmt])
_GA_ARMCI_NETWORK_WITH([openib],    [Infiniband OpenIB])
_GA_ARMCI_NETWORK_WITH([portals],   [Cray XT portals])
_GA_ARMCI_NETWORK_WITH([gemini],    [Cray XE Gemini])
_GA_ARMCI_NETWORK_WITH([sockets],   [Ethernet TCP/IP])
# Temporarily add ARMCI_NETWORK_CPPFLAGS to CPPFLAGS.
ga_save_CPPFLAGS="$CPPFLAGS"; CPPFLAGS="$CPPFLAGS $ARMCI_NETWORK_CPPFLAGS"
# Temporarily add ARMCI_NETWORK_LDFLAGS to LDFLAGS.
ga_save_LDFLAGS="$LDFLAGS"; LDFLAGS="$LDFLAGS $ARMCI_NETWORK_LDFLAGS"
# Temporarily add ARMCI_NETWORK_LIBS to LIBS.
ga_save_LIBS="$LIBS"; LIBS="$ARMCI_NETWORK_LIBS $LIBS"
AS_IF([test "x$enable_autodetect" = xyes],
    [AC_MSG_NOTICE([searching for ARMCI_NETWORK...])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_bgml" != xno],
        [_GA_ARMCI_NETWORK_BGML()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_cray_shmem" != xno],
        [_GA_ARMCI_NETWORK_CRAY_SHMEM()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_dcmf" != xno],
        [_GA_ARMCI_NETWORK_DCMF()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_lapi" != xno],
        [_GA_ARMCI_NETWORK_LAPI()])
dnl     AS_IF([test "x$ga_armci_network" = x && test "x$with_mpi_spawn" != xno],
dnl         [_GA_ARMCI_NETWORK_MPI_SPAWN()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_openib" != xno],
        [_GA_ARMCI_NETWORK_OPENIB()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_portals" != xno],
        [_GA_ARMCI_NETWORK_PORTALS()])
     AS_IF([test "x$ga_armci_network" = x && test "x$with_gemini" != xno],
        [_GA_ARMCI_NETWORK_GEMINI()])
     AS_IF([test "x$ga_armci_network" = x],
        [AC_MSG_WARN([!!!])
         AC_MSG_WARN([No ARMCI_NETWORK detected, defaulting to SOCKETS])
         AC_MSG_WARN([!!!])
         ga_armci_network=SOCKETS; with_sockets=yes])],
    [# Not autodetecting
     # Check whether multiple armci networks were selected by user.
     AS_CASE([$armci_network_count],
        [0], [AC_MSG_WARN([No ARMCI_NETWORK specified, defaulting to SOCKETS])
              ga_armci_network=SOCKETS; with_sockets=yes],
        [1], [AS_IF([test "x$ga_armci_network" = xBGML],
                 [_GA_ARMCI_NETWORK_BGML([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=BGML failed])])])
              AS_IF([test "x$ga_armci_network" = xCRAY_SHMEM],
                 [_GA_ARMCI_NETWORK_CRAY_SHMEM([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=CRAY_SHMEM failed])])])
              AS_IF([test "x$ga_armci_network" = xDCMF],
                 [_GA_ARMCI_NETWORK_DCMF([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=DCMF failed])])])
              AS_IF([test "x$ga_armci_network" = xLAPI],
                 [_GA_ARMCI_NETWORK_LAPI([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=LAPI failed])])])
              AS_IF([test "x$ga_armci_network" = xMPI_SPAWN],
                 [_GA_ARMCI_NETWORK_MPI_SPAWN([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=MPI_SPAWN failed])])])
              AS_IF([test "x$ga_armci_network" = xOPENIB],
                 [_GA_ARMCI_NETWORK_OPENIB([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=OPENIB failed])])])
              AS_IF([test "x$ga_armci_network" = xPORTALS],
                 [_GA_ARMCI_NETWORK_PORTALS([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=PORTALS failed])])])
              AS_IF([test "x$ga_armci_network" = xGEMINI],
                 [_GA_ARMCI_NETWORK_GEMINI([],
                    [AC_MSG_ERROR([test for ARMCI_NETWORK=GEMINI failed])])])
             ],
        [AC_MSG_WARN([too many armci networks specified: $armci_network_count])
         AC_MSG_WARN([the following were specified:])
         _GA_ARMCI_NETWORK_WARN([bgml])
         _GA_ARMCI_NETWORK_WARN([cray-shmem])
         _GA_ARMCI_NETWORK_WARN([dcmf])
         _GA_ARMCI_NETWORK_WARN([lapi])
         _GA_ARMCI_NETWORK_WARN([mpi-spawn])
         _GA_ARMCI_NETWORK_WARN([openib])
         _GA_ARMCI_NETWORK_WARN([portals])
         _GA_ARMCI_NETWORK_WARN([gemini])
         _GA_ARMCI_NETWORK_WARN([sockets])
         AC_MSG_ERROR([please select only one armci network])])])
# Remove ARMCI_NETWORK_CPPFLAGS from CPPFLAGS.
CPPFLAGS="$ga_save_CPPFLAGS"
# Remove ARMCI_NETWORK_LDFLAGS from LDFLAGS.
LDFLAGS="$ga_save_LDFLAGS"
# Remove ARMCI_NETWORK_LIBS from LIBS.
LIBS="$ga_save_LIBS"
_GA_ARMCI_NETWORK_AM_CONDITIONAL([bgml])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([cray-shmem])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([dcmf])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([lapi])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([mpi-spawn])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([openib])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([gemini])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([portals])
_GA_ARMCI_NETWORK_AM_CONDITIONAL([sockets])
AC_SUBST([ARMCI_NETWORK_LDFLAGS])
AC_SUBST([ARMCI_NETWORK_LIBS])
AC_SUBST([ARMCI_NETWORK_CPPFLAGS])

# TODO
AM_CONDITIONAL([DCMF_VER_2],   [test x != x])  # always false
AM_CONDITIONAL([DCMF_VER_0_2], [test x != x]) # always false
AM_CONDITIONAL([DCMF_VER_0_3], [test x = x]) # always true

# permanent hack
AS_IF([test x$ga_armci_network = xPORTALS],
    [ARMCI_SRC_DIR=src-portals],
    [ARMCI_SRC_DIR=src])
AS_IF([test x$ga_armci_network = xGEMINI],
    [ARMCI_SRC_DIR=src-gemini
     AC_DEFINE([CRAY_UGNI], [1], [for Gemini])
     AC_DEFINE([LIBONESIDED], [1], [for Gemini])],
    [ARMCI_SRC_DIR=src])
AC_SUBST([ARMCI_SRC_DIR])

# tcgmsg5 requires this
AS_IF([test x$ga_armci_network = xLAPI],
[AC_DEFINE([NOTIFY_SENDER], [1],
    [this was defined unconditionally when using LAPI for tcgmsg 5])
AC_DEFINE([LAPI], [1], [tcgmsg 5 requires this when using LAPI])
])

ga_cray_xt_networks=no
AS_IF([test x$ga_armci_network = xPORTALS], [ga_cray_xt_networks=yes])
AS_IF([test x$ga_armci_network = xCRAY_SHMEM], [ga_cray_xt_networks=yes])
AM_CONDITIONAL([CRAY_XT_NETWORKS], [test x$ga_cray_xt_networks = xyes])

ga_cv_sysv_hack=no
# Only perform this hack for ARMCI build.
AS_IF([test "x$ARMCI_TOP_BUILDDIR" != x], [
    AS_IF([test x$ga_cv_sysv = xno],
        [AS_CASE([$ga_armci_network],
            [BGML|DCMF|PORTALS|GEMINI], [ga_cv_sysv_hack=no],
                [ga_cv_sysv_hack=yes])],
        [ga_cv_sysv_hack=yes])
AS_IF([test x$ga_cv_sysv_hack = xyes],
    [AC_DEFINE([SYSV], [1],
        [Defined if we want this system to use SYSV shared memory])])
])
AM_CONDITIONAL([SYSV], [test x$ga_cv_sysv_hack = xyes])
])dnl
