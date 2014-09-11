#!/bin/bash
DIR="${BASH_SOURCE%/*}"
source $DIR/../flags.bash

define_bool_flag 'foo' 'help text'
define_bool_flag 'bar' 'useless flag'

parse_flags $@

if $FLAGS_foo; then
  echo "foo"
else
  echo "not foo"
fi
