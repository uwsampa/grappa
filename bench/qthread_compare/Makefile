QTHREAD=../../tools/qthread-1.7-install

default: qthread-yield-time
	LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$(QTHREAD)/lib ./$<

qthread-yield-time: CFLAGS+=-I$(QTHREAD)/include
qthread-yield-time: LDFLAGS+=-L$(QTHREAD)/lib
qthread-yield-time: LDFLAGS+=-lqthread

