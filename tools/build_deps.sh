#!/bin/bash

cd GASNet-1.18.2
./configure --prefix=`pwd`/build
make -j
make install
cd ..

cd gflags
./configure
make clean
make -j
cd ..

cd google-glog
./configure
make clean
make -j
cd ..

cd gperftools-2.0
./configure
make clean
make -j
cd ..

cd VampirTrace-5.12.2
./configure --prefix=`pwd`/build
make clean
make -j
make install
cd ..

