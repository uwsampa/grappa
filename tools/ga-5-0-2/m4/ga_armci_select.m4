# GA_ARMCI_SELECT(SHORT_NAME, LONG_NAME, FUNCTION, LIBRARY LIST,
                  HEADERFILE [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
#
# DESCRIPTION
#
#   Heavily modified version of AC_caolan_SEARCH_PACKAGE from autoconf
#   archives.
#
#   Provides --with-PACKAGE, --with-PACKAGE-include and
#   --with-PACKAGE-libdir options to configure. Supports the now standard
#   --with-PACKAGE=DIR approach where the package's include dir and lib dir
#   are underneath DIR, but also allows the include and lib directories to
#   be specified seperately
#
#   adds the extra -Ipath to CFLAGS if needed adds extra -Lpath to LD_FLAGS
#   if needed searches for the FUNCTION in each of the LIBRARY LIST with
#   AC_SEARCH_LIBRARY and thus adds the lib to LIBS
#
# LAST MODIFICATION
#
#   2008-04-12
#
# COPYLEFT
#
#   Copyright (c) 2008 Caolan McNamara <caolan@skynet.ie>
#   Copyright (c) 2008 Alexandre Duret-Lutz <adl@gnu.org>
#   Copyright (c) 2008 Matthew Mueller <donut@azstarnet.com>
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved.

AC_DEFUN([GA_ARMCI_SELECT],
[AC_ARG_WITH([$1],
    [AS_HELP_STRING([--with-$1[[=DIR]]], [select armci network as $2])],
    [with_$1=$withval
    AS_IF([test "${with_$1}" != yes],
        [$1_include="$withval/include"
        $1_libdir="$withval/lib"])],
    [with_$1=no])
AC_ARG_WITH([$1-include],
    [AS_HELP_STRING([--with-$1-include=DIR],
        [specify exact include dir for $2 headers])],
    [$1_include="$withval"
    AS_IF([test "${$1_include}" != no -a "${with_$1}" = no],
        [with_$1=yes])])
AC_ARG_WITH([$1-libdir],
    [AS_HELP_STRING([--with-$1-libdir=DIR],
        [specify exact library dir for $2 library])],
    [$1_libdir="$withval"
    AS_IF([test "${$1_libdir}" != no -a "${with_$1}" = no],
        [with_$1=yes])])
AS_IF([test "${with_$1}" != no],
    [OLD_LIBS=$LIBS
    OLD_LDFLAGS=$LDFLAGS
    OLD_CFLAGS=$CFLAGS
    OLD_CPPFLAGS=$CPPFLAGS
    AS_IF([test "${$1_libdir}"],
        [LDFLAGS="$LDFLAGS -L${$1_libdir}"])
    AS_IF([test "${$1_include}"],
        [CPPFLAGS="$CPPFLAGS -I${$1_include}"
        CFLAGS="$CFLAGS -I${$1_include}"])
    success=no
    AC_SEARCH_LIBS($3,$4,success=yes)
    AC_CHECK_HEADERS($5,success=yes)
    AS_IF([test "$success" = yes],
        [ifelse([$6], , , [$6])],
        [ifelse([$7], , , [$7])
        LIBS=$OLD_LIBS
        LDFLAGS=$OLD_LDFLAGS
        CPPFLAGS=$OLD_CPPFLAGS
        CFLAGS=$OLD_CFLAGS])])
])# GA_ARMCI_SELECT
