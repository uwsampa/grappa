#!/bin/bash

cd GASNet-1.18.2
make distclean
./configure --prefix=`pwd`/../built_deps
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
<<<<<<< HEAD
make -j
make install
cd ..

cd gperftools-2.0
./configure --prefix=`pwd`/../built_deps
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

