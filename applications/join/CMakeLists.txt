
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
  Query.hpp
)

set(TEST_SOURCES
  Local_graph_tests.cpp
  Hypercube_tests.cpp
)

foreach(query ${QUERIES_IND})
  #not including headers because sources use explicit path
  add_grappa_application(${query}.exe ${SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator)
endforeach()

foreach(query ${QUERIES_MAIN})
  #not including headers because sources use explicit path
  add_grappa_application(${query}.exe ${SOURCES} ${QUERIES_SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator)
endforeach()


foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} ${SOURCES} ${test_cpp})
  target_link_libraries(${test} generator)
endforeach()


