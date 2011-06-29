# GA_ARG_PARSE(ARG, VAR_LIBS, VAR_LDFLAGS, VAR_CPPFLAGS)
# ------------------------------------------------------
# Parse whitespace-separated ARG into appropriate LIBS, LDFLAGS, and
# CPPFLAGS variables.
# TODO ADD test -d tests for *lib* and *include* cases??
AC_DEFUN([GA_ARG_PARSE],
[for arg in $$1 ; do
    AS_CASE([$arg],
        [yes],          [],
        [no],           [],
        [-l*],          [$2="$$2 $arg"],
        [-L*],          [$3="$$3 $arg"],
        [-WL*],         [$3="$$3 $arg"],
        [-Wl*],         [$3="$$3 $arg"],
        [-I*],          [$4="$$4 $arg"],
        [-*lib*],       [$3="$$3 $arg"],
        [*.a],          [$2="$$2 $arg"],
        [*.so],         [$2="$$2 $arg"],
        [*lib*],        [AS_IF([test -d $arg], [$3="$$3 -L$arg"])],
        [-*include*],   [$4="$$4 $arg"],
        [*include*],    [AS_IF([test -d $arg], [$4="$$4 -I$arg"])],
        [AS_IF([test -d $arg/lib], [$3="$$3 -L$arg/lib"])
         AS_IF([test -d $arg/include], [$4="$$4 -I$arg/include"])])
done])dnl
