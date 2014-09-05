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

test "./bool.bash" "not foo"

test "./bool.bash --foo" "foo"

test "./bool.bash --foo=true" "foo"

test "./bool.bash --foo=false" "not foo"

echo "All tests successful!"
