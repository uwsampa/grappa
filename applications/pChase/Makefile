
#
# BIT  = { 32 | 64 }
# MODE = { NUMA | SMP }
#
ifndef BIT
BIT	= 64
endif
ifndef MODE
MODE	= NUMA
endif
ifeq ($(MODE), NUMA)
LIB	= -lpthread -lnuma
else
LIB	= -lpthread 
endif

SRC	= Main.C Chain.C Experiment.C Lock.C Output.C Run.C SpinBarrier.C Timer.C Thread.C Types.C
HDR	= $(SRC:.C=.h)
OBJ	= $(SRC:.C=.o)
EXE	= pChase$(BIT)_$(MODE)
HYPDIR	= /web/hypercomputing.org/www/doc/Guest/pChase
PCHDIR	= /web/pchase.org/www/doc/Guest/pChase
TARFILE	= tgz/pChase-`date +"%Y-%m-%d"`.tgz

RM	= /bin/rm
MV	= /bin/mv
CI	= /usr/bin/ci
CO	= /usr/bin/co
CP	= /bin/cp
TAR	= /bin/tar

CXXFLAGS= -O3 -m$(BIT) -D$(MODE)

.C.o:
	$(CXX) -c $(CXXFLAGS) $<

$(EXE):	$(OBJ)
	$(CXX) -o $(EXE) $(CXXFLAGS) $(OBJ) $(LIB)

$(OBJ):	$(HDR)

rmexe:
	$(RM) -rf $(EXE)

rmobj:
	$(RM) -rf $(OBJ)

ci:
	$(CI) -f $(SRC) $(HDR) Makefile

co:
	$(CO) -l $(SRC) $(HDR) Makefile

tar:
	$(TAR) -cvzf $(TARFILE) $(SRC) $(HDR) Makefile License.htm License.txt pChase.sh run-pChase.sh

cptar:
	$(TAR) -cvzf $(TARFILE) $(SRC) $(HDR) Makefile License.htm License.txt pChase.sh run-pChase.sh
	$(CP) $(TARFILE) $(HYPDIR)/tgz
	$(CP) $(SRC) $(HDR) Makefile License.htm License.txt pChase.sh run-pChase.sh $(HYPDIR)
	$(CP) $(TARFILE) $(PCHDIR)/tgz
	$(CP) $(SRC) $(HDR) Makefile License.htm License.txt pChase.sh run-pChase.sh $(PCHDIR)
