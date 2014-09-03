#####################################################################
# Common BASH helpers, including a mini flag-parsing library.
#####################################################################

function has_srun {
  type srun >/dev/null 2>&1
}

