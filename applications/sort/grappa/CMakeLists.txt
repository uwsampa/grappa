
set(SOURCES
  main.cpp
  npb_intsort.h
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__)

add_grappa_application(sort.exe ${SOURCES})
