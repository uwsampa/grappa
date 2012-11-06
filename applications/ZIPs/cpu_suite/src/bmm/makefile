# These get overridden by the global makefile.  However, if you just want to
# compile in this directory, you should be able to do

CC=gcc
CFLAGS=-O3 -std=c99 -DRESTRICT="restrict"  -DB_S_X2="b_s_x2_" -I. -I../support 


SUPPORT_DIR=../support
BMM_DIR=.

SUPPORT_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o

SUPPORT_INC = $(SUPPORT_DIR)/common_inc.h $(SUPPORT_DIR)/local_types.h \
	$(SUPPORT_DIR)/bench.h $(SUPPORT_DIR)/bm_timers.h \
	$(SUPPORT_DIR)/error_routines.h $(SUPPORT_DIR)/brand.h \
	$(SUPPORT_DIR)/iobmversion.h


BMM_OBJS = bmm.o driver.o print_cmplr_flags.o

OBJS = $(BMM_OBJS) $(SUPPORT_OBJS)

all:            bmm

clean:
		/bin/rm -f ../../bin/bmm $(BMM_OBJS) ./*~ ./#*# ./cflags.h


bmm:            $(BMM_OBJS) $(BMM_DIR)/bmm.c ./cflags.h $(SUPPORT_INC)
		$(CC) $(CFLAGS) -o ../../bin/bmm $(OBJS) $(LDFLAGS)

driver.o:               $(BMM_DIR)/driver.c $(BMM_DIR)/makefile $(BMM_DIR)/bmm.h $(SUPPORT_INC)
			$(CC) $(CFLAGS)  $(UID) -c $<

bmm.o:                  $(BMM_DIR)/bmm.c $(BMM_DIR)/makefile $(BMM_DIR)/bmm.h $(BMM_DIR)/bmm_cksm_times.h $(SUPPORT_INC)
			success=0 && $(CC) $(CFLAGS) $(UID) -c $< && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

print_cmplr_flags.o:    $(SUPPORT_DIR)/print_cmplr_flags.c $(BMM_DIR)/makefile ./cflags.h
			$(CC) $(CFLAGS) -DHAS_CFLAGS -c $<

cflags.h:
			echo "char cflags[] = " \"$(CC) $(CFLAGS) \" \; > cflags.h

