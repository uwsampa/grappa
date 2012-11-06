SUPPORT_DIR = .
OBJS = ./error_routines.o ./bm_timers.o ./brand.o ./read_write_io.o ./for_drivers.o ./uqid.o ./data_blks.o

CPU_OBJS = $(SUPPORT_DIR)/for_drivers.o $(SUPPORT_DIR)/error_routines.o  \
	$(SUPPORT_DIR)/bm_timers.o $(SUPPORT_DIR)/brand.o $(SUPPORT_DIR)/uqid.o


all: 	$(OBJS) gen_file sec_frm_epoch

clean:
	/bin/rm -f $(OBJS) ./*~ ./#*# ./cflags.h *.o

cpu_clean: 
	/bin/rm -f $(CPU_OBJS)

gen_file:	./gen_file.c ./common_inc.h ./local_types.h ./data_blks.c ./bm_timers.c ./bm_timers.h
		$(CC) $(CFLAGS) ./gen_file.c -o gen_file ./data_blks.o ./bm_timers.o ./error_routines.o

error_routines.o:       ./error_routines.c ./common_inc.h ./local_types.h ./error_routines.h
			success=0 && $(CC) $(CFLAGS) $(UID) $(OFMT) -c error_routines.c && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

brand.o:                ./brand.c ./brand.h ./local_types.h
			success=0 && $(CC) $(CFLAGS) $(UID) $(OFMT) -c brand.c && success=1;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

bm_timers.o:            ./bm_timers.c ./common_inc.h ./local_types.h ./bm_timers.h ./error_routines.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c bm_timers.c

read_write_io.o:        ./read_write_io.c ./common_inc.h ./local_types.h ./error_routines.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c read_write_io.c

for_drivers.o:          ./for_drivers.c ./bench.h ./common_inc.h ./local_types.h ./bm_timers.h \
                            ./error_routines.h ./brand.h ./iobmversion.h ./uqid.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c for_drivers.c

uqid.o:                 ./uqid.c ./bench.h ./common_inc.h ./local_types.h ./bm_timers.h \
                            ./error_routines.h ./brand.h ./iobmversion.h ./uqid.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c uqid.c

data_blks.o:            ./data_blks.c
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c data_blks.c

sec_frm_epoch:          ./sec_frm_epoch.c
			$(CC) $(CFLAGS) sec_frm_epoch.c -o sec_frm_epoch

