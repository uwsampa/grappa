AC_DEFUN([ACVT_CUDAWRAP],
[
	check_cudart="yes"
	force_cudart="no"
	have_cudartwrap="no"

	cudartshlib_pathname=

	AC_ARG_ENABLE(cudawrap,
		AC_HELP_STRING([--enable-cudawrap],
		[enable support for tracing CUDA via library wrapping, default: enable if found by configure]),
	[AS_IF([test x"$enableval" = "xyes"], [force_cudart="yes"], [check_cudart="no"])])

	AS_IF([test "$check_cudart" = "yes"],
	[
		AC_REQUIRE([ACVT_CUDA])

		AS_IF([test x"$cudart_error" = "xno"],
		[
			AC_ARG_WITH(cudart-shlib,
				AC_HELP_STRING([--with-cudart-shlib=CUDARTSHLIB], [give the pathname for the shared CUDA runtime library, default: automatically by configure]),
			[
				AS_IF([test x"$withval" = "xyes" -o x"$withval" = "xno"],
				[AC_MSG_ERROR([value of '--with-cudart-shlib' not properly set])])
				cudartshlib_pathname=$withval
			])
		])

		AS_IF([test x"$cudart_error" = "xno"],
		[
			AC_MSG_CHECKING([for pathname of shared CUDA runtime library])

			AS_IF([test x"$cudartshlib_pathname" != x],
			[
				AC_MSG_RESULT([skipped (--with-cudart-shlib=$cudartshlib_pathname)])
			],
			[
				AS_IF([test x"$have_rtld_next" = "xyes"],
				[
					AC_MSG_RESULT([not needed])
				],
				[
					AS_IF([test x"$CUDATKLIBDIR" != x],
					[cudartlib_dir=`echo $CUDATKLIBDIR | sed s/\-L//`])
					cudartshlib_pathname=$cudartlib_dir`echo $CUDARTLIB | sed s/\-l/lib/`".so"

					AS_IF([! test -f $cudartshlib_pathname],
					[
						AC_MSG_RESULT([unknown])
						AC_MSG_NOTICE([error: could not determine pathname of shared CUDA runtime library])
						cudart_error="yes"
					],
					[
						AC_MSG_RESULT([$cudartshlib_pathname])
					])
				])
			])
		])

		AS_IF([test x"$cudart_error" = "xno"],
		[
			AS_IF([test x"$cudartshlib_pathname" != x],
			[
				AC_DEFINE_UNQUOTED([CUDARTSHLIB_PATHNAME],
				["$cudartshlib_pathname"], [pathname of shared CUDA runtime library])
			])
			have_cudartwrap="yes"
		])
	])
])
