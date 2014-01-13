
set(APP_GRAPH500 ../graph500)
set(APP_BFS "${APP_GRAPH500}/grappa")

set(SOURCES
  "${APP_GRAPH500}/prng.c"
  Tuple.hpp
  Tuple.cpp
  relation_io.hpp
  DHT.hpp
  Hypercube.hpp
  Hypercube.cpp
  local_graph.cpp
  local_graph.hpp
  "${APP_BFS}/oned_csr.h"
  "${APP_BFS}/oned_csr.cpp"
  "${APP_BFS}/timer.h"
  "${APP_BFS}/common.h"
  "${APP_BFS}/graph.hpp"
  "${APP_BFS}/graph.cpp"
)
set(INCLUDE_DIRS
  "${APP_GRAPH500}"
)

#TODO: encapsulate graph500/grappa sources

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

set(QUERIES_MAIN
  Query
)

set(QUERIES_IND 
  triangles
  triangles_partition
  twohop
)

set(QUERIES_SOURCES
  squares.cpp
  squares.hpp
  squares_partition.cpp
  squares_partition.hpp
  squares_bushy.cpp
  squares_bushy.hpp
  Query.hpp
)

set(TEST_SOURCES
  Local_graph_tests.cpp
  Hypercube_tests.cpp
)
  
include_directories(${INCLUDE_DIRS})


foreach(query ${QUERIES_IND})
  add_grappa_application(${query}.exe ${SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator)
endforeach()

foreach(query ${QUERIES_MAIN})
  add_grappa_application(${query}.exe ${SOURCES} ${QUERIES_SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator)
endforeach()


foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} 2 2 ${SOURCES} ${test_cpp})
  target_link_libraries(${test} generator)
endforeach()


