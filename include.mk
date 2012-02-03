#############################################################################
# common variables for softxmt project
#############################################################################

#
# for janus03? at HPC Advisory Council
#

CC=gcc44
CXX=g++44
#CXX=mpic++
LD=mpic++

#CFLAGS+= -std=c++0x

# some library paths
GASNET=/home/jnelson/gasnet18-mellanox-default
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit
LDFLAGS+= -L$(GASNET)/lib

HUGETLBFS=/home/jnelson/libhugetlbfs-2.12
CFLAGS+= -I$(HUGETLBFS)/include
LDFLAGS+= -L$(HUGETLBFS)/lib

GFLAGS=/home/jnelson/gflags-install
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

BOOST=/home/jnelson/boost-install
CFLAGS+= -I$(BOOST)/include
LDFLAGS+= -L$(BOOST)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

COMMON=../common
CFLAGS+= -I$(COMMON)

GREENERY=../greenery
CFLAGS+= -I$(GREENERY)
LDFLAGS+= -L$(GREENERY)



