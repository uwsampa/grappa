#!/bin/bash

# Copyright 2010-2012 University of Washington. All Rights Reserved.
# LICENSE_PLACEHOLDER
# This software was created with Government support under DE
# AC05-76RL01830 awarded by the United States Department of
# Energy. The Government has certain rights in the software.

# Grappa contains 3rd party code. Those 3rd party codes are made
# available with original copyright notices and license terms included.

cd GASNet-1.18.2
#make distclean
./configure --prefix=`pwd`/../built_deps --enable-segment-everything CC='gcc -g '
make -j
make install
cd ..

cd gflags
./configure --prefix=`pwd`/../built_deps
make clean
make -j
make install
cd ..

cd google-glog
./configure --prefix=`pwd`/../built_deps
make clean
make -j
make install
cd ..

cd gperftools-2.0
./configure --prefix=`pwd`/../built_deps --enable-frame-pointers
make clean
make -j
make install
cd ..

cd VampirTrace-5.12.2
./configure --prefix=`pwd`/../built_deps --disable-cudawrap
make clean
make -j
make install
cd ..

