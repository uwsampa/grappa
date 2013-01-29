
# change the UTS_RNG value by running the following command (from your `build` directory):
# > cmake -DUTS_RNG=Devine ..

if(NOT DEFINED UTS_RNG)
  set(UTS_RNG BRG CACHE STRING "Which random number generator to use for UTS.")
  set_property(CACHE UTS_RNG PROPERTY STRINGS BRG Devine ALFG)
endif()

if(${UTS_RNG} MATCHES BRG)
  set(RNG_NAME rng/brg_sha1)
  set(RNG_DEF -DBRG_RNG)

elseif(${UTS_RNG} MATCHES ALFG)
  set(RNG_NAME rng/alfg)
  set(RNG_DEF -DUTS_ALFG)

# these were in the makefile but no sources for devine_sha1
# elseif(${UTS_RNG} MATCHES Devine)
#   set(RNG_NAME rng/devine_sha1)
#   set(RNG_DEF -DDEVINE_RNG)

endif()

set(SOURCES
  uts_grappa.cpp
  uts.c
  ${RNG_NAME}.c
  ${RNG_NAME}.h
)
# compile uts.c as C++ file...
set_source_files_properties( uts.c PROPERTIES LANGUAGE CXX )

add_definitions(${RNG_DEF})

add_grappa_application(uts_grappa.exe ${SOURCES})
