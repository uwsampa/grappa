#############################################################################
# common variables for softxmt project
#############################################################################

#
# common across machines
#
COMMON=../common
CFLAGS+= -I$(COMMON)

#GREENERY=../greenery
#CFLAGS+= -I$(GREENERY)
#LDFLAGS+= -L$(GREENERY)

#
# different for specific machines
#

# #
# # for janus03? at HPC Advisory Council
# #

# CC=gcc44
# CXX=g++44
# #CXX=mpic++
# LD=mpic++

# #CFLAGS+= -std=c++0x

# # some library paths
# GASNET=/home/jnelson/gasnet18-mellanox-default
# CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit
# LDFLAGS+= -L$(GASNET)/lib

# HUGETLBFS=/home/jnelson/libhugetlbfs-2.12
# CFLAGS+= -I$(HUGETLBFS)/include
# LDFLAGS+= -L$(HUGETLBFS)/lib

# GFLAGS=/home/jnelson/gflags-install
# CFLAGS+= -I$(GFLAGS)/include
# LDFLAGS+= -L$(GFLAGS)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

# GLOG=/home/jnelson/glog-install
# CFLAGS+= -I$(GLOG)/include
# LDFLAGS+= -L$(GLOG)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

# BOOST=/home/jnelson/boost-install
# CFLAGS+= -I$(BOOST)/include
# LDFLAGS+= -L$(BOOST)/lib
# LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

#
# for n01, n02, n04 
#

CC=gcc
CXX=g++
LD=mpiCC

# some library paths
GASNET=/sampa/share/gasnet-rhel6
CFLAGS+= -I$(GASNET)/include -I$(GASNET)/include/ibv-conduit
LDFLAGS+= -L$(GASNET)/lib

HUGETLBFS=/usr
CFLAGS+= -I$(HUGETLBFS)/include
LDFLAGS+= -L$(HUGETLBFS)/lib64

GFLAGS=/sampa/share/gflags
CFLAGS+= -I$(GFLAGS)/include
LDFLAGS+= -L$(GFLAGS)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GFLAGS)/lib

GLOG=/sampa/share/glog
CFLAGS+= -I$(GLOG)/include
LDFLAGS+= -L$(GLOG)/lib
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(GLOG)/lib

EZLOGGER=/sampa/share/ezlogger
CFLAGS+= -I$(EZLOGGER)/include

BOOST=/usr
CFLAGS+= -I$(BOOST)/include
LDFLAGS+= -L$(BOOST)/lib64
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH):$(BOOST)/lib

MPIRUN=mpirun -l




