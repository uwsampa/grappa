# check if function exists
function fn_exists { declare -f $1 >/dev/null; }

#####################################################################
# Parse flags passed to it. Flags must be pre-defined using 
# `define_flag` before calling this.
#
# TODO: implement short flags
# 
# Typical usage:
# 
#   # define some flags
#   define_flags 'host' 'localhost' 'Hostname to connect to' 'h'
#   define_flags 'port' 4000 'Port to connect to' 'p'
#   
#   parse_flags $@
#   
#   echo "connecting to $FLAGS_host:$FLAGS_port"
#   echo "remaining args -> $FLAGS_extra"
#   
#   ----
#   > ./script --help
#   usage:
#     --help  print this help message
#     --host=localhost  Hostname to connect to
#     --port=4000  Port to connect to
#   
#   > ./script --host=google.com -- -extra
#   connecting to google.com:4000
#   remaining args -> -extra
#
#####################################################################
function parse_flags {
  while (( "$#" )); do
    flag=
    case $1 in
      --) # stop parsing when we reach '--'
        shift
        break
        ;;
      --*=*) # match flags with val after an '=' (e.g. --num=3)
        a=${1##--}
        flag=${a%%=*};
        val=${a##*=}
        ;;
      --*) # match flags with val and space (e.g. --num 3)
        flag=${1/--/}
        # don't use next arg as value if it's a flag
        if [[ "$2" =~ \s*-.* ]]; then
          val=
        else
          val="$2"
        fi
        ;;
      # -*)
      #   echo "short: ${1/-/}"
      #   ;;
      # *)
      #   echo "pos: $1"
      #   ;;
    esac
    if [[ $flag ]]; then
      if fn_exists "__handle_flag_$flag"; then
        eval "__handle_flag_$flag $val"
      else
        echo ">> invalid flag: $flag";
        exit 0
      fi
    fi
    shift
  done
  
  # store the remaining flags for the caller to use
  FLAGS_extra=$@
}

###################################################################
# Define a new flag, initialize variable with default value.
# Note: works like gflags/shflags (but not perfectly compatible)
# 
# Usage:
#
#   define_flag <long_name> <default_value> <description> [<short_name>]
# 
# Defines a flag `--long_name` (and optionally a 1-character 'short' 
# flag, such as '-l').
# 
# The variable `FLAGS_${long_name}` will be used to store the value.
# If this variable was already set in the environment, it will keep 
# that value (so these can be specified via environment variables); 
# if not set already, it will be initialized to the 'default' value.
# Finally, when `parse_flags` is called, it will override the value 
# if the flag is present.
#
# As with gflags, a built-in `--help` or `-h` flag is created which
# prints a message with all of the flags and their descriptions.
#
# example usage:
#   
#   # create flag '--partition'
#   define_flag 'partition' 'sampa' "Slurm partition to use" 'p'
#   
#   # which can be specified in any of these ways:
#   > ./script -p grappa
#   > ./script --partition=grappa
#   > ./script --partition grappa
#   > FLAGS_partition=grappa ./script
#
###################################################################
function define_flag {
  long_name=$1
  default=$2
  desc=$3
  short_name=$4
  
  FLAGS_help_msg="$FLAGS_help_msg
  --$long_name=$default  $desc" 
  
  eval "FLAGS_${long_name}=$default"
  
  eval "__handle_flag_${long_name}() { FLAGS_${long_name}=\$1; declare -x FLAGS_${long_name}; }"
  if [ -n "$short_name" ]; then
    eval "__handle_flag_${short_name}() { FLAGS_${long_name}=\$1; }"
  fi
}

###################################################################
# Define boolean flag, defaults to false.
#
# # Usage:
#
#   define_bool_flag <long_name> <description> [<short_name>]
#
# # To test a flag in a script:
#   if $FLAGS_foo; then echo 'foo'; fi
# # Or to check for false:
#   if [ $FLAGS_foo = false ]; then echo 'not foo'; fi
#
###################################################################
function define_bool_flag {
  long_name=$1
  desc=$3
  short_name=$4
  default=false
  
  FLAGS_help_msg="$FLAGS_help_msg
  --$long_name=$default  $desc" 
  
  eval "FLAGS_${long_name}=$default"
  
  eval "__handle_flag_${long_name}() {
    if [ -n \"\$1\" ]; then
      if flags_true \$1; then
        FLAGS_${long_name}=true
      else
        FLAGS_${long_name}=false
      fi
    else
      FLAGS_${long_name}=true
    fi
    declare -x FLAGS_${long_name}
  }"
  eval "__handle_flag_no-${long_name}() {
    FLAGS_${long_name}=false
    declare -x FLAGS_${long_name}
  }"
  if [ -n "$short_name" ]; then
    eval "__handle_flag_${short_name}() { FLAGS_${long_name}=\$1; }"
  fi
}


function __handle_flag_help {
  echo "usage:
  --help,-h  print this help message$FLAGS_help_msg" >&2
  exit 1
}
function __handle_flag_h { __handle_flag_help; }

function flags_true {
  if [ -n "$1" ]; then
    [ "$1" = "true" ] && return 0
    [ "$1" = "yes" ] && return 0
    [ "$1" = "1" ] && return 0
    
    [ "$1" = "false" ] && return 1
    [ "$1" = "0" ] && return 1
    [ "$1" = "no" ] && return 1
  else
    return 1
  fi
}
