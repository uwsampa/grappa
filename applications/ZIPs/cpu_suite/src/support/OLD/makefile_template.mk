# CVS info
# $Date: 2010/04/07 18:42:26 $
# $Revision: 1.11 $
# $RCSfile: makefile_template.mk,v $
# $State: Stab $:

# BASECFLAGS = -I.

SHELL		= /bin/sh

#Shortened output
OFMT		=
#Unique ids on MPPs
UID		=

CC	=
CFLAGS	=

SPLINT	= /usr/bin/splint
SPFLAGS = -unrecog -weak -posix-lib

OBJS = ./error_routines.o ./bm_timers.o ./brand.o ./read_write_io.o ./for_drivers.o ./uqid.o ./data_blks.o

all: $(OBJS) gen_file sec_frm_epoch


$(OBJS):		makefile

gen_file:		./gen_file.c ./common_inc.h ./local_types.h ./data_blks.c ./bm_timers.c ./bm_timers.h
			$(CC) $(CFLAGS) ./gen_file.c -o gen_file ./data_blks.o ./bm_timers.o ./error_routines.o
			date

error_routines.o: 	./error_routines.c ./common_inc.h ./local_types.h ./error_routines.h
			success=0 && $(CC) $(CFLAGS) $(UID) $(OFMT) -c error_routines.c && success=1; \
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

brand.o: 		./brand.c ./brand.h ./local_types.h
			success=0 && $(CC) $(CFLAGS) $(UID) $(OFMT) -c brand.c && success=1;\
			if [ $$success -eq 0 ]; \
			then \
				touch .error;\
				exit -1; \
			fi

bm_timers.o: 		./bm_timers.c ./common_inc.h ./local_types.h ./bm_timers.h ./error_routines.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c bm_timers.c

read_write_io.o: 	./read_write_io.c ./common_inc.h ./local_types.h ./error_routines.h
		 	$(CC) $(CFLAGS) $(UID) $(OFMT) -c read_write_io.c

for_drivers.o: 		./for_drivers.c ./bench.h ./common_inc.h ./local_types.h ./bm_timers.h \
                            ./error_routines.h ./brand.h ./iobmversion.h ./uqid.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c for_drivers.c

uqid.o: 		./uqid.c ./bench.h ./common_inc.h ./local_types.h ./bm_timers.h \
                            ./error_routines.h ./brand.h ./iobmversion.h ./uqid.h
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c uqid.c

data_blks.o:		./data_blks.c
			$(CC) $(CFLAGS) $(UID) $(OFMT) -c data_blks.c

sec_frm_epoch: 		./sec_frm_epoch.c
			$(CC) $(CFLAGS) sec_frm_epoch.c -o sec_frm_epoch
			date

test_brand: 		./brand.c ./brand.h ./local_types.h
			$(CC) $(CFLAGS) -DTEST_BRAND ./brand.c -o ./test_brand

# Check unannotated C code (can't handle ~)
splint:
		$(SPLINT) $(SPFLAGS)  ./gen_file.c  ./error_routines.c ./brand.c \
                    ./ bm_timers.c ./read_write_io.c ./for_drivers.c ./uqid.c ./data_blks.c

check:
		echo "support check"

# Check the file generator
gen_check:	./gen_file
		./gen_file /tmp/abc_1k 1k
		./gen_file /tmp/abc_2K 2K
		./gen_file /tmp/abc_1m 1m
		./gen_file /tmp/abc_2M 2M
		/bin/ls -l /tmp/abc_[12][kKmM]    > gen_file_check_output
		/bin/rm -f /tmp/abc_[12][kKmM]
		./gen_file /tmp/abc_1g 1g
		./gen_file /tmp/abc_1G 1G
		/bin/ls -l /tmp/abc_[12][gG]     >> gen_file_check_output
		/bin/rm -f /tmp/abc_[12][gG]


# Check the random number generator against known good values
brand_check:    ./test_brand
		./test_brand > brand_check_output

clean:
		/bin/rm -f $(OBJS) ./*~ ./#*# ./cflags.h ./gen_file_check_output \
                      ./brand_check_output ./gen_file.o ./sec_frm_epoch.o ./*.oo

clobber:	clean
		/bin/rm -f ./makefile gen_file ./sec_frm_epoch ./test_brand ./test_brand


TAGS:   bench.h bm_timers.c bm_timers.h brand.c brand.h error_routines.c \
                   error_routines.h for_drivers.c gen_file.c iobench.h iobmversion.h print_cmplr_flags.c \
                   print_cmplr_flags.c sec_frm_epoch.c
		etags -l c   bench.h bm_timers.c bm_timers.h brand.c brand.h error_routines.c \
                   error_routines.h for_drivers.c gen_file.c iobench.h iobmversion.h print_cmplr_flags.c \
                   print_cmplr_flags.c sec_frm_epoch.c
