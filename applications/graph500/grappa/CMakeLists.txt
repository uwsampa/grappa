
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
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__ -DGRAPH_GENERATOR_GRAPPA)

# build generator.a using other makefile
# (because I'm lazy and we don't change the generator much...)
add_custom_command(OUTPUT generator.a
  COMMAND make -f Makefile.grappa genlib
  COMMAND cp generator.a ../grappa/generator.a
  WORKING_DIRECTORY ../generator
)

set(BFS_VARIANTS
  bfs
  bfs_local
  bfs_beamer
  bfs_nosteam
  bfs_steal_phases
)

foreach(bfs ${BFS_VARIANTS})
  add_executable(${bfs}.exe EXCLUDE_FROM_ALL ${SOURCES})
  target_link_libraries(${bfs}.exe Grappa)
  set_property(TARGET ${bfs}.exe PROPERTY FOLDER "Applications") # For organization in Xcode project
endforeach()
