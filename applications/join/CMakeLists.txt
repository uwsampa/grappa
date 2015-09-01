
set(APP_GRAPH500 ../graph500)
set(APP_BFS "${APP_GRAPH500}/grappa")

set(QUERYLIB_SOURCES
  "${APP_GRAPH500}/prng.c"
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
  MapReduce.cpp
  MapReduce.hpp
  HashJoin.hpp
  HashJoin.cpp
  Aggregates.hpp
  DHT_symmetric.hpp
)
set(QUERYIO_SOURCES
  relation_io.hpp
  relation_io.cpp
  Tuple.hpp
  Tuple.cpp
  relation.hpp
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
  HashJoin_tests.cpp
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
add_library(querylib STATIC EXCLUDE_FROM_ALL ${QUERYLIB_SOURCES})
add_library(queryio STATIC EXCLUDE_FROM_ALL ${QUERYIO_SOURCES})

add_dependencies(queryio all-third-party)
add_dependencies(querylib all-third-party)

# exe targets for standalone query codes
foreach(query ${QUERIES_IND})
  get_filename_component(query_name ${query}, NAME_WE)
  add_grappa_application(${query_name}.exe ${query})
  target_link_libraries(${query_name}.exe generator querylib queryio)
endforeach()

# exe targets for generated query codes
foreach(query ${GENERATED_SOURCES})
  get_filename_component(query_name ${query}, NAME_WE)
  add_grappa_application(${query_name}.exe ${query})
    
  # Raco C++ environment
  set(RACO_DIR "$ENV{RACO_HOME}")
  if (RACO_DIR)
    include_directories($ENV{RACO_HOME}/c_test_environment)
    link_directories($ENV{RACO_HOME}/c_test_environment) # TODO: replace this with something nicer like find()
  else()
      # should only trigger if there exist any generated queries (i.e. applications/join/grappa_*.cpp files)
      message(FATAL_ERROR "Undefined RACO_HOME environment variable, required
      for generated queries applications/join/grappa_*.cpp ")
  endif()

  target_link_libraries(${query_name}.exe generator querylib queryio racoc)
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
  target_link_libraries(${query}.exe generator querylib queryio)
endforeach()


foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} 2 2 ${test_cpp})
  target_link_libraries(${test} querylib queryio)
endforeach()

# create a test, which will be run with the given number of nodes (nnode),
# and processors per node (ppn), and added to the aggregate targets for 
# passing or failing tests
macro(add_check test_cpp nnode ppn target)
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")


  add_grappa_test(${test} ${nnode} ${ppn} ${test_cpp})
  target_link_libraries(${test} generator queryio)
  ### TODO: add separate check-all-join-* targets to include these application files
  ###add_dependencies( check-all-${target} check-${test_name} )
  ###add_dependencies( check-all-${target}-compile-only ${test})
endmacro()

add_check(Relation_io_tests.cpp 2 1 pass)

