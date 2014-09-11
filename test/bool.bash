#!/bin/bash
DIR="${BASH_SOURCE%/*}"
source $DIR/../flags.bash

define_bool_flag 'foo' 'help text'
define_bool_flag 'bar' 'useless flag'

parse_flags $@

if flags_true $FLAGS_foo && [ $FLAGS_foo = true ] && $FLAGS_foo; then
  echo "$FLAGS_foo"
else
  echo "$FLAGS_foo"
fi
