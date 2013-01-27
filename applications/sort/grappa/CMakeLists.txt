
set(SOURCES
  main.cpp
  npb_intsort.h
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
add_definitions(-Drestrict=__restrict__)

add_executable(sort.exe EXCLUDE_FROM_ALL ${SOURCES})
target_link_libraries(sort.exe Grappa)
set_property(TARGET sort.exe PROPERTY FOLDER "Applications") # For organization in Xcode project
