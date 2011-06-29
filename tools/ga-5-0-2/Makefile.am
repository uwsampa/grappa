##############################################################################
# Makefile.am for Global Arrays.
#
# Rationale:
# This Makefile.am follows many of the suggestions outlined in the paper
# "Recursive Make Considered Harmful".  We do not use automake's
# 'include' feature (instead preferring a single, large Makefile.am).
# We do recurse into the armci directory, but since armci is a stand-alone
# package this is the correct behavior.
#
# Additional targets:
# Besides the traditional make targets supplied by automake, we have added the
# "checkprogs" target to build example programs and test programs.
#
# Notes:
# In general, each subdirectory has a corresponding section within this
# Makefile.am with the notable exception being the many examples getting
# rolled up into the global/examples section.
#

# The usual aclocal nonsense to get include paths right.
ACLOCAL_AMFLAGS = -I m4

# All public headers, installed programs, test programs, and example programs
# are listed in these variables.  Appended to throughout.  These are the
# automake variables used.
include_HEADERS =
bin_PROGRAMS =
check_PROGRAMS =
check_LTLIBRARIES =
lib_LTLIBRARIES = libga.la
libga_la_SOURCES =
libga_la_LIBADD =
nodist_libga_la_SOURCES =
EXTRA_DIST = README build-aux/inside.pl
BUILT_SOURCES =
CLEANFILES =
AM_FFLAGS =
AM_CFLAGS =
AM_CXXFLAGS =
AM_CPPFLAGS =
AM_LDFLAGS =
FLDFLAGS =
LDADD =

AM_FFLAGS += $(GA_FOPT)
AM_FFLAGS += $(FFLAG_INT)
AM_CFLAGS += $(GA_COPT)
AM_CXXFLAGS += $(GA_CXXOPT)

AM_CPPFLAGS += $(SCALAPACK_CPPFLAGS)
AM_CPPFLAGS += $(LAPACK_CPPFLAGS)
AM_CPPFLAGS += $(BLAS_CPPFLAGS)
AM_CPPFLAGS += $(GA_MP_CPPFLAGS)
AM_CPPFLAGS += $(ARMCI_NETWORK_CPPFLAGS)

AM_LDFLAGS += $(SCALAPACK_LDFLAGS)
AM_LDFLAGS += $(LAPACK_LDFLAGS)
AM_LDFLAGS += $(BLAS_LDFLAGS)
AM_LDFLAGS += $(GA_MP_LDFLAGS)
AM_LDFLAGS += $(ARMCI_NETWORK_LDFLAGS)
if NOFORT
else
AM_LDFLAGS += $(FLD_NOMAIN)
endif

FLDFLAGS += $(SCALAPACK_LDFLAGS)
FLDFLAGS += $(LAPACK_LDFLAGS)
FLDFLAGS += $(BLAS_LDFLAGS)
FLDFLAGS += $(GA_MP_LDFLAGS)
FLDFLAGS += $(ARMCI_NETWORK_LDFLAGS)

LDADD += $(LTLIBOBJS)
LDADD += libga.la
LDADD += $(SCALAPACK_LIBS)
LDADD += $(LAPACK_LIBS)
LDADD += $(BLAS_LIBS)
LDADD += $(GA_MP_LIBS)
LDADD += $(ARMCI_NETWORK_LIBS)

# Certain trickery when turning Fortran support on or off.
if NOFORT
else
MAYBE_DUMMY_F_LINK = dummy.f
MAYBE_FLIBS = $(FLIBS)
endif

##############################################################################
# compiler and linker flags
#
# Important for external tools wanting to know how to link to GA. This
# functionality caries over from the pre-autotools GA build.
SED_NORMALIZE_WHITESPACE = $(SED) 's/ [ ]*/ /g;s/" /"/g;s/ "/"/g'
.PHONY: flags
flags:
	@echo '# =========================================================================== '
	@echo '# Suggested compiler/linker options are as follows.'
	@echo '# GA libraries are installed in $(libdir)' 
	@echo '# GA headers are installed in $(includedir)' 
	@echo '#'
	@echo 'CPPFLAGS="$(SCALAPACK_CPPFLAGS) $(LAPACK_CPPFLAGS) $(BLAS_CPPFLAGS) $(GA_MP_CPPFLAGS) $(ARMCI_NETWORK_CPPFLAGS) -I$(includedir)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo '#'
	@echo 'LDFLAGS="$(SCALAPACK_LDFLAGS) $(LAPACK_LDFLAGS) $(BLAS_LDFLAGS) $(GA_MP_LDFLAGS) $(ARMCI_NETWORK_LDFLAGS) -L$(libdir)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo '#'
	@echo '# For Fortran Programs: '
	@echo 'FFLAGS="$(AM_FFLAGS)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo 'LIBS="-lga $(SCALAPACK_LIBS) $(LAPACK_LIBS) $(BLAS_LIBS) $(GA_MP_LIBS) $(ARMCI_NETWORK_LIBS)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo '#'
	@echo '# For C Programs: '
	@echo 'CFLAGS="$(AM_CFLAGS)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo 'LIBS="-lga $(SCALAPACK_LIBS) $(LAPACK_LIBS) $(BLAS_LIBS) $(GA_MP_LIBS) $(ARMCI_NETWORK_LIBS) $(MAYBE_FLIBS)"' | $(SED_NORMALIZE_WHITESPACE)
if CXX_BINDINGS
	@echo '#'
	@echo '# For C++ Programs: '
	@echo 'CXXFLAGS="$(AM_CXXFLAGS)"' | $(SED_NORMALIZE_WHITESPACE)
	@echo 'LIBS="-lga++ -lga $(SCALAPACK_LIBS) $(LAPACK_LIBS) $(BLAS_LIBS) $(GA_MP_LIBS) $(ARMCI_NETWORK_LIBS) $(MAYBE_FLIBS)"' | $(SED_NORMALIZE_WHITESPACE)
endif
	@echo '# =========================================================================== '

##############################################################################
# config
#
# The config.h file is automatically generated from config.h.in at configure
# time.  However, it contains symbols which certain Fortran compilers do not
# understand.  Therefore, we create a config.fh file which is Fortran-specific
# and works with all Fortran compilers.
#
BUILT_SOURCES += config.fh
CLEANFILES += config.fh
nodist_libga_la_SOURCES += config.fh

config.fh: config.h
	@-rm -f config.fh
	$(AM_V_SED) \
	$(SED) "/^#/!d" config.h > config.fh

##############################################################################
# compat
#
# Although the compat directory houses replacements for missing or erroneous
# standard C functions and such sources are conditionally compiled based on
# results from configure tests, without the "random" implementation the
# m4-generated tests always fail for scatter and copy_patch.
libga_la_SOURCES += compat/random.c

##############################################################################
# gaf2c
#
libga_la_SOURCES += gaf2c/gaf2c.c
libga_la_SOURCES += gaf2c/drand.c
if NOFORT
else
libga_la_SOURCES += gaf2c/farg.F
endif

include_HEADERS += gaf2c/typesf2c.h
include_HEADERS += gaf2c/farg.h

AM_CPPFLAGS += -I$(top_build_prefix)gaf2c
AM_CPPFLAGS += -I$(top_srcdir)/gaf2c

if NOFORT
else
check_PROGRAMS += gaf2c/testarg
endif

gaf2c_testarg_SOURCES = gaf2c/testarg.c gaf2c/testargf.f
gaf2c_testarg_LDFLAGS = $(FLDFLAGS)

##############################################################################
# ma
#
libga_la_SOURCES += ma/error.c
libga_la_SOURCES += ma/f2c.c
libga_la_SOURCES += ma/ma.c
libga_la_SOURCES += ma/ma.h
libga_la_SOURCES += ma/memcpy.h
libga_la_SOURCES += ma/string-util.c
libga_la_SOURCES += ma/table.c
if NOFORT
else
libga_la_SOURCES += ma/maf.F
endif

include_HEADERS += ma/error.h
include_HEADERS += ma/macdecls.h
include_HEADERS += ma/macommon.h
include_HEADERS += ma/maf2c.fh
include_HEADERS += ma/mafdecls.fh
include_HEADERS += ma/matypes.h
include_HEADERS += ma/scope.h
include_HEADERS += ma/string-util.h
include_HEADERS += ma/table.h

AM_CPPFLAGS += -I$(top_build_prefix)ma
AM_CPPFLAGS += -I$(top_srcdir)/ma

check_PROGRAMS += ma/testc
check_PROGRAMS += ma/test-coalesce
check_PROGRAMS += ma/test-inquire
if NOFORT
else
check_PROGRAMS += ma/testf
endif

ma_testf_SOURCES         = ma/testf.F
ma_testc_SOURCES         = ma/testc.c
ma_test_coalesce_SOURCES = ma/test-coalesce.c
ma_test_inquire_SOURCES  = ma/test-inquire.c

ma_testf_LDFLAGS = $(FLDFLAGS)

nodist_EXTRA_ma_testc_SOURCES         = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_ma_test_coalesce_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_ma_test_inquire_SOURCES  = $(MAYBE_DUMMY_F_LINK)

##############################################################################
# LinAlg/lapack+blas
#
if NOFORT
else # !NOFORT
if HAVE_LAPACK
else # !HAVE_LAPACK
libga_la_SOURCES += LinAlg/lapack+blas/dgetf2.f
libga_la_SOURCES += LinAlg/lapack+blas/dgetrf.f
libga_la_SOURCES += LinAlg/lapack+blas/dgetrs.f
libga_la_SOURCES += LinAlg/lapack+blas/dlacpy.f
libga_la_SOURCES += LinAlg/lapack+blas/dlaset.f
libga_la_SOURCES += LinAlg/lapack+blas/dlaswp.f
libga_la_SOURCES += LinAlg/lapack+blas/ilaenv.f
libga_la_SOURCES += LinAlg/lapack+blas/lsame.f
libga_la_SOURCES += LinAlg/lapack+blas/xerbla.f
endif # HAVE_LAPACK
if HAVE_BLAS
else # !HAVE_BLAS
libga_la_SOURCES += LinAlg/lapack+blas/cgemm.f
libga_la_SOURCES += LinAlg/lapack+blas/dblas.f
libga_la_SOURCES += LinAlg/lapack+blas/sgemm.f
libga_la_SOURCES += LinAlg/lapack+blas/zaxpy.f
libga_la_SOURCES += LinAlg/lapack+blas/zgemm.f
endif # HAVE_BLAS
endif # NOFORT

EXTRA_DIST += LinAlg/lapack+blas/README

##############################################################################
# tcgmsg
#
if MSG_COMMS_TCGMSG
libga_la_SOURCES += tcgmsg/capi.c
if USE_VAMPIR
libga_la_SOURCES += tcgmsg/tcgmsg_vampir.c
libga_la_SOURCES += tcgmsg/tcgmsg_vampir.h
endif

include_HEADERS += tcgmsg/msgtypesc.h
include_HEADERS += tcgmsg/msgtypesf.h
include_HEADERS += tcgmsg/tcgmsg.fh
include_HEADERS += tcgmsg/tcgmsg.h

AM_CPPFLAGS += -I$(top_srcdir)/tcgmsg

endif # MSG_COMMS_TCGMSG

EXTRA_DIST += tcgmsg/README

##############################################################################
# tcgmsg/examples
#
# NOTE: While we don't build the tcgmsg examples, we include everything in the
# distribution anyway.
#
if MSG_COMMS_TCGMSG

endif # MSG_COMMS_TCGMSG

EXTRA_DIST += tcgmsg/examples/blkdat120lin.f
EXTRA_DIST += tcgmsg/examples/blkdat15.f
EXTRA_DIST += tcgmsg/examples/blkdat240lin.f
EXTRA_DIST += tcgmsg/examples/blkdat30.f
EXTRA_DIST += tcgmsg/examples/blkdat60.f
EXTRA_DIST += tcgmsg/examples/blkdat60lin.f
EXTRA_DIST += tcgmsg/examples/cscf120lin.h
EXTRA_DIST += tcgmsg/examples/cscf15.h
EXTRA_DIST += tcgmsg/examples/cscf240lin.h
EXTRA_DIST += tcgmsg/examples/cscf30.h
EXTRA_DIST += tcgmsg/examples/cscf60.h
EXTRA_DIST += tcgmsg/examples/cscf60lin.h
EXTRA_DIST += tcgmsg/examples/cscf.h
EXTRA_DIST += tcgmsg/examples/daxpy1.s
EXTRA_DIST += tcgmsg/examples/daxpy.f
EXTRA_DIST += tcgmsg/examples/ddot.f
EXTRA_DIST += tcgmsg/examples/demo.proto
EXTRA_DIST += tcgmsg/examples/diagon.f
EXTRA_DIST += tcgmsg/examples/dscal.f
EXTRA_DIST += tcgmsg/examples/fexit.f.proto
EXTRA_DIST += tcgmsg/examples/getmem.c
EXTRA_DIST += tcgmsg/examples/grid.c
EXTRA_DIST += tcgmsg/examples/ieeetrap.c
EXTRA_DIST += tcgmsg/examples/integ.f
EXTRA_DIST += tcgmsg/examples/jacobi.f
EXTRA_DIST += tcgmsg/examples/Makefile.proto
EXTRA_DIST += tcgmsg/examples/mc.f
EXTRA_DIST += tcgmsg/examples/mc.input
EXTRA_DIST += tcgmsg/examples/md.f
EXTRA_DIST += tcgmsg/examples/mxv_daxpy1.f
EXTRA_DIST += tcgmsg/examples/mxv_dgemv.f
EXTRA_DIST += tcgmsg/examples/mxv_fortran.f
EXTRA_DIST += tcgmsg/examples/output.f
EXTRA_DIST += tcgmsg/examples/prtri.f
EXTRA_DIST += tcgmsg/examples/random.c
EXTRA_DIST += tcgmsg/examples/README
EXTRA_DIST += tcgmsg/examples/runit
EXTRA_DIST += tcgmsg/examples/runit.grid
EXTRA_DIST += tcgmsg/examples/scfblas.f
EXTRA_DIST += tcgmsg/examples/scf.f
EXTRA_DIST += tcgmsg/examples/timer.f
EXTRA_DIST += tcgmsg/examples/trace.out
EXTRA_DIST += tcgmsg/examples/xpix.shar

##############################################################################
# tcgmsg/ipcv4.0
#
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG4
libga_la_SOURCES += tcgmsg/ipcv4.0/brdcst.c
libga_la_SOURCES += tcgmsg/ipcv4.0/checkbyte.c
libga_la_SOURCES += tcgmsg/ipcv4.0/cluster.c
libga_la_SOURCES += tcgmsg/ipcv4.0/cluster.h
libga_la_SOURCES += tcgmsg/ipcv4.0/defglobals.h
libga_la_SOURCES += tcgmsg/ipcv4.0/drand48.c
libga_la_SOURCES += tcgmsg/ipcv4.0/error.c
libga_la_SOURCES += tcgmsg/ipcv4.0/evlog.c
libga_la_SOURCES += tcgmsg/ipcv4.0/evlog.h
libga_la_SOURCES += tcgmsg/ipcv4.0/evon.c
libga_la_SOURCES += tcgmsg/ipcv4.0/globalop.c
libga_la_SOURCES += tcgmsg/ipcv4.0/llog.c
libga_la_SOURCES += tcgmsg/ipcv4.0/mdtob.c
libga_la_SOURCES += tcgmsg/ipcv4.0/mdtoi.c
libga_la_SOURCES += tcgmsg/ipcv4.0/mitob.c
libga_la_SOURCES += tcgmsg/ipcv4.0/mitod.c
libga_la_SOURCES += tcgmsg/ipcv4.0/mtime.c
libga_la_SOURCES += tcgmsg/ipcv4.0/niceftn.c
libga_la_SOURCES += tcgmsg/ipcv4.0/nnodes.c
libga_la_SOURCES += tcgmsg/ipcv4.0/nodeid.c
libga_la_SOURCES += tcgmsg/ipcv4.0/nxtval.c
libga_la_SOURCES += tcgmsg/ipcv4.0/pbegin.c
libga_la_SOURCES += tcgmsg/ipcv4.0/pbeginf.c
libga_la_SOURCES += tcgmsg/ipcv4.0/pfilecopy.c
libga_la_SOURCES += tcgmsg/ipcv4.0/sema.c
libga_la_SOURCES += tcgmsg/ipcv4.0/sema.h
libga_la_SOURCES += tcgmsg/ipcv4.0/setdbg.c
libga_la_SOURCES += tcgmsg/ipcv4.0/shmem.c
libga_la_SOURCES += tcgmsg/ipcv4.0/shmem.h
libga_la_SOURCES += tcgmsg/ipcv4.0/signals.c
libga_la_SOURCES += tcgmsg/ipcv4.0/signals.h
libga_la_SOURCES += tcgmsg/ipcv4.0/snd.c
libga_la_SOURCES += tcgmsg/ipcv4.0/sndrcv.h
libga_la_SOURCES += tcgmsg/ipcv4.0/sndrcvP.h
libga_la_SOURCES += tcgmsg/ipcv4.0/sockets.c
libga_la_SOURCES += tcgmsg/ipcv4.0/sockets.h
libga_la_SOURCES += tcgmsg/ipcv4.0/srftoc.h
libga_la_SOURCES += tcgmsg/ipcv4.0/srmover.c
libga_la_SOURCES += tcgmsg/ipcv4.0/stats.c
libga_la_SOURCES += tcgmsg/ipcv4.0/synch.c
libga_la_SOURCES += tcgmsg/ipcv4.0/usleep.c
libga_la_SOURCES += tcgmsg/ipcv4.0/waitall.c
libga_la_SOURCES += tcgmsg/ipcv4.0/waitcom.c
libga_la_SOURCES += tcgmsg/ipcv4.0/xdrstuff.c
libga_la_SOURCES += tcgmsg/ipcv4.0/xdrstuff.h

AM_CPPFLAGS += -I$(top_srcdir)/tcgmsg/ipcv4.0

check_PROGRAMS += tcgmsg/ipcv4.0/hello
check_PROGRAMS += tcgmsg/ipcv4.0/test
check_PROGRAMS += tcgmsg/ipcv4.0/testmap
check_PROGRAMS += tcgmsg/ipcv4.0/toplot
check_PROGRAMS += tcgmsg/ipcv4.0/xdrtest
if NOFORT
else
check_PROGRAMS += tcgmsg/ipcv4.0/testf
check_PROGRAMS += tcgmsg/ipcv4.0/testpf
endif

tcgmsg_ipcv4_0_hello_SOURCES   = tcgmsg/ipcv4.0/hello.c
tcgmsg_ipcv4_0_test_SOURCES    = tcgmsg/ipcv4.0/test.c
tcgmsg_ipcv4_0_testf_SOURCES   = tcgmsg/ipcv4.0/testf.F
tcgmsg_ipcv4_0_testmap_SOURCES = tcgmsg/ipcv4.0/testmap.c
tcgmsg_ipcv4_0_testpf_SOURCES  = tcgmsg/ipcv4.0/testpf.f
tcgmsg_ipcv4_0_toplot_SOURCES  = tcgmsg/ipcv4.0/toplot.c
tcgmsg_ipcv4_0_xdrtest_SOURCES = tcgmsg/ipcv4.0/xdrtest.c

tcgmsg_ipcv4_0_testf_LDFLAGS   = $(FLDFLAGS)
tcgmsg_ipcv4_0_testpf_LDFLAGS  = $(FLDFLAGS)

nodist_EXTRA_tcgmsg_ipcv4_0_hello_SOURCES   = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv4_0_test_SOURCES    = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv4_0_testmap_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv4_0_toplot_SOURCES  = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv4_0_xdrtest_SOURCES = $(MAYBE_DUMMY_F_LINK)

bin_PROGRAMS += parallel

parallel_SOURCES = tcgmsg/ipcv4.0/parallel.c
nodist_EXTRA_parallel_SOURCES = $(MAYBE_DUMMY_F_LINK)

endif # MSG_COMMS_TCGMSG4
endif # MSG_COMMS_TCGMSG

EXTRA_DIST += tcgmsg/ipcv4.0/README

##############################################################################
# tcgmsg/ipcv5.0
#
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG5
libga_la_SOURCES += tcgmsg/ipcv5.0/drand48.c
libga_la_SOURCES += tcgmsg/ipcv5.0/evlog.h
libga_la_SOURCES += tcgmsg/ipcv5.0/globalop.c
libga_la_SOURCES += tcgmsg/ipcv5.0/llog.c
libga_la_SOURCES += tcgmsg/ipcv5.0/mdtob.c
libga_la_SOURCES += tcgmsg/ipcv5.0/mdtoi.c
libga_la_SOURCES += tcgmsg/ipcv5.0/misc.c
libga_la_SOURCES += tcgmsg/ipcv5.0/mitob.c
libga_la_SOURCES += tcgmsg/ipcv5.0/mitod.c
libga_la_SOURCES += tcgmsg/ipcv5.0/mtime.c
libga_la_SOURCES += tcgmsg/ipcv5.0/niceftn.c
libga_la_SOURCES += tcgmsg/ipcv5.0/nnodes.c
libga_la_SOURCES += tcgmsg/ipcv5.0/nodeid.c
libga_la_SOURCES += tcgmsg/ipcv5.0/pbegin.c
libga_la_SOURCES += tcgmsg/ipcv5.0/pfilecopy.c
libga_la_SOURCES += tcgmsg/ipcv5.0/queues.c
libga_la_SOURCES += tcgmsg/ipcv5.0/shmem.h
libga_la_SOURCES += tcgmsg/ipcv5.0/signals.c
libga_la_SOURCES += tcgmsg/ipcv5.0/sndrcv.h
libga_la_SOURCES += tcgmsg/ipcv5.0/snd_rcv_probe.c
libga_la_SOURCES += tcgmsg/ipcv5.0/srftoc.h
libga_la_SOURCES += tcgmsg/ipcv5.0/tcgmsgP.h
libga_la_SOURCES += tcgmsg/ipcv5.0/usleep.c
libga_la_SOURCES += tcgmsg/ipcv5.0/waitall.c
if ARMCI_NETWORK_LAPI
libga_la_SOURCES += tcgmsg/ipcv5.0/async_send_lapi.c
libga_la_SOURCES += tcgmsg/ipcv5.0/lapi_putget.c
else
libga_la_SOURCES += tcgmsg/ipcv5.0/async_send.c
libga_la_SOURCES += tcgmsg/ipcv5.0/busy.c
libga_la_SOURCES += tcgmsg/ipcv5.0/error.c
libga_la_SOURCES += tcgmsg/ipcv5.0/nxtval.shm.c
libga_la_SOURCES += tcgmsg/ipcv5.0/pbeginf.c
libga_la_SOURCES += tcgmsg/ipcv5.0/shmem.c
libga_la_SOURCES += tcgmsg/ipcv5.0/synch.c
endif # ARMCI_NETWORK_LAPI

AM_CPPFLAGS += -I$(top_srcdir)/tcgmsg/ipcv5.0

check_PROGRAMS += tcgmsg/ipcv5.0/hello
check_PROGRAMS += tcgmsg/ipcv5.0/test
if MSG_COMMS_PVM
check_PROGRAMS += tcgmsg/ipcv5.0/pvm.ring
endif
if NOFORT
else
check_PROGRAMS += tcgmsg/ipcv5.0/jacobi
check_PROGRAMS += tcgmsg/ipcv5.0/testf
endif

tcgmsg_ipcv5_0_hello_SOURCES    = tcgmsg/ipcv5.0/hello.c
tcgmsg_ipcv5_0_jacobi_SOURCES   = tcgmsg/ipcv5.0/fexit.f       \
                                  tcgmsg/ipcv5.0/getmem.c      \
                                  tcgmsg/ipcv5.0/jacobi.F      \
                                  tcgmsg/ipcv5.0/mxv_fortran.f \
                                  tcgmsg/ipcv5.0/timer.f
tcgmsg_ipcv5_0_pvm_ring_SOURCES = tcgmsg/ipcv5.0/checkbyte.c   \
                                  tcgmsg/ipcv5.0/pvm.c
tcgmsg_ipcv5_0_testf_SOURCES    = tcgmsg/ipcv5.0/testf.F
tcgmsg_ipcv5_0_test_SOURCES     = tcgmsg/ipcv5.0/checkbyte.c   \
                                  tcgmsg/ipcv5.0/test.c

tcgmsg_ipcv5_0_jacobi_LDFLAGS   = $(FLDFLAGS)
tcgmsg_ipcv5_0_testf_LDFLAGS    = $(FLDFLAGS)

nodist_EXTRA_tcgmsg_ipcv5_0_hello_SOURCES    = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv5_0_pvm_ring_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_ipcv5_0_test_SOURCES     = $(MAYBE_DUMMY_F_LINK)

endif # MSG_COMMS_TCGMSG5
endif # MSG_COMMS_TCGMSG

##############################################################################
# tcgmsg/tcgmsg-mpi
#
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSGMPI
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/checkbyte.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/clustercheck.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/collect.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/drand48.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/evlog.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/evlog.h
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/evon.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/llog.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/misc.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/niceftn.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/nxtval-armci.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/p2p.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/pbeginf.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/pfilecopy.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/sizeof.c
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/sndrcv.h
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/srftoc.h
libga_la_SOURCES += tcgmsg/tcgmsg-mpi/tcgmsgP.h

AM_CPPFLAGS += -I$(top_srcdir)/tcgmsg/tcgmsg-mpi

check_PROGRAMS += tcgmsg/tcgmsg-mpi/test_asyn
check_PROGRAMS += tcgmsg/tcgmsg-mpi/test
if NOFORT
else
check_PROGRAMS += tcgmsg/tcgmsg-mpi/testf
endif

tcgmsg_tcgmsg_mpi_test_asyn_SOURCES = tcgmsg/tcgmsg-mpi/test_asyn.c
tcgmsg_tcgmsg_mpi_testf_SOURCES     = tcgmsg/tcgmsg-mpi/testf.F
tcgmsg_tcgmsg_mpi_test_SOURCES      = tcgmsg/tcgmsg-mpi/test.c

tcgmsg_tcgmsg_mpi_testf_LDFLAGS     = $(FLDFLAGS)

nodist_EXTRA_tcgmsg_tcgmsg_mpi_test_asyn_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_tcgmsg_tcgmsg_mpi_test_SOURCES      = $(MAYBE_DUMMY_F_LINK)

endif # MSG_COMMS_TCGMSGMPI
endif # MSG_COMMS_TCGMSG

EXTRA_DIST += tcgmsg/tcgmsg-mpi/README

##############################################################################
# global
#
EXTRA_DIST += global/README

##############################################################################
# global/src
#
libga_la_SOURCES += global/src/base.c
libga_la_SOURCES += global/src/base.h
libga_la_SOURCES += global/src/capi.c
libga_la_SOURCES += global/src/c.names.h
libga_la_SOURCES += global/src/collect.c
libga_la_SOURCES += global/src/datatypes.c
libga_la_SOURCES += global/src/decomp.c
libga_la_SOURCES += global/src/diag.fh
libga_la_SOURCES += global/src/DP.c
libga_la_SOURCES += global/src/elem_alg.c
libga_la_SOURCES += global/src/ga_ckpt.h
libga_la_SOURCES += global/src/gaconfig.h
libga_la_SOURCES += global/src/ga_malloc.c
libga_la_SOURCES += global/src/ga_profile.h
libga_la_SOURCES += global/src/ga_solve_seq.c
libga_la_SOURCES += global/src/ga_symmetr.c
libga_la_SOURCES += global/src/ga_trace.c
libga_la_SOURCES += global/src/ga_vampir.fh
libga_la_SOURCES += global/src/ga_vampir.h
libga_la_SOURCES += global/src/ghosts.c
libga_la_SOURCES += global/src/global.h
libga_la_SOURCES += global/src/global.nalg.c
libga_la_SOURCES += global/src/global.npatch.c
libga_la_SOURCES += global/src/global.periodic.c
libga_la_SOURCES += global/src/globalp.h
libga_la_SOURCES += global/src/global.util.c
libga_la_SOURCES += global/src/hsort.scat.c
libga_la_SOURCES += global/src/matmul.c
libga_la_SOURCES += global/src/matmul.h
libga_la_SOURCES += global/src/matrix.c
libga_la_SOURCES += global/src/nbutil.c
libga_la_SOURCES += global/src/onesided.c
libga_la_SOURCES += global/src/scalapack.fh
libga_la_SOURCES += global/src/select.c
libga_la_SOURCES += global/src/sparse.c
libga_la_SOURCES += global/src/xgemm.c
if NOFORT
else
libga_la_SOURCES += global/src/complex.F
libga_la_SOURCES += global/src/ga_diag_seq.F
libga_la_SOURCES += global/src/rsg.f
endif
if ENABLE_CHECKPOINT
libga_la_SOURCES += global/src/ga_ckpt.c
endif
if CRAY_XT_NETWORKS
libga_la_SOURCES += global/src/ga_dgemmf.F
endif
if ENABLE_PEIGS
libga_la_SOURCES += global/src/ga_diag.F
else
libga_la_SOURCES += global/src/peigstubs.c
endif
if HAVE_SCALAPACK
libga_la_SOURCES += global/src/scalapack.F
else
libga_la_SOURCES += global/src/sclstubs.c
endif
if USE_VAMPIR
libga_la_SOURCES += global/ga_vt.c
libga_la_SOURCES += global/ga_vt.h
libga_la_SOURCES += global/src/ga_vampir.c
endif

include_HEADERS += global/src/gacommon.h
include_HEADERS += global/src/ga.h
include_HEADERS += global/src/global.fh

AM_CPPFLAGS += -I$(top_build_prefix)global/src
AM_CPPFLAGS += -I$(top_srcdir)/global/src

##############################################################################
# global/testing
#
gtsrc =
gtsrc += global/testing/testutil.fh
gtsrc += global/testing/testutil.h
gtsrc += global/testing/util.c
if NOFORT
else
gtsrc += global/testing/ffflush.F
gtsrc += global/testing/testblas.f
endif

check_PROGRAMS += global/testing/elempatch
check_PROGRAMS += global/testing/getmem
check_PROGRAMS += global/testing/mtest
check_PROGRAMS += global/testing/ntestc
check_PROGRAMS += global/testing/ntestfc
check_PROGRAMS += global/testing/packc
check_PROGRAMS += global/testing/patch_enumc
check_PROGRAMS += global/testing/print
check_PROGRAMS += global/testing/scan_addc
check_PROGRAMS += global/testing/scan_copyc
check_PROGRAMS += global/testing/testc
check_PROGRAMS += global/testing/testmult
check_PROGRAMS += global/testing/unpackc
if NOFORT
if HAVE_LAPACK
check_PROGRAMS += global/testing/ga_lu
endif
else
check_PROGRAMS += global/testing/bin
check_PROGRAMS += global/testing/blktest
check_PROGRAMS += global/testing/d2test
check_PROGRAMS += global/testing/g2test
check_PROGRAMS += global/testing/g3test
check_PROGRAMS += global/testing/ga_lu
check_PROGRAMS += global/testing/ghosts
check_PROGRAMS += global/testing/jacobi
check_PROGRAMS += global/testing/mir_perf2
check_PROGRAMS += global/testing/mmatrix
check_PROGRAMS += global/testing/mulmatpatch
check_PROGRAMS += global/testing/nb2test
check_PROGRAMS += global/testing/ndim
check_PROGRAMS += global/testing/patch
check_PROGRAMS += global/testing/patch_enumf
check_PROGRAMS += global/testing/perfmod
check_PROGRAMS += global/testing/perform
check_PROGRAMS += global/testing/perf
check_PROGRAMS += global/testing/pg2testmatmult
check_PROGRAMS += global/testing/pg2test
check_PROGRAMS += global/testing/pgtestmatmult
check_PROGRAMS += global/testing/pgtest
check_PROGRAMS += global/testing/random
check_PROGRAMS += global/testing/scan
check_PROGRAMS += global/testing/simple_groups
check_PROGRAMS += global/testing/sparse
check_PROGRAMS += global/testing/sprsmatmult
check_PROGRAMS += global/testing/stride
check_PROGRAMS += global/testing/testeig
check_PROGRAMS += global/testing/testmatmult
check_PROGRAMS += global/testing/testsolve
check_PROGRAMS += global/testing/test
if HAVE_SCALAPACK
check_PROGRAMS += global/testing/testspd
endif
endif
if MSG_COMMS_MPI
check_PROGRAMS += global/testing/ga-mpi
check_PROGRAMS += global/testing/lock
endif
if SYSV
check_PROGRAMS += global/testing/ipc_clean
endif

if HAVE_M4
GLOBAL_TESTING_M4_DEPS =
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_GA_FILL.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_main.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_ACC.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_ADD_PATCH.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_COPY_PATCH.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_DOT_PATCH.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_FILL_PATCH.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_GATHER.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_GET.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_PERIODIC_ACC.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_PERIODIC_GET.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_PERIODIC_PUT.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_PUT.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_SCALE_PATCH.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_SCATTER_ACC.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_NGA_SCATTER.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_util_comm.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ndim_util.src
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest_src/ngatest.def
GLOBAL_TESTING_M4_DEPS += global/testing/nga-onesided.m4
GLOBAL_TESTING_M4_DEPS += global/testing/nga-patch.m4
GLOBAL_TESTING_M4_DEPS += global/testing/nga-periodic.m4
GLOBAL_TESTING_M4_DEPS += global/testing/nga-scatter.m4
GLOBAL_TESTING_M4_DEPS += global/testing/ngatest.m4
GLOBAL_TESTING_M4_DEPS += global/testing/nga-util.m4

EXTRA_DIST += $(GLOBAL_TESTING_M4_DEPS)
EXTRA_DIST += global/testing/README

global/testing/nga-onesided.F: $(GLOBAL_TESTING_M4_DEPS)
global/testing/nga-patch.F:    $(GLOBAL_TESTING_M4_DEPS)
global/testing/nga-periodic.F: $(GLOBAL_TESTING_M4_DEPS)
global/testing/nga-scatter.F:  $(GLOBAL_TESTING_M4_DEPS)
global/testing/nga-util.F:     $(GLOBAL_TESTING_M4_DEPS)
global/testing/ngatest.F:      $(GLOBAL_TESTING_M4_DEPS)

CLEANFILES += global/testing/nga-onesided.F
CLEANFILES += global/testing/nga-patch.F
CLEANFILES += global/testing/nga-periodic.F
CLEANFILES += global/testing/nga-scatter.F
CLEANFILES += global/testing/nga-util.F
CLEANFILES += global/testing/ngatest.F

if NOFORT
else
check_PROGRAMS += global/testing/nga-onesided
check_PROGRAMS += global/testing/nga-patch
check_PROGRAMS += global/testing/nga-periodic
check_PROGRAMS += global/testing/nga-scatter
check_PROGRAMS += global/testing/nga-util
check_PROGRAMS += global/testing/ngatest
endif
.m4.F:
	$(AM_V_M4) \
	$(M4) -I$(top_srcdir)/global/testing $< > $@ 
endif

global_testing_bin_SOURCES            = global/testing/bin.F $(gtsrc)
global_testing_blktest_SOURCES        = global/testing/blktest.F $(gtsrc)
global_testing_d2test_SOURCES         = global/testing/d2test.F $(gtsrc)
global_testing_elempatch_SOURCES      = global/testing/elempatch.c $(gtsrc)
global_testing_g2test_SOURCES         = global/testing/g2test.F $(gtsrc)
global_testing_g3test_SOURCES         = global/testing/g3test.F $(gtsrc)
global_testing_ga_lu_SOURCES          = global/testing/ga_lu.c $(gtsrc)
global_testing_ga_mpi_SOURCES         = global/testing/ga-mpi.c $(gtsrc)
global_testing_getmem_SOURCES         = global/testing/getmem.c $(gtsrc)
global_testing_ghosts_SOURCES         = global/testing/ghosts.F $(gtsrc)
global_testing_ipc_clean_SOURCES      = global/testing/ipc.clean.c $(gtsrc)
global_testing_jacobi_SOURCES         = global/testing/jacobi.F $(gtsrc)
global_testing_lock_SOURCES           = global/testing/lock.c $(gtsrc)
global_testing_mir_perf2_SOURCES      = global/testing/mir_perf2.F $(gtsrc)
global_testing_mmatrix_SOURCES        = global/testing/mmatrix.F $(gtsrc)
global_testing_mtest_SOURCES          = global/testing/mtest.c $(gtsrc)
global_testing_mulmatpatch_SOURCES    = global/testing/mulmatpatch.F $(gtsrc)
global_testing_nb2test_SOURCES        = global/testing/nb2test.F $(gtsrc)
global_testing_ndim_SOURCES           = global/testing/ndim.F $(gtsrc)
global_testing_ntestc_SOURCES         = global/testing/ntestc.c $(gtsrc)
global_testing_ntestfc_SOURCES        = global/testing/ntestfc.c $(gtsrc)
global_testing_patch_SOURCES          = global/testing/patch.F $(gtsrc)
global_testing_packc_SOURCES          = global/testing/packc.c $(gtsrc)
global_testing_patch_enumc_SOURCES    = global/testing/patch_enumc.c $(gtsrc)
global_testing_patch_enumf_SOURCES    = global/testing/patch_enumf.F $(gtsrc)
global_testing_perfmod_SOURCES        = global/testing/perfmod.F $(gtsrc)
global_testing_perform_SOURCES        = global/testing/perform.F $(gtsrc)
global_testing_perf_SOURCES           = global/testing/perf.F $(gtsrc)
global_testing_pg2testmatmult_SOURCES = global/testing/pg2testmatmult.F $(gtsrc)
global_testing_pg2test_SOURCES        = global/testing/pg2test.F $(gtsrc)
global_testing_pgtestmatmult_SOURCES  = global/testing/pgtestmatmult.F $(gtsrc)
global_testing_pgtest_SOURCES         = global/testing/pgtest.F $(gtsrc)
global_testing_print_SOURCES          = global/testing/print.c $(gtsrc)
global_testing_random_SOURCES         = global/testing/random.F $(gtsrc)
global_testing_scan_SOURCES           = global/testing/scan.F $(gtsrc)
global_testing_scan_addc_SOURCES      = global/testing/scan_addc.c $(gtsrc)
global_testing_scan_copyc_SOURCES     = global/testing/scan_copyc.c $(gtsrc)
global_testing_simple_groups_SOURCES  = global/testing/simple_groups.F $(gtsrc)
global_testing_sparse_SOURCES         = global/testing/sparse.F $(gtsrc)
global_testing_sprsmatmult_SOURCES    = global/testing/sprsmatmult.F $(gtsrc)
global_testing_stride_SOURCES         = global/testing/stride.F $(gtsrc)
global_testing_testc_SOURCES          = global/testing/testc.c $(gtsrc)
global_testing_testeig_SOURCES        = global/testing/testeig.F $(gtsrc)
global_testing_testmatmult_SOURCES    = global/testing/testmatmult.F $(gtsrc)
global_testing_testmult_SOURCES       = global/testing/testmult.c $(gtsrc)
global_testing_testsolve_SOURCES      = global/testing/testsolve.F $(gtsrc)
global_testing_testspd_SOURCES        = global/testing/testspd.F $(gtsrc)
global_testing_test_SOURCES           = global/testing/test.F $(gtsrc)
global_testing_unpackc_SOURCES        = global/testing/unpackc.c $(gtsrc)
nodist_global_testing_nga_onesided_SOURCES = global/testing/nga-onesided.F $(gtsrc)
nodist_global_testing_nga_patch_SOURCES    = global/testing/nga-patch.F $(gtsrc)
nodist_global_testing_nga_periodic_SOURCES = global/testing/nga-periodic.F $(gtsrc)
nodist_global_testing_nga_scatter_SOURCES  = global/testing/nga-scatter.F $(gtsrc)
nodist_global_testing_nga_util_SOURCES     = global/testing/nga-util.F $(gtsrc)
nodist_global_testing_ngatest_SOURCES      = global/testing/ngatest.F $(gtsrc)

global_testing_bin_LDFLAGS            = $(FLDFLAGS)
global_testing_blktest_LDFLAGS        = $(FLDFLAGS)
global_testing_d2test_LDFLAGS         = $(FLDFLAGS)
global_testing_g2test_LDFLAGS         = $(FLDFLAGS)
global_testing_g3test_LDFLAGS         = $(FLDFLAGS)
global_testing_ghosts_LDFLAGS         = $(FLDFLAGS)
global_testing_jacobi_LDFLAGS         = $(FLDFLAGS)
global_testing_mir_perf2_LDFLAGS      = $(FLDFLAGS)
global_testing_mmatrix_LDFLAGS        = $(FLDFLAGS)
global_testing_mulmatpatch_LDFLAGS    = $(FLDFLAGS)
global_testing_nb2test_LDFLAGS        = $(FLDFLAGS)
global_testing_ndim_LDFLAGS           = $(FLDFLAGS)
global_testing_nga_onesided_LDFLAGS   = $(FLDFLAGS)
global_testing_nga_patch_LDFLAGS      = $(FLDFLAGS)
global_testing_nga_periodic_LDFLAGS   = $(FLDFLAGS)
global_testing_nga_scatter_LDFLAGS    = $(FLDFLAGS)
global_testing_nga_util_LDFLAGS       = $(FLDFLAGS)
global_testing_ngatest_LDFLAGS        = $(FLDFLAGS)
global_testing_patch_LDFLAGS          = $(FLDFLAGS)
global_testing_patch_enumf_LDFLAGS    = $(FLDFLAGS)
global_testing_perfmod_LDFLAGS        = $(FLDFLAGS)
global_testing_perform_LDFLAGS        = $(FLDFLAGS)
global_testing_perf_LDFLAGS           = $(FLDFLAGS)
global_testing_pg2testmatmult_LDFLAGS = $(FLDFLAGS)
global_testing_pg2test_LDFLAGS        = $(FLDFLAGS)
global_testing_pgtestmatmult_LDFLAGS  = $(FLDFLAGS)
global_testing_pgtest_LDFLAGS         = $(FLDFLAGS)
global_testing_random_LDFLAGS         = $(FLDFLAGS)
global_testing_scan_LDFLAGS           = $(FLDFLAGS)
global_testing_simple_groups_LDFLAGS  = $(FLDFLAGS)
global_testing_sparse_LDFLAGS         = $(FLDFLAGS)
global_testing_sprsmatmult_LDFLAGS    = $(FLDFLAGS)
global_testing_stride_LDFLAGS         = $(FLDFLAGS)
global_testing_testeig_LDFLAGS        = $(FLDFLAGS)
global_testing_testmatmult_LDFLAGS    = $(FLDFLAGS)
global_testing_testsolve_LDFLAGS      = $(FLDFLAGS)
global_testing_testspd_LDFLAGS        = $(FLDFLAGS)
global_testing_test_LDFLAGS           = $(FLDFLAGS)

nodist_EXTRA_global_testing_elempatch_SOURCES   = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_ga_lu_SOURCES       = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_ga_mpi_SOURCES      = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_getmem_SOURCES      = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_ipc_clean_SOURCES   = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_lock_SOURCES        = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_mtest_SOURCES       = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_ntestc_SOURCES      = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_ntestfc_SOURCES     = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_packc_SOURCES       = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_patch_enumc_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_print_SOURCES       = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_scan_addc_SOURCES   = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_scan_copyc_SOURCES  = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_testc_SOURCES       = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_testmult_SOURCES    = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_global_testing_unpackc_SOURCES     = $(MAYBE_DUMMY_F_LINK)

##############################################################################
# global/examples
#
check_PROGRAMS += global/examples/lennard-jones/lennard
if NOFORT
else
check_PROGRAMS += global/examples/boltzmann/boltz
check_PROGRAMS += global/examples/conjugate_gradient/ga_cg
check_PROGRAMS += global/examples/md_cluster/grp_sim
check_PROGRAMS += global/examples/scf/scf
endif # NOFORT

global_examples_boltzmann_boltz_SOURCES = \
global/examples/boltzmann/boltzmann.F     \
global/examples/boltzmann/common          \
global/examples/boltzmann/equil.F         \
global/examples/boltzmann/get_patch.F     \
global/examples/boltzmann/initpar.F       \
global/examples/boltzmann/main.F          \
global/examples/boltzmann/printdat.F      \
global/examples/boltzmann/properties.F    \
global/examples/boltzmann/setup.F         \
global/examples/boltzmann/timestep.F      \
global/examples/boltzmann/vorticity.F
global_examples_boltzmann_boltz_LDFLAGS = $(FLDFLAGS)
global_examples_boltzmann_boltz_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/global/examples/boltzmann

global_examples_conjugate_gradient_ga_cg_SOURCES = \
global/examples/conjugate_gradient/computeloops.F  \
global/examples/conjugate_gradient/finclude.h      \
global/examples/conjugate_gradient/ga_cg.c         \
global/examples/conjugate_gradient/matvecmul.c     \
global/examples/conjugate_gradient/read_input.c

global_examples_lennard_jones_lennard_SOURCES = \
global/examples/lennard-jones/lennard.c
nodist_EXTRA_global_examples_lennard_jones_lennard_SOURCES = $(MAYBE_DUMMY_F_LINK)

global_examples_md_cluster_grp_sim_SOURCES = \
global/examples/md_cluster/cell_list.F       \
global/examples/md_cluster/cl_sim.F          \
global/examples/md_cluster/cluster.F         \
global/examples/md_cluster/common.fh         \
global/examples/md_cluster/estats.F          \
global/examples/md_cluster/factor.F          \
global/examples/md_cluster/force.F           \
global/examples/md_cluster/gather.F          \
global/examples/md_cluster/grp_sim.F         \
global/examples/md_cluster/hash.F            \
global/examples/md_cluster/heapsort.F        \
global/examples/md_cluster/kin.F             \
global/examples/md_cluster/mdinit.F          \
global/examples/md_cluster/mdout.F           \
global/examples/md_cluster/mdstep.F          \
global/examples/md_cluster/newcfg.F          \
global/examples/md_cluster/nextc.F           \
global/examples/md_cluster/pairs.F           \
global/examples/md_cluster/ran3.F            \
global/examples/md_cluster/rdpar.F           \
global/examples/md_cluster/scatter.F         \
global/examples/md_cluster/shuffle.F         \
global/examples/md_cluster/sort.F            \
global/examples/md_cluster/tstats.F          \
global/examples/md_cluster/update.F
global_examples_md_cluster_grp_sim_LDFLAGS = $(FLDFLAGS)

global_examples_scf_scf_SOURCES = \
global/examples/scf/cscf.h        \
global/examples/scf/input.F       \
global/examples/scf/integ.F       \
global/examples/scf/output.F      \
global/examples/scf/scf.F         \
global/examples/scf/timer.F
global_examples_scf_scf_LDFLAGS = $(FLDFLAGS)

EXTRA_DIST += global/examples/boltzmann/README
EXTRA_DIST += global/examples/lennard-jones/README
EXTRA_DIST += global/examples/md_cluster/atom.inp
EXTRA_DIST += global/examples/scf/be16.inpt
EXTRA_DIST += global/examples/scf/be.inpt
EXTRA_DIST += global/examples/scf/ft-scf.F
EXTRA_DIST += global/examples/scf/README

##############################################################################
# global/X
#
if ENABLE_XREGION
if NO_X
else
bin_PROGRAMS += global/X/xregion

global_X_xregion_SOURCES =    \
global/X/xregion_colormap.c   \
global/X/xregion_dialog.c     \
global/X/xregion_display.c    \
global/X/xregion_fileio.c     \
global/X/xregion_main.c       \
global/X/xregion_overview.c   \
global/X/xregion_pixregion.c  \
global/X/xregion_scrollbars.c \
global/X/xregion_util.c       \
global/X/xregion_view.c       \
global/X/xregion_xcmap.c

global_X_xregion_CPPFLAGS = $(X_CPPFLAGS)
global_X_xregion_LDFLAGS = $(X_LDFLAGS)
global_X_xregion_LDADD = -lXaw -lXmu -lXt -lX11 -lXau -lXdmcp -lXext

endif
endif

EXTRA_DIST += global/X/README

##############################################################################
# global/trace
#
bin_PROGRAMS += global/trace/adjust
bin_PROGRAMS += global/trace/collisions

global_trace_adjust_SOURCES = global/trace/adjust.c
global_trace_adjust_LDFLAGS =
global_trace_adjust_LDADD =

global_trace_collisions_SOURCES = global/trace/collisions.c
global_trace_collisions_LDFLAGS =
global_trace_collisions_LDADD =

EXTRA_DIST += global/trace/README
EXTRA_DIST += global/trace/test.f

##############################################################################
# global/doc
#
EXTRA_DIST += global/doc/README
EXTRA_DIST += global/doc/Supercomputing94.pdf
EXTRA_DIST += global/doc/user.pdf

##############################################################################
# pario
#
libga_la_SOURCES += pario/dra/buffers.c
libga_la_SOURCES += pario/dra/buffers.h
libga_la_SOURCES += pario/dra/capi.c
libga_la_SOURCES += pario/dra/disk.arrays.c
libga_la_SOURCES += pario/dra/disk.param.c
libga_la_SOURCES += pario/dra/draf2c.h
libga_la_SOURCES += pario/dra/drap.h
libga_la_SOURCES += pario/dra/env.c
libga_la_SOURCES += pario/dra/fortran.c
libga_la_SOURCES += pario/dra/patch.util.c
libga_la_SOURCES += pario/eaf/eaf.c
libga_la_SOURCES += pario/eaf/eaf_f2c.c
libga_la_SOURCES += pario/eaf/eafP.h
libga_la_SOURCES += pario/elio/elio.c
libga_la_SOURCES += pario/elio/eliop.h
libga_la_SOURCES += pario/elio/pablo.h
libga_la_SOURCES += pario/elio/stat.c
libga_la_SOURCES += pario/sf/sf_capi.c
libga_la_SOURCES += pario/sf/sff2c.h
libga_la_SOURCES += pario/sf/sf_fortran.c
libga_la_SOURCES += pario/sf/shared.files.c

include_HEADERS += pario/dra/dra.fh
include_HEADERS += pario/dra/dra.h
include_HEADERS += pario/eaf/eaf.fh
include_HEADERS += pario/eaf/eaf.h
include_HEADERS += pario/elio/chemio.h
include_HEADERS += pario/elio/elio.h
include_HEADERS += pario/sf/coms.h
include_HEADERS += pario/sf/sf.fh
include_HEADERS += pario/sf/sf.h

AM_CPPFLAGS += -I$(top_srcdir)/pario/dra
AM_CPPFLAGS += -I$(top_srcdir)/pario/eaf
AM_CPPFLAGS += -I$(top_srcdir)/pario/elio
AM_CPPFLAGS += -I$(top_srcdir)/pario/sf

EXTRA_DIST += pario/dra/dbg_read.c
EXTRA_DIST += pario/dra/dbg_write.c
EXTRA_DIST += pario/dra/dra2arviz.c
EXTRA_DIST += pario/dra/global.unsup.c

if NOFORT
else
check_PROGRAMS += pario/dra/big
check_PROGRAMS += pario/dra/dra_mxm
check_PROGRAMS += pario/dra/ntest
check_PROGRAMS += pario/dra/perf
check_PROGRAMS += pario/dra/perf2
check_PROGRAMS += pario/dra/perf3
check_PROGRAMS += pario/dra/test
check_PROGRAMS += pario/dra/test_mxm
check_PROGRAMS += pario/dra/time_mxm
check_PROGRAMS += pario/eaf/test
check_PROGRAMS += pario/sf/test
endif

check_PROGRAMS += pario/dra/bign
check_PROGRAMS += pario/dra/dbg_read
check_PROGRAMS += pario/dra/dbg_write
check_PROGRAMS += pario/dra/dra2arviz
check_PROGRAMS += pario/dra/ntestc
check_PROGRAMS += pario/dra/perfn
check_PROGRAMS += pario/dra/rate

dtsrc =
dtsrc += pario/dra/ffflush.F
dtsrc += pario/dra/util.c

pario_dra_big_SOURCES       = pario/dra/big.F $(dtsrc)
pario_dra_bign_SOURCES      = pario/dra/bign.c
pario_dra_dbg_read_SOURCES  = pario/dra/dbg_read.c
pario_dra_dbg_write_SOURCES = pario/dra/dbg_write.c
pario_dra_dra2arviz_SOURCES = pario/dra/dra2arviz.c
pario_dra_dra_mxm_SOURCES   = pario/dra/dra_mxm.F $(dtsrc)
pario_dra_ntest_SOURCES     = pario/dra/ntest.F $(dtsrc)
pario_dra_ntestc_SOURCES    = pario/dra/ntestc.c
pario_dra_perf_SOURCES      = pario/dra/perf.F $(dtsrc)
pario_dra_perf2_SOURCES     = pario/dra/perf2.F $(dtsrc)
pario_dra_perf3_SOURCES     = pario/dra/perf3.F $(dtsrc)
pario_dra_perfn_SOURCES     = pario/dra/perfn.c
pario_dra_rate_SOURCES      = pario/dra/rate.c
pario_dra_test_SOURCES      = pario/dra/test.F $(dtsrc)
pario_dra_test_mxm_SOURCES  = pario/dra/test_mxm.F $(dtsrc)
pario_dra_time_mxm_SOURCES  = pario/dra/time_mxm.F $(dtsrc)
pario_eaf_test_SOURCES      = pario/eaf/test.F $(dtsrc)
pario_sf_test_SOURCES       = pario/sf/test.F $(dtsrc)

nodist_EXTRA_pario_dra_bign_SOURCES      = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_dbg_read_SOURCES  = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_dbg_write_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_dra2arviz_SOURCES = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_ntestc_SOURCES    = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_perfn_SOURCES     = $(MAYBE_DUMMY_F_LINK)
nodist_EXTRA_pario_dra_rate_SOURCES      = $(MAYBE_DUMMY_F_LINK)

pario_dra_big_LDFLAGS       = $(FLDFLAGS)
pario_dra_dra_mxm_LDFLAGS   = $(FLDFLAGS)
pario_dra_ntest_LDFLAGS     = $(FLDFLAGS)
pario_dra_perf_LDFLAGS      = $(FLDFLAGS)
pario_dra_perf2_LDFLAGS     = $(FLDFLAGS)
pario_dra_perf3_LDFLAGS     = $(FLDFLAGS)
pario_dra_test_LDFLAGS      = $(FLDFLAGS)
pario_dra_test_mxm_LDFLAGS  = $(FLDFLAGS)
pario_dra_time_mxm_LDFLAGS  = $(FLDFLAGS)
pario_eaf_test_LDFLAGS      = $(FLDFLAGS)
pario_sf_test_LDFLAGS       = $(FLDFLAGS)

EXTRA_DIST += pario/dra/README

##############################################################################
# ga++/src
#
if CXX_BINDINGS

lib_LTLIBRARIES += libga++.la

libga___la_SOURCES =
libga___la_SOURCES += ga++/src/GAServices.cc
libga___la_SOURCES += ga++/src/GlobalArray.cc
libga___la_SOURCES += ga++/src/PGroup.cc
libga___la_SOURCES += ga++/src/init_term.cc
libga___la_SOURCES += ga++/src/overload.cc
libga___la_SOURCES += ga++/src/services.cc

include_HEADERS += ga++/src/ga++.h
include_HEADERS += ga++/src/GAServices.h
include_HEADERS += ga++/src/GlobalArray.h
include_HEADERS += ga++/src/init_term.h
include_HEADERS += ga++/src/PGroup.h
include_HEADERS += ga++/src/services.h

AM_CPPFLAGS += -I$(top_srcdir)/ga++/src

endif # CXX_BINDINGS

EXTRA_DIST += ga++/README

##############################################################################
# python
#
if HAVE_PYTHON

python: install
	(cd $(top_srcdir)/python && $(PYTHON) setup.py build_ext -L$(libdir) -I$(includedir)) || exit 1
	(cd $(top_srcdir)/python && $(PYTHON) setup.py install) || exit 1

endif

##############################################################################
# ga++/testing
#
if CXX_BINDINGS

check_PROGRAMS += ga++/testing/elempatch
check_PROGRAMS += ga++/testing/mtest
check_PROGRAMS += ga++/testing/ntestc
check_PROGRAMS += ga++/testing/testc
check_PROGRAMS += ga++/testing/testmult

ga___testing_elempatch_SOURCES = ga++/testing/elempatch.cc
ga___testing_mtest_SOURCES     = ga++/testing/mtest.cc
ga___testing_ntestc_SOURCES    = ga++/testing/ntestc.cc \
                                 ga++/testing/util.c
ga___testing_testc_SOURCES     = ga++/testing/testc.cc
ga___testing_testmult_SOURCES  = ga++/testing/testmult.cc

ga___testing_elempatch_LDADD   = libga++.la $(LDADD) $(MAYBE_FLIBS)
ga___testing_mtest_LDADD       = libga++.la $(LDADD) $(MAYBE_FLIBS)
ga___testing_ntestc_LDADD      = libga++.la $(LDADD) $(MAYBE_FLIBS)
ga___testing_testc_LDADD       = libga++.la $(LDADD) $(MAYBE_FLIBS)
ga___testing_testmult_LDADD    = libga++.la $(LDADD) $(MAYBE_FLIBS)

ga___testing_elempatch_LDFLAGS = $(FLDFLAGS)
ga___testing_mtest_LDFLAGS     = $(FLDFLAGS)
ga___testing_ntestc_LDFLAGS    = $(FLDFLAGS)
ga___testing_testc_LDFLAGS     = $(FLDFLAGS)
ga___testing_testmult_LDFLAGS  = $(FLDFLAGS)

endif # CXX_BINDINGS

##############################################################################
# armci
SUBDIRS = armci .

armci/libarmcii.la:
	($(am__cd) armci && $(MAKE) $(AM_MAKEFLAGS) all) || exit 1;

AM_CPPFLAGS += -I$(top_srcdir)/armci/$(ARMCI_SRC_DIR)

# This is the "noinst" libarmci so that libtool combines it with libga.
libga_la_LIBADD += $(SCALAPACK_LIBS)
libga_la_LIBADD += $(LAPACK_LIBS)
libga_la_LIBADD += $(BLAS_LIBS)
libga_la_LIBADD += armci/libarmcii.la
libga_la_LIBADD += $(GA_MP_LIBS)
libga_la_LIBADD += $(ARMCI_NETWORK_LIBS)

##############################################################################
# tascel
if ENABLE_TASCEL
SUBDIRS += tascel
endif

##############################################################################
# the end
#
.PHONY: checkprogs checkprogs-recursive checkprogs-am
checkprogs: checkprogs-recursive
checkprogs-am: $(check_PROGRAMS)

# automake does not append $(EXEEXT) to user-defined make variables
# so we must duplicate all examples here to get this custom target... yuck
.PHONY: examples examples-recursive examples-am
examples: examples-recursive
examples-am: $(EXAMPLES)
EXAMPLES =
EXAMPLES += global/examples/lennard-jones/lennard$(EXEEXT)
if NOFORT
else
if MSG_COMMS_TCGMSG
EXAMPLES += global/examples/scf/scf$(EXEEXT)
endif
EXAMPLES += global/examples/boltzmann/boltz$(EXEEXT)
EXAMPLES += global/examples/conjugate_gradient/ga_cg$(EXEEXT)
EXAMPLES += global/examples/md_cluster/grp_sim$(EXEEXT)
endif # NOFORT

GA_RECURSIVE_TARGETS = checkprogs-recursive examples-recursive
# COPIED FROM MAKEFILE GENERATED BY AUTOMAKE 1.11.1
# Modified to use our own recursive targets.
$(GA_RECURSIVE_TARGETS): all
	@fail= failcom='exit 1'; \
	for f in x $$MAKEFLAGS; do \
	  case $$f in \
	    *=* | --[!k]*);; \
	    *k*) failcom='fail=yes';; \
	  esac; \
	done; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  ($(am__cd) $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	  || eval $$failcom; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"
# END COPY

# support verbose/silent make rules for additional programs
# m4
AM_V_M4 = $(am__v_M4_$(V))
am__v_M4_ = $(am__v_M4_$(AM_DEFAULT_VERBOSITY))
am__v_M4_0 = @echo "  M4    " $@;
# sed
AM_V_SED = $(am__v_SED_$(V))
am__v_SED_ = $(am__v_SED_$(AM_DEFAULT_VERBOSITY))
am__v_SED_0 = @echo "  SED   " $@;

.PHONY: pfiles clean-pfiles
if MSG_COMMS_TCGMSG
if MSG_COMMS_MPI
else
pfiles: $(check_PROGRAMS)
	for p in $(check_PROGRAMS); do \
		echo "`whoami` `hostname` 4 `pwd`/$$p /tmp" > $$p.p; \
	done
check: pfiles
clean-pfiles:
	find . -name '*.p' -exec rm {} \; ;
clean-local: clean-pfiles
MAYBE_PFILES = pfiles
endif # MSG_COMMS_MPI
endif # MSG_COMMS_TCGMSG

##############################################################################
# test suite
#
# some tests were commented out either because they required an input file
# (some examples do) or they were failing for unknown reasons and we didn't
# want to further debug
#
TESTS = $(SERIAL_TESTS) $(PARALLEL_TESTS)
XFAIL_TESTS = $(SERIAL_TESTS_XFAIL) $(PARALLEL_TESTS_XFAIL)
TESTS_ENVIRONMENT = \
maybe_mpi=`if echo "$(SERIAL_TESTS)" | $(GREP) "$$p" > /dev/null; then echo ""; else echo "$(MPIEXEC)"; fi`; $$maybe_mpi
.PHONY: check-ga
check-ga: $(all-am) $(check_LTLIBRARIES) $(check_PROGRAMS) $(MAYBE_PFILES)
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS
SERIAL_TESTS =
SERIAL_TESTS_XFAIL =
PARALLEL_TESTS =
PARALLEL_TESTS_XFAIL =
if NOFORT
else
SERIAL_TESTS += gaf2c/testarg$(EXEEXT)
endif
#SERIAL_TESTS += ma/testc$(EXEEXT)
SERIAL_TESTS += ma/test-coalesce$(EXEEXT)
SERIAL_TESTS += ma/test-inquire$(EXEEXT)
if NOFORT
else
SERIAL_TESTS += ma/testf$(EXEEXT)
SERIAL_TESTS_XFAIL += ma/testf$(EXEEXT)
endif
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG4
PARALLEL_TESTS += tcgmsg/ipcv4.0/hello$(EXEEXT)
#PARALLEL_TESTS += tcgmsg/ipcv4.0/test$(EXEEXT) # requires user input
#PARALLEL_TESTS += tcgmsg/ipcv4.0/testmap$(EXEEXT) # requires user input
#PARALLEL_TESTS += tcgmsg/ipcv4.0/toplot$(EXEEXT) # not a test
#PARALLEL_TESTS += tcgmsg/ipcv4.0/xdrtest$(EXEEXT) # requires user input
if NOFORT
else
PARALLEL_TESTS += tcgmsg/ipcv4.0/testf$(EXEEXT)
#PARALLEL_TESTS += tcgmsg/ipcv4.0/testpf$(EXEEXT) # requires input files
endif
endif # MSG_COMMS_TCGMSG4
endif # MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG5
PARALLEL_TESTS += tcgmsg/ipcv5.0/hello$(EXEEXT)
#PARALLEL_TESTS += tcgmsg/ipcv5.0/test$(EXEEXT) # requires user input
if MSG_COMMS_PVM
PARALLEL_TESTS += tcgmsg/ipcv5.0/pvm.ring$(EXEEXT)
endif
if NOFORT
else
#PARALLEL_TESTS += tcgmsg/ipcv5.0/jacobi$(EXEEXT) # requires user input
#PARALLEL_TESTS += tcgmsg/ipcv5.0/testf$(EXEEXT) # requires user input
endif
endif # MSG_COMMS_TCGMSG5
endif # MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSG
if MSG_COMMS_TCGMSGMPI
#PARALLEL_TESTS += tcgmsg/tcgmsg-mpi/test_asyn$(EXEEXT) # useless test
#PARALLEL_TESTS += tcgmsg/tcgmsg-mpi/test$(EXEEXT) # requires user input
if NOFORT
else
PARALLEL_TESTS += tcgmsg/tcgmsg-mpi/testf$(EXEEXT)
endif
endif # MSG_COMMS_TCGMSGMPI
endif # MSG_COMMS_TCGMSG
PARALLEL_TESTS += global/testing/elempatch$(EXEEXT)
PARALLEL_TESTS += global/testing/getmem$(EXEEXT)
PARALLEL_TESTS += global/testing/mtest$(EXEEXT)
PARALLEL_TESTS += global/testing/ntestc$(EXEEXT)
PARALLEL_TESTS += global/testing/ntestfc$(EXEEXT)
PARALLEL_TESTS += global/testing/packc$(EXEEXT)
PARALLEL_TESTS += global/testing/patch_enumc$(EXEEXT)
PARALLEL_TESTS += global/testing/print$(EXEEXT)
PARALLEL_TESTS += global/testing/scan_addc$(EXEEXT)
PARALLEL_TESTS += global/testing/scan_copyc$(EXEEXT)
PARALLEL_TESTS += global/testing/testc$(EXEEXT)
PARALLEL_TESTS += global/testing/testmult$(EXEEXT)
PARALLEL_TESTS += global/testing/unpackc$(EXEEXT)
if NOFORT
if HAVE_LAPACK
PARALLEL_TESTS += global/testing/ga_lu$(EXEEXT)
endif
else
PARALLEL_TESTS += global/testing/bin$(EXEEXT)
PARALLEL_TESTS += global/testing/blktest$(EXEEXT)
#PARALLEL_TESTS += global/testing/d2test$(EXEEXT) # needs input file
PARALLEL_TESTS += global/testing/g2test$(EXEEXT)
PARALLEL_TESTS += global/testing/g3test$(EXEEXT)
PARALLEL_TESTS += global/testing/ga_lu$(EXEEXT)
PARALLEL_TESTS += global/testing/ghosts$(EXEEXT)
PARALLEL_TESTS += global/testing/jacobi$(EXEEXT)
PARALLEL_TESTS += global/testing/mir_perf2$(EXEEXT)
PARALLEL_TESTS += global/testing/mmatrix$(EXEEXT)
PARALLEL_TESTS += global/testing/mulmatpatch$(EXEEXT)
PARALLEL_TESTS += global/testing/nb2test$(EXEEXT)
PARALLEL_TESTS += global/testing/ndim$(EXEEXT)
PARALLEL_TESTS += global/testing/patch$(EXEEXT)
PARALLEL_TESTS += global/testing/patch_enumf$(EXEEXT)
PARALLEL_TESTS += global/testing/perfmod$(EXEEXT)
PARALLEL_TESTS += global/testing/perform$(EXEEXT)
PARALLEL_TESTS += global/testing/perf$(EXEEXT)
#PARALLEL_TESTS += global/testing/pg2testmatmult$(EXEEXT) # needs 8 procs exactly
PARALLEL_TESTS += global/testing/pg2test$(EXEEXT)
#PARALLEL_TESTS += global/testing/pgtestmatmult$(EXEEXT) # needs 8 procs exactly
PARALLEL_TESTS += global/testing/pgtest$(EXEEXT)
#PARALLEL_TESTS += global/testing/random$(EXEEXT) # takes too long
PARALLEL_TESTS += global/testing/scan$(EXEEXT)
PARALLEL_TESTS += global/testing/simple_groups$(EXEEXT)
PARALLEL_TESTS += global/testing/sparse$(EXEEXT)
PARALLEL_TESTS += global/testing/sprsmatmult$(EXEEXT)
PARALLEL_TESTS += global/testing/stride$(EXEEXT)
PARALLEL_TESTS += global/testing/testeig$(EXEEXT)
PARALLEL_TESTS += global/testing/testmatmult$(EXEEXT)
PARALLEL_TESTS += global/testing/testsolve$(EXEEXT)
PARALLEL_TESTS += global/testing/test$(EXEEXT)
if HAVE_SCALAPACK
PARALLEL_TESTS += global/testing/testspd$(EXEEXT)
endif
endif
if MSG_COMMS_MPI
PARALLEL_TESTS += global/testing/ga-mpi$(EXEEXT)
PARALLEL_TESTS += global/testing/lock$(EXEEXT)
endif
if SYSV
PARALLEL_TESTS += global/testing/ipc_clean$(EXEEXT)
endif
if NOFORT
else
PARALLEL_TESTS += global/testing/nga-onesided$(EXEEXT)
PARALLEL_TESTS += global/testing/nga-patch$(EXEEXT)
PARALLEL_TESTS += global/testing/nga-periodic$(EXEEXT)
PARALLEL_TESTS += global/testing/nga-scatter$(EXEEXT)
PARALLEL_TESTS += global/testing/nga-scatter$(EXEEXT)
PARALLEL_TESTS += global/testing/nga-util$(EXEEXT)
PARALLEL_TESTS += global/testing/ngatest$(EXEEXT)
endif
PARALLEL_TESTS += global/examples/lennard-jones/lennard$(EXEEXT)
if NOFORT
else
if MSG_COMMS_TCGMSG
#PARALLEL_TESTS += global/examples/scf/scf$(EXEEXT) # needs input
endif
PARALLEL_TESTS += global/examples/boltzmann/boltz$(EXEEXT)
#PARALLEL_TESTS += global/examples/conjugate_gradient/ga_cg$(EXEEXT) # needs user input
#PARALLEL_TESTS += global/examples/md_cluster/grp_sim$(EXEEXT) # needs input
endif # NOFORT
PARALLEL_TESTS += pario/dra/bign$(EXEEXT)
PARALLEL_TESTS += pario/dra/ntestc$(EXEEXT)
PARALLEL_TESTS += pario/dra/perfn$(EXEEXT)
PARALLEL_TESTS += pario/dra/rate$(EXEEXT)
if CXX_BINDINGS
PARALLEL_TESTS += ga++/testing/elempatch$(EXEEXT)
PARALLEL_TESTS += ga++/testing/mtest$(EXEEXT)
PARALLEL_TESTS += ga++/testing/ntestc$(EXEEXT)
PARALLEL_TESTS += ga++/testing/testc$(EXEEXT)
PARALLEL_TESTS += ga++/testing/testmult$(EXEEXT)
endif
