# Utilities for Bash scripts

# Setup shflags (option parsing library) (possibly download it)
function init_flags {
  third_party_root="./third-party"
  shflags_src="https://shflags.googlecode.com/files/shflags-1.0.3.tgz"
  shflags_root="${third_party_root}/shflags-1.0.3"

  # if we don't have shflags already, get it
  if [[ ! -e "${shflags_root}/src/shflags" ]]; then
      ( cd "${third_party_root}" && wget -qO- "${shflags_src}" | tar xz )
  fi

  # now source shflags
  source "${shflags_root}/src/shflags"      
}
