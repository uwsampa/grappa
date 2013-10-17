
# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

option(TRACING "Sample statistics with VTrace at regular intervals." OFF)

set(GCC_BASE /sampa/share/gcc-4.7.2/rtf)

if( NOT DEFINED ENV{CXX} )
  message(FATAL_ERROR "you must set CC & CXX environment variables!")
  # message("defaulting to GCC 4.7.2")
  # set(CMAKE_C_COMPILER ${GCC_BASE}/bin/gcc)
  # set(CMAKE_CXX_COMPILER ${GCC_BASE}/bin/g++)
endif()

project(Grappa)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()
set(CMAKE_BUILD_TYPE RelWithDebInfo)
site_name(MACHINENAME)

# set(GRAPPA_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/tools/built_deps")
set(GRAPPA_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/extern")

if(TRACING)
  set(GOOGLE_PROFILER ON)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE -DGOOGLE_PROFILER)

  include_directories("${GRAPPA_DEPENDENCY_DIR}/include/vampirtrace")
  set(CMAKE_C_FLAGS "-vt:cc ${GCC_BASE}/bin/gcc ${CMAKE_C_FLAGS} -pthread")
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

###########################################
# External projects
###########################################
set(EXTERN_BUILD_DIR "${CMAKE_BINARY_DIR}/extern")

ExternalProject_Add( extern_gasnet
  PREFIX             ${EXTERN_BUILD_DIR}/gasnet
  STAMP_DIR          ${EXTERN_BUILD_DIR}/gasnet
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gasnet
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/GASNet-1.18.2
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-segment-everything
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/gflags
# URL_HASH           SHA1=bfa0b399e03147b20cafcae62b59ee9163309821 # not supported by Sampa's CMake
ExternalProject_Add( extern_gflags
  URL                "http://gflags.googlecode.com/files/gflags-2.0-no-svn-files.tar.gz"
  SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gflags-2.0
  PREFIX             ${EXTERN_BUILD_DIR}/gflags
  STAMP_DIR          ${EXTERN_BUILD_DIR}/gflags
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gflags
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/google-glog
# URL                http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz
# URL_HASH           SHA1=ed40c26ecffc5ad47c618684415799ebaaa30d65
ExternalProject_Add( extern_glog
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/google-glog
  PREFIX             ${EXTERN_BUILD_DIR}/glog
  STAMP_DIR          ${EXTERN_BUILD_DIR}/glog
  BINARY_DIR         ${EXTERN_BUILD_DIR}/glog
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> CFLAGS=-I${GRAPPA_DEPENDENCY_DIR}/include LDFLAGS=-L${GRAPPA_DEPENDENCY_DIR}/lib
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
  DEPENDS extern_gflags
)
# add_dependencies(extern_glog extern_gflags)

# URL_HASH           SHA1=b799b99d9f021988bbc931db1c21b2f94826d4f0
ExternalProject_Add( extern_gperftools
  URL                http://gperftools.googlecode.com/files/gperftools-2.1.tar.gz
  SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gperftools-2.1
  PREFIX             ${EXTERN_BUILD_DIR}/gperftools
  STAMP_DIR          ${EXTERN_BUILD_DIR}/gperftools
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gperftools
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-frame-pointers
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# set(extern_targets  extern_gasnet extern_gflags extern_glog extern_gperftools )
add_custom_target(all_externs DEPENDS extern_gasnet extern_gflags extern_glog extern_gperftools )

# pre-compiled external dependencies
# TODO: make CMake build these (see GraphLab's CMake)
include_directories("${GRAPPA_DEPENDENCY_DIR}/include")
link_directories("${GRAPPA_DEPENDENCY_DIR}/lib")
list(APPEND GRAPPA_LIBS
  gflags
  glog
)

if(TRACING)
  set(GOOGLE_PROFILER ON)

  list(APPEND GRAPPA_LIBS vt otf z papi dl m)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE)

  # set(CMAKE_C_COMPILER "${GRAPPA_DEPENDENCY_DIR}/bin/vtcc")
  include_directories("${GRAPPA_DEPENDENCY_DIR}/include/vampirtrace")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lvt -lotf -lz -lpapi -ldl -lm")
endif()

if( GOOGLE_PROFILER )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  add_definitions( -DGOOGLE_PROFILER )
endif()

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
include_directories("${GRAPPA_DEPENDENCY_DIR}/include/${GASNET_CONDUIT}-conduit")

list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)

# add_definitions(-DSHMMAX=12884901888) # 12 GB
add_definitions(-DSHMMAX=6442450944) # 6 GB

get_filename_component(CC_PATH ${CMAKE_C_COMPILER} PATH)
#link_directories("${CC_PATH}/../lib64")

# make sure we use the correct libstdc++
# link_directories("${GCC_BASE}/bin/../lib64")

if(NOT APPLE)
  list(APPEND GRAPPA_LIBS pthread rt)
endif()

# boost is separate
if(APPLE)
  set(BOOST_BASE "/usr/local")
  include_directories("${BOOST_BASE}/include")
  link_directories("${BOOST_BASE}/lib")
else()
  set(BOOST_BASE "/sampa/share/gcc-4.7.2/src/boost_1_51_0")
  include_directories("${BOOST_BASE}")
  link_directories("${BOOST_BASE}/stage/lib")
endif()
# find_library(BOOST boost PATHS ${BOOST_BASE} REQUIRED)

# TODO: look for {,-mt} and pick what's available
if(APPLE)
  list(APPEND GRAPPA_LIBS
    boost_unit_test_framework-mt
    boost_filesystem-mt
    boost_system-mt
  )
else()
  list(APPEND GRAPPA_LIBS
    boost_unit_test_framework
    boost_filesystem
    boost_system
  )
endif()

# MPI (mostly for booting, but we also use MPI communication in some places now)
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})

macro(add_grappa_exe name)
    add_executable(${name} EXCLUDE_FROM_ALL ${ARGN})
    target_link_libraries(${name} Grappa)
    target_link_libraries(${name} ${MPI_CXX_LIBRARIES})
endmacro(add_grappa_exe)

macro(add_grappa_application name)
  add_grappa_exe(${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications") # For organization in Xcode project
endmacro(add_grappa_application)

macro(add_grappa_test name)
  add_grappa_exe(${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${test_name}"
  )
endmacro(add_grappa_test)

# put Grappa system directory on include path for everything following this
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/system")
include_directories(system)
include_directories(system/tasks)


add_subdirectory(system)
add_subdirectory(applications)
add_subdirectory(bin)

add_subdirectory(scratch)

