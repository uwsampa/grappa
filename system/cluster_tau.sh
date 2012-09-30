# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

## Environment variables when tracing with Tau

export TAU_TRACE=0
export TAU_PROFILE=1
export TAU_SYNCHRONIZE_CLOCKS=1
export PROFILEDIR=`pwd`/profiles
export TRACEDIR=`pwd`/profiles
#export PROFILEDIR=/scratch/bdmyers/profiles
#export TRACEDIR=/scratch/bdmyers/profiles
export TAU_VERBOSE=0
export PATH="/sampa/share/tau-perf/tau/x86_64/bin:$PATH"
