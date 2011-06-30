# GA_F77_CHECK_COMPILER_OPTION
# ----------------------------
# Check that a compiler option is accepted without warning messages.
#
# Synopsis:
# GA_F77_CHECK_COMPILER_OPTION(optionname,variable,action-if-ok,action-if-fail)
#
# Output Effects:
#
# If no actions are specified, a working value is added to the given
# variable.
#
# Notes:
# This is now careful to check that the output is different, since 
# some compilers are noisy.
# 
# We are extra careful to prototype the functions in case compiler options
# that complain about poor code are in effect.
#
# Because this is a long script, we have ensured that you can pass a 
# variable containing the option name as the first argument.
AC_DEFUN([GA_F77_CHECK_COMPILER_OPTION],
[ga_f77_check_compiler_option_result="no"
msg1="that Fortran compiler accepts option $1"
msg2="that routines compiled with $1 can be linked with ones compiled without"
save_FFLAGS="$FFLAGS"
FFLAGS="$1 $FFLAGS"
rm -f conftest.out
cat >conftest.f <<EOF
        program main
        end
EOF
rm -f conftest2.out
cat >conftest2.f <<EOF
        subroutine try()
        end
EOF
# It is important to use the AC_TRY_EVAL in case F77 is not a single word
# but is something like "f77 -64" where the switch has changed the
# compiler
ac_fscompilelink='$F77 $save_FFLAGS -o conftest conftest.f $LDFLAGS >conftest.bas 2>&1'
ac_fscompilelink2='$F77 $FFLAGS -o conftest conftest.f $LDFLAGS >conftest.out 2>&1'
ac_fscompile3='$F77 -c $save_FFLAGS conftest2.f >conftest2.bas 2>&1'
ac_fscompilelink4='$F77 $FFLAGS -o conftest conftest2.o conftest.f $LDFLAGS >conftest2.out 2>&1'
rm -f conftest.out
rm -f conftest.bas
rm -f conftest2.out
rm -f conftest2.bas
AC_MSG_CHECKING([$msg1])
AS_IF([AC_TRY_EVAL(ac_fscompilelink) && test -x conftest],
    [AS_IF([AC_TRY_EVAL(ac_fscompilelink2) && test -x conftest],
        [AS_IF([diff -b conftest.out conftest.bas >/dev/null 2>&1],
            [AC_MSG_RESULT(yes)
            AC_MSG_CHECKING([$msg2])       
            AS_IF([AC_TRY_EVAL(ac_fscompile3) && test -s conftest2.o],
                [AS_IF([AC_TRY_EVAL(ac_fscompilelink4) && test -x conftest],
                    [AS_IF([diff -b conftest2.out conftest.out >/dev/null 2>&1],
                        [ga_f77_check_compiler_option_result="yes"],
                        [echo "configure: Compiler output differed in two cases" >&AC_FD_CC
                        diff -b conftest2.out conftest.out >&AC_FD_CC])],
                    [echo "configure: failed program was:" >&AC_FD_CC
                    cat conftest2.f >&AC_FD_CC])],
                [echo "configure: failed program was:" >&AC_FD_CC
                cat conftest2.f >&AC_FD_CC])],
            [echo "configure: Compiler output differed in two cases" >&AC_FD_CC
            diff -b conftest.out conftest.bas >&AC_FD_CC])],
        [echo "configure: failed program was:" >&AC_FD_CC
        cat conftest.f >&AC_FD_CC
        AS_IF([test "$ga_f77_check_compiler_option_result" != "yes" -a -s conftest.out],
            [cat conftest.out >&AC_FD_CC])])],
    [echo "configure: Could not compile program" >&AC_FD_CC
    cat conftest.f >&AC_FD_CC
    cat conftest.bas >&AC_FD_CC])
AS_IF([test "$ga_f77_check_compiler_option_result" = "yes"],
    [AC_MSG_RESULT(yes)   
    ifelse([$3],,$2="$2 $1",$3)],
    [AC_MSG_RESULT(no)
    $4])
FFLAGS="$save_FFLAGS"
rm -f conftest*
])# GA_F77_CHECK_COMPILER_OPTION

