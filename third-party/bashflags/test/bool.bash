#!/bin/bash
DIR="${BASH_SOURCE%/*}"
source $DIR/../flags.bash

define_bool_flag 'foo' 'help text'

parse_flags $@

if $FLAGS_foo; then
  echo "foo"
else
  echo "not foo"
fi
