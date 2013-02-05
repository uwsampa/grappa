#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo Configuring CMake build...
cd $DIR
mkdir build
cd build

# set up compiler (done by cmake now, unless overridden here)
#BASE=/sampa/share/gcc-4.7.2/rtf
#export CC=$BASE/bin/gcc
#export CXX=$BASE/bin/g++

# run cmake (uses CC and CXX variables to select which compiler to use)
# note: this will use 'Make' as the build system on Linux; if you would like to use
#       something different like 'Ninja' or 'Xcode', simply specify 'cmake -G Ninja'
cmake ..

echo "Configure complete. To build:"
echo "> cd build; make"
