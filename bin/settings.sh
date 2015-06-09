#!/bin/bash

# find Grappa installation location
SCRIPT_PATH="${BASH_SOURCE[0]}";
if ([ -h "${SCRIPT_PATH}" ])
then
    while([ -h "${SCRIPT_PATH}" ])
    do
        SCRIPT_PATH=`readlink "${SCRIPT_PATH}"`
    done
fi
pushd . > /dev/null
cd `dirname ${SCRIPT_PATH}` > /dev/null
SCRIPT_PATH=`pwd`
cd ..
GRAPPA_PREFIX=`pwd`
popd  > /dev/null

# make Grappa installation location visible
export GRAPPA_PREFIX

# load important Grappa environment variables
source $GRAPPA_PREFIX/bin/env.sh

