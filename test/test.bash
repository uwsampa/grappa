#!/bin/bash
DIR="${BASH_SOURCE%/*}"
cd $DIR

function test {
  cmd=$1
  correct=$2
  echo ">> $cmd"
  output=$(eval $cmd)
  if [[ "$output" != $correct ]]; then
    echo $'error:\n  correct: '$correct$'\n  output:  '$output
    exit 1
  fi
}

test "./echo.bash" "text=default,extra="

test "./echo.bash --text=foo" "text=foo,extra="

test "./echo.bash --text=foo -- extra" "text=foo,extra=extra"

test "./echo.bash -- extra" "text=default,extra=extra"

test "./echo.bash --help 2>&1" "*help text*"

test "./bool.bash" "false"

test "./bool.bash --foo" "true"

test "./bool.bash --no-foo" "false"

test "./bool.bash --foo=true" "true"

test "./bool.bash --foo=false" "false"

test "./bool.bash --foo=yes" "true"

test "./bool.bash --foo=1" "true"

test "./bool.bash --foo=no" "false"

test "./bool.bash --foo=0" "false"

test "./bool.bash --foo --bar" "true"

echo "All tests successful!"
