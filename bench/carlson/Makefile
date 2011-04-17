CFLAGS = -O1 -g -std=c99 -Wall -Wextra -I../.. -fopenmp -msse4.1 -I../../greenery
LDFLAGS= -L../../greenery
LDLIBS= -lgreenery -lrt -fopenmp -lnuma -lhugetlbfs
BENCHES= carlson

RESULTS= $(BENCHES)

SRCS=  carlson.c

.PHONY: all clean

all: $(RESULTS)
	objdump -S carlson | tee carlson.dis
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 16 -f 7
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7 -u 1
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7 -u 8
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 8 -f 7 -u 32
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 6 -t 1 -f 7
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 6 -t 16 -f 7
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 6 -t 1 -f 7 -u 1
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 6 -t 1 -f 7 -u 8
	GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 6 -t 8 -f 7 -u 32


# GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7
# GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 16 -f 7
# GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7 -u 1
# GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 1 -f 7 -u 8
# GOMP_CPU_AFFINITY="0-11:2" hugectl --heap -- numactl --membind=0 -- ./carlson -c 1 -t 8 -f 7 -u 32


carlson: carlson.o $(MYLIBS)

clean:
	rm -f $(RESULTS) $(SRCS:.c=.o)
