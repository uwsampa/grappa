
set(SOURCES
  common.h
  oned_csr.h
  oned_csr.cpp
  options.h
  options.cpp
  timer.h
  ../prng.c
  simple_graphs.hpp
  simple_graphs.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

add_grappa_application(isopath.exe ${SOURCES} isopath.cpp)
target_link_libraries(isopath.exe generator)
