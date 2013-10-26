
# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

option(TRACING "Sample statistics with VTrace at regular intervals." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# if( NOT DEFINED ENV{CXX} )
#   message(FATAL_ERROR "you must set CC & CXX environment variables!")
# endif()

project(Grappa)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

set(CMAKE_C_FLAGS_DEBUG "-O1 -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -DDEBUG")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
site_name(MACHINENAME)

# set(TOOLS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/tools/built_deps")
# set(TOOLS_INSTALL_DIR "${CMAKE_SOURCE_DIR}/extern")
set(TOOLS_INSTALL_DIR "${CMAKE_BINARY_DIR}/tools")

if(TRACING)
  # TODO:Fix VampirTrace support
  
  set(GOOGLE_PROFILER ON)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE -DGOOGLE_PROFILER)

  include_directories("${TOOLS_INSTALL_DIR}/include/vampirtrace")
  
  set(GCC_BASE /sampa/share/gcc-4.7.2/rtf)
  set(CMAKE_C_FLAGS "-vt:cc  ${CMAKE_C_FLAGS} -pthread")
  set(CMAKE_CXX_FLAGS "-vt:cxx ${GCC_BASE}/bin/g++ ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lvt -lotf -lz -lpapi -ldl -lm")
endif()

# global C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Winline -Wno-inline -mno-red-zone")
add_definitions("-DENABLE_RDMA_AGGREGATOR")


# TODO: use -stdlib=libc++ too?

###########################
# Use RUNPATH if available
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
   set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

# TODO: check if not OSX, or find out how to do "try" in CMake
# set runpath, too
if(NOT APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
endif()
##### </RUNPATH> ########

if(TRACING)
  set(GOOGLE_PROFILER ON)

  list(APPEND GRAPPA_LIBS vt otf z papi dl m)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE)

  # set(CMAKE_C_COMPILER "${TOOLS_INSTALL_DIR}/bin/vtcc")
  include_directories("${TOOLS_INSTALL_DIR}/include/vampirtrace")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lvt -lotf -lz -lpapi -ldl -lm")
endif()

if( GOOGLE_PROFILER )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  add_definitions( -DGOOGLE_PROFILER )
endif()

# pre-compiled external dependencies
# TODO: make CMake build these (see GraphLab's CMake)
include_directories("${TOOLS_INSTALL_DIR}/include")
link_directories("${TOOLS_INSTALL_DIR}/lib")
list(APPEND GRAPPA_LIBS
  gflags
  glog
)

######################################
# Gasnet
######################################
if(DEFINED APPLE)
  set(GASNET_CONDUIT "udp")
else()
  set(GASNET_CONDUIT "ibv") # values: ibv,mpi
endif()

set(GASNET_THREAD "seq")  # values: seq,par,parsync -- seq recommended
list(APPEND GRAPPA_LIBS "gasnet-${GASNET_CONDUIT}-${GASNET_THREAD}")

if( ${GASNET_CONDUIT} MATCHES "ibv")
  list(APPEND GRAPPA_LIBS "ibverbs")
endif()
if( ${GASNET_CONDUIT} MATCHES "mpi")
  list(APPEND GRAPPA_LIBS "ammpi")
endif()
if( ${GASNET_CONDUIT} MATCHES "udp")
  list(APPEND GRAPPA_LIBS "amudp")
endif()

string(TOUPPER ${GASNET_CONDUIT} GASNET_CONDUIT_UP)
string(TOUPPER ${GASNET_THREAD} GASNET_THREAD_UP)

add_definitions(
  "-DGASNET_${GASNET_THREAD_UP}"
  "-DGASNET_${GASNET_CONDUIT_UP}"
  "-DGASNET_CONDUIT_${GASNET_CONDUIT_UP}"
)
include_directories("${TOOLS_INSTALL_DIR}/include/${GASNET_CONDUIT}-conduit")

list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)


add_definitions(-DSHMMAX=12884901888) # 12 GB
# add_definitions(-DSHMMAX=6442450944) # 6 GB

if(NOT APPLE)
  list(APPEND GRAPPA_LIBS pthread rt)
endif()

# build these boost libraries... (see tools/CMakeLists.txt for the ExternalProject declaration)
set(Boost_LIBRARIES boost_filesystem boost_system boost_unit_test_framework )

# set(BOOST_ROOT ...)
# find_package(Boost REQUIRED COMPONENTS filesystem system unit_test_framework )
# include_directories(${Boost_INCLUDE_DIR})
# link_directories(${Boost_LIBRARY_DIRS})

# MPI (mostly for booting, but we also use MPI communication in some places now)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

macro(add_grappa_exe name )
  add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
  target_link_libraries(${name} Grappa)
  target_link_libraries(${name} ${MPI_CXX_LIBRARIES})
  target_link_libraries(${name} ${Boost_LIBRARIES})
endmacro(add_grappa_exe)

macro(add_grappa_application name)
  add_grappa_exe(${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications") # For organization in Xcode project
endmacro(add_grappa_application)

set(GRAPPA_TESTS "")

macro(add_grappa_test name)
  get_filename_component(base ${name} NAME_WE) # name without extension
  
  add_grappa_exe(${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${base}"
  )
  
  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${base}"
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_srun.rb --test=${base} --nnode=2 --ppn=2
    DEPENDS Grappa ${name}
  )
  set_property(TARGET "check-${base}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${base}")
endmacro(add_grappa_test)


# put Grappa system directory on include path for everything following this
include_directories(system)
include_directories(system/tasks)

####################################
# subdirectories
add_subdirectory(applications)
add_subdirectory(bin)
add_subdirectory(scratch)
add_subdirectory(system)
add_subdirectory(tools)
