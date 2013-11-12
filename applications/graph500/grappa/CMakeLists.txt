
set(SOURCES
  common.h
  oned_csr.h
  oned_csr.cpp
  options.h
  options.cpp
  timer.h
  verify.hpp
  verify.cpp
  ../prng.c
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

set(BFS_VARIANTS
  bfs
  bfs_local
  bfs_beamer
  bfs_nosteal
  bfs_steal_phases
)

# make separate build targets for each BFS variant
foreach(bfs ${BFS_VARIANTS})
  add_grappa_application(${bfs}.exe ${SOURCES} main.cpp ${bfs}.cpp)
  target_link_libraries(${bfs}.exe generator)
endforeach()

add_grappa_application(graph_new.exe ${SOURCES} main_new.cpp bfs_local_adj.cpp cc_new.cpp)
target_link_libraries(graph_new.exe generator)
