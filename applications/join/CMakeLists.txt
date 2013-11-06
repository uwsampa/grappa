
set(APP_GRAPH500 ../graph500)
set(APP_BFS "${APP_GRAPH500}/grappa")

set(SOURCES
  "${APP_GRAPH500}/prng.c"
  Tuple.hpp
  Tuple.cpp
  relation_IO.hpp
  DHT.hpp
  "${APP_BFS}/oned_csr.h"
  "${APP_BFS}/oned_csr.cpp"
  "${APP_BFS}/timer.h"
  "${APP_BFS}/common.h"
  "${APP_BFS}/graph.hpp"
  "${APP_BFS}/graph.cpp"
)
#TODO: encapsulate graph500/grappa sources

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

set(QUERIES
  triangles
  triangles_partition
  twohop
)

foreach(query ${QUERIES})
  #not including headers because sources use explicit path
  add_grappa_application(${query}.exe ${SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator)
endforeach()

