#!/bin/bash

cd GASNet-1.18.2
#make distclean
./configure --prefix=`pwd`/../built_deps --enable-segment-everything CC='cc -g '
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
./configure --prefix=`pwd`/../built_deps
make clean
make -j
make install
cd ..

