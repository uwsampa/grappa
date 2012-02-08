#conduit
#[MPI,IBV,SMP]
CONDUIT=IBV

#threading
#[PAR,PARSYNC,SEQ]
THREADING_GASNET=PAR

#tracing
#[0,1]
TRACING_GASNET=0

if [ $CONDUIT == "IBV" ]
    then
    export MA_CONDUIT_LIB=ibv
    export MA_CONDUIT_DEF=IBV
    else
    export MA_CONDUIT_LIB=mpi
    export MA_CONDUIT_DEF=MPI
    export MA_ADD_GASNET_LIBS=-lammpi
fi

if [ $THREADING_GASNET == "PAR" ]
    then
    export MA_THREADING_GASNET_DEF=PAR
    export MA_THREADING_GASNET_LIB=par
elif [ $THREADING_GASNET == "PARSYNC" ]
    then
    export MA_THREADING_GASNET_DEF=PARSYNC
    export MA_THREADING_GASNET_LIB=parsync
else
    export MA_THREADING_GASNET_DEF=SEQ
    export MA_THREADING_GASNET_LIB=seq
fi

if [ $TRACING_GASNET == 0 ]
    then
    export GASNET=/sampa/share/gasnet-rhel6
    else
    export GASNET=/sampa/share/gasnet-rhel6-tracing
fi

THIS_DIR=`pwd`
GLM_DIR=$THIS_DIR/../../global_memory
GLA_DIR=$THIS_DIR/../../mark_scratch
COL_DIR=$THIS_DIR/collective

cd $GLA_DIR
make clean objects

cd $COL_DIR
make clean collective.o

cd $GLM_DIR
make clean libsplitphase.a

cd $THIS_DIR
make clean linked_list.exe
