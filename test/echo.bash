#!/bin/bash
DIR="${BASH_SOURCE%/*}"
source $DIR/../flags.bash

define_flag 'text' 'default' 'help text'

parse_flags $@

echo "text=$FLAGS_text,extra=$FLAGS_extra"
