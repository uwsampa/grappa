
# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.
GRAPPA_HOME?=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
# check if autodetect GRAPPA_HOME is consistent
AUTO_HOME=$(shell /bin/bash -c '(while [[ ! -e include.mk && "`pwd`" != "/" ]]; do cd ..; done; pwd )')
ifneq ($(GRAPPA_HOME), $(AUTO_HOME))
$(warning warning: Environment variable GRAPPA_HOME was set but does not match the autodetected home: env=$(GRAPPA_HOME), auto=$(AUTO_HOME))
endif

all: fft.exe

include $(GRAPPA_HOME)/include.mk
include $(GRAPPA_HOME)/system/Makefile

FFTW_DIR = $(GRAPPA_HOME)/applications/fft/fftw

SOURCES = main.c

fft.exe: $(SOURCES)
	mpicc -std=c99 -I$(FFTW_DIR)/include $(SOURCES) -L$(FFTW_DIR)/lib -lfftw3_mpi -lfftw3 -lm -o fft.exe