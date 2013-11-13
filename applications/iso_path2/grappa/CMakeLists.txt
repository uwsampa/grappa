
set(SOURCES
  common.h
  oned_csr.h
  oned_csr.cpp
  options.h
  options.cpp
  rmat.h
  rmat.cpp
  timer.h
  verify.hpp
  verify.cpp
  ../prng.c
  simple_graphs.hpp
  simple_graphs.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

add_grappa_application(iso_path.exe ${SOURCES} iso_path.cpp)
target_link_libraries(iso_path.exe generator)
