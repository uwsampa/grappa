
set(SOURCES
  common.h
  main.cpp
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
  add_executable(${bfs}.exe EXCLUDE_FROM_ALL ${SOURCES} ${bfs}.cpp)
  target_link_libraries(${bfs}.exe Grappa generator)
  set_property(TARGET ${bfs}.exe PROPERTY FOLDER "Applications") # For organization in Xcode project
endforeach()
