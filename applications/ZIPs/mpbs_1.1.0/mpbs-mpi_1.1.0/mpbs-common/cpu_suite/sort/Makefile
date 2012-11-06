# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=icc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -xHost -I. -I../support 

SUPPORT_DIR=../support
A_SORT_DIR=.

SmallSortSz = 96
PSZ1  =     6
PSZ3  =     8


SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h

A_SORT_OBJS = ./a_sort.o ./driver.o ./psort.o ./print_cmplr_flags.o

OBJS            = $(A_SORT_OBJS) $(SUPPORT_OBJS)

all: 		a_sort

clean:
		/bin/rm -f a_sort *.o $(A_SORT_OBJS) ./*~ ./#*# ./cflags.h

a_sort:         $(A_SORT_OBJS) $(A_SORT_DIR)/a_sort.c ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) -DSmallSortSz=$(SmallSortSz) -DPSZ1=$(PSZ1) \
			-DPSZ3=$(PSZ3) -o a_sort $(OBJS) -lm $(LDFLAGS)

driver.o:  $(A_SORT_DIR)/driver.c $(A_SORT_DIR)/Makefile $(SUPPORT_INC)
		$(CC) $(CFLAGS)  $(UID) -c $<

a_sort.o:       $(A_SORT_DIR)/a_sort.c $(A_SORT_DIR)/Makefile $(SUPPORT_INC)
		success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1 ;\
		if [ $$success -eq 0 ]; \
		then \
			touch .error;\
			exit -1; \
		fi

print_cmplr_flags.o: $(SUPPORT_DIR)/print_cmplr_flags.c $(A_SORT_DIR)/Makefile ./cflags.h
		$(CC) $(CFLAGS) -DHAS_CFLAGS -c $<

cflags.h:
		echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

