#!/bin/bash

########################################################################
## This file is part of Grappa, a system for scaling irregular
## applications on commodity clusters. 

## Copyright (C) 2010-2014 University of Washington and Battelle
## Memorial Institute. University of Washington authorizes use of this
## Grappa software.

## Grappa is free software: you can redistribute it and/or modify it
## under the terms of the Affero General Public License as published
## by Affero, Inc., either version 1 of the License, or (at your
## option) any later version.

## Grappa is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## Affero General Public License for more details.

## You should have received a copy of the Affero General Public
## License along with this program. If not, you may obtain one from
## http://www.affero.org/oagpl.html.
########################################################################

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
./configure --prefix=`pwd`/../built_deps
make clean
make -j
make install
cd ..

