
set(APP_GRAPH500 ../graph500)
set(APP_BFS "${APP_GRAPH500}/grappa")

set(SOURCES
  "${APP_GRAPH500}/prng.c"
  Tuple.hpp
  Tuple.cpp
  relation_io.hpp
  relation_io.cpp
  relation.hpp
  utils.h
  MatchesDHT.hpp
  MatchesDHT.cpp
  DoubleDHT.hpp
  Hypercube.hpp
  Hypercube.cpp
  local_graph.cpp
  local_graph.hpp
  utility.hpp
  utility.cpp
  "${APP_BFS}/oned_csr.h"
  "${APP_BFS}/oned_csr.cpp"
  "${APP_BFS}/timer.h"
  "${APP_BFS}/common.h"
  "${APP_BFS}/graph.hpp"
  "${APP_BFS}/graph.cpp"
  stats.h
  stats.cpp
  strings.h
  strings.cc
  MapReduce.cpp
  MapReduce.hpp
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

FILE(GLOB GENERATED_SOURCES "grappa_*.cpp")

set(QUERIES_IND 
  triangles.cpp
  triangles_partition.cpp
  twohop.cpp
  DoubleDHT_test.cpp
  MapReduce_tests.cpp
  KMeansMR.cpp
  convert2bin.cpp
)

set(QUERIES_SOURCES
  squares.cpp
  squares.hpp
  squares_partition.cpp
  squares_partition.hpp
  squares_bushy.cpp
  squares_bushy.hpp
  squares_partition_bushy.cpp
  squares_partition_bushy.hpp
  Query.hpp
)

set(TEST_SOURCES
  Local_graph_tests.cpp
  Hypercube_tests.cpp
)
  
include_directories(${INCLUDE_DIRS})

# query code dependences
add_library(querylib STATIC EXCLUDE_FROM_ALL ${SOURCES})

# exe targets for standalone query codes
foreach(query ${QUERIES_IND})
  get_filename_component(query_name ${query}, NAME_WE)
  add_grappa_application(${query_name}.exe ${query})
  target_link_libraries(${query_name}.exe generator querylib)
endforeach()

# exe targets for generated query codes
foreach(query ${GENERATED_SOURCES})
  get_filename_component(query_name ${query}, NAME_WE)
  add_grappa_application(${query_name}.exe ${query})
  target_link_libraries(${query_name}.exe generator querylib)
  list(APPEND GENERATED_EXES "${query_name}.exe")
endforeach()

# add aggregate target to build the executables for 
# all of the generated queries
add_custom_target(compile-gens)
# only add dependencies if the list isn't empty
if (GENERATED_EXES)
  add_dependencies(compile-gens ${GENERATED_EXES})
endif()

# exe target for top level query runner
foreach(query ${QUERIES_MAIN})
  add_grappa_application(${query}.exe ${QUERIES_SOURCES} ${query}.cpp)
  target_link_libraries(${query}.exe generator querylib)
endforeach()


foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} 2 2 ${test_cpp})
  target_link_libraries(${test} querylib)
endforeach()


