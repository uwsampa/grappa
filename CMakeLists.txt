
# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)
include(ExternalProject)

option(TRACING "Sample statistics with VTrace at regular intervals." OFF)
option(PROFILING "Sample profile with GPerftools at regular intervals." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# if( NOT DEFINED ENV{CXX} )
#   message(FATAL_ERROR "you must set CC & CXX environment variables!")
# endif()

# set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.clang.3_4")

project(Grappa)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")

set(CMAKE_C_FLAGS_DEBUG "-O1 -DDEBUG -g -DGUARD_PAGES_ON_STACK")
set(CMAKE_CXX_FLAGS_DEBUG "-O1 -DDEBUG -g -DGUARD_PAGES_ON_STACK")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
site_name(MACHINENAME)


###########################
# Use RUNPATH if available
# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)
# use final INSTALL_RPATH even in build tree (this lets us manually add things to CMAKE_INSTALL_RPATH)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${CMAKE_INSTALL_RPATH}")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
# set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1") # not a system directory
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")

## Not setting runpath because having MPI libs in LD_LIBRARY_PATH was messing up VampirTrace's ability to find its own libs. Maybe there's another way to fix this, but it just seems more robust (for now) to not allow LD_LIBRARY_PATH to affect our libs (after configure uses it to find them).
# set runpath, too
# if(NOT APPLE)
#   set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")
# endif()

##### </RUNPATH> ########


#############################################################################
# configure profiling and tracing
#############################################################################
if(TRACING)
  # TODO:Fix VampirTrace support
  message("-- Tracing enabled.")
  
  set(GOOGLE_PROFILER ON)

  add_definitions(-DVTRACE_SAMPLED -DVTRACE -DGOOGLE_PROFILER)

  ### TODO: one of these is correct
  include_directories("${THIRD_PARTY_ROOT}/include/vampirtrace")
  
  include_directories("${VAMPIR_ROOT}/include/vampirtrace")
  link_directories("${VAMPIR_ROOT}/lib")
  
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finstrument-functions")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finstrument-functions")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -vt:inst manual")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -vt:inst manual")
  
  # list(APPEND GRAPPA_LIBS vt otf z papi dl m)
  
  # set(CMAKE_C_FLAGS "-vt:cc  ${CMAKE_C_FLAGS} -pthread")
  # set(CMAKE_CXX_FLAGS "-vt:cxx ${GCC_BASE}/bin/g++ ${CMAKE_CXX_FLAGS}")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --as-needed --whole-archive -lvt --no-whole-archive --no-as-needed -lopen-trace-format -lz -lpapi -ldl -lm")

  if(NOT THIRD_PARTY_ROOT STREQUAL ${VAMPIR_ROOT})
    set(CMAKE_INSTALL_RPATH "${VAMPIR_ROOT}/lib:${CMAKE_INSTALL_RPATH}")
  endif()
endif()

if(PROFILING)
  message("-- Profiling enabled.")
  set(GOOGLE_PROFILER ON)
endif()

if( GOOGLE_PROFILER )
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  add_definitions( -DGOOGLE_PROFILER )
endif()

# global C++ flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Winline -Wno-inline -mno-red-zone")
add_definitions("-DENABLE_RDMA_AGGREGATOR")

# TODO: use -stdlib=libc++ too?




#########################################
# Determine third party tools to build
set(third_party_docstring "Directory to look for/install third-party stuff to.")
if(NOT THIRD_PARTY_ROOT)
  set(THIRD_PARTY_ROOT "${CMAKE_BINARY_DIR}/third-party" CACHE PATH "${third_party_docstring}")
else()
  set(THIRD_PARTY_ROOT "${THIRD_PARTY_ROOT}" CACHE PATH "${third_party_docstring}")
endif()
include_directories("${THIRD_PARTY_ROOT}/include")
link_directories("${THIRD_PARTY_ROOT}/lib")
set(CMAKE_INSTALL_RPATH "${THIRD_PARTY_ROOT}/lib:${CMAKE_INSTALL_RPATH}")

######################################################################
# MPI
######################################################################
find_package(MPI REQUIRED)
include_directories(${MPI_INCLUDE_PATH})
list(APPEND GRAPPA_LIBS ${MPI_CXX_LIBRARIES})

######################################################################
# PMI: helpful for some systems with Slurm
######################################################################
find_library(PMI_FOUND pmi)
if(PMI_FOUND)
  list(APPEND GRAPPA_LIBS pmi)
endif()

######################################################################
# Pthreads
######################################################################
if(NOT APPLE)
  find_library(PTHREADS_FOUND pthread REQUIRED)
  list(APPEND GRAPPA_LIBS pthread)
endif()

######################################################################
# POSIX realtime extensions
######################################################################
if(NOT APPLE)
  find_library(RT_FOUND rt REQUIRED)
  list(APPEND GRAPPA_LIBS rt)
endif()

######################################################################
# Google flags
# in theory, it may come from third-party or from system directories,
# but for simplicity now just use the one in third-party.
######################################################################
find_library(GFLAGS_FOUND libgflags.a gflags ${THIRD_PARTY_ROOT}/lib NO_CMAKE_SYSTEM_PATH)
message("GFLAGS_FOUND: ${GFLAGS_FOUND}")
list(APPEND GRAPPA_LIBS libgflags.a)

######################################################################
# Google logging (Grappa-customized)
######################################################################
find_library(GLOG_FOUND libglog.a glog ${THIRD_PARTY_ROOT}/lib NO_CMAKE_SYSTEM_PATH)
message("GLOG_FOUND: ${GLOG_FOUND}")
list(APPEND GRAPPA_LIBS libglog.a)
list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)

######################################################################
# Graph500 Kronecker generator (Grappa-customized)
######################################################################
find_library(GRAPH500_FOUND libgraph500-generator.a graph500-generator ${THIRD_PARTY_ROOT}/graph500-generator NO_CMAKE_SYSTEM_PATH)
message("GRAPH500_FOUND: ${GRAPH500_FOUND}")
include_directories("${CMAKE_SOURCE_DIR}/third-party/graph500-generator")
link_directories("${THIRD_PARTY_ROOT}/graph500-generator")
list(APPEND GRAPPA_LIBS libgraph500-generator.a)

######################################################################
# Boost
######################################################################
set(BOOST_COMPONENTS filesystem system unit_test_framework)

if(VERBOSE)
  set(Boost_DETAILED_FAILURE_MSG ON)
  set(Boost_DEBUG ON)
endif()

# ideally we would verify we can find both static and dynamic versions
# of the libraries, since we prefer each for different parts of
# Boost. But since there's not good way to select both in
# FindBoost.cmake, for now we'll just assume if we find a Boost
# installation it will have both.
### set(Boost_USE_STATIC_LIBS ON)

# try single-threaded first
set(Boost_USE_MULTITHREADED OFF)

#
# first, see if we already built a copy of boost in third-party; prefer that over any other boost
#

# save user-specified pointers to boost
if(DEFINED BOOSTROOT)
  set(BOOSTROOT_BACKUP ${BOOST_ROOT})
  set(RESTORE_BOOSTROOT 1)
  unset(BOOSTROOT)
endif()
if(DEFINED BOOST_ROOT)
  set(BOOST_ROOT_BACKUP ${BOOST_ROOT})
  set(RESTORE_BOOST_ROOT 1)
endif()

# now look in third-party
set(BOOST_ROOT ${THIRD_PARTY_ROOT})
set(Boost_NO_SYSTEM_PATHS 1)
if(VERBOSE)
  find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS})
else()
  find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS} QUIET)
endif()

#
# if boost was not found in third-party, restore any user-specified pointers and search there and in standard paths
#
if(NOT Boost_FOUND)

  # restore any user-specified pointers
  unset(Boost_NO_SYSTEM_PATHS)
  if(BOOSTROOT_BACKUP)
    set(BOOSTROOT ${BOOSTROOT_BACKUP})
  endif()
  if(BOOST_ROOT_BACKUP)
    set(BOOST_ROOT ${BOOST_ROOT_BACKUP})
  endif()

  # search for single-threaded version
  if(VERBOSE)
    find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS})
  else()
    find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS} QUIET)
  endif()

  # if no single-threaded, try multi-threaded next
  if(NOT Boost_FOUND)
    set(Boost_USE_MULTITHREADED ON)
    if(VERBOSE)
      find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS})
    else()
      find_package(Boost 1.51 COMPONENTS ${BOOST_COMPONENTS} QUIET)
    endif()
  endif()
endif()

# if we did find a pre-installed version, add its paths to our build environment
if(Boost_FOUND)
  message("-- Boost found:  ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION} -- ${BOOST_PREFIX}")
  include_directories(${Boost_INCLUDE_DIR})
  link_directories(${Boost_LIBRARY_DIRS})
  string(REGEX REPLACE "/include" "" BOOST_PREFIX ${Boost_INCLUDE_DIR} )
  # Boost_LIBRARIES is now set with all the libraries we requested
  foreach(lib ${Boost_LIBRARY_DIRS})
    set(CMAKE_INSTALL_RPATH "${lib}:${CMAKE_INSTALL_RPATH}")
  endforeach()
else()
  message("-- Boost not found. Building.")
  # build is specified in third-party/CMakeLists.txt

  # now we need to set flags appropriately.
  # include path is fine since we already include THIRD_PARTY_ROOT
  # link path is fine since we already include THIRD_PARTY_ROOT
  # RPATH is fine since we already include THIRD_PARTY_ROOT
  set(Boost_INCLUDE_DIR ${THIRD_PARTY_ROOT}/include)
  set(Boost_LIBRARY_DIR ${THIRD_PARTY_ROOT}/lib)
  list(APPEND tool_list third-party-boost)
endif()

# we don't use ${Boost_LIBRARIES} because we want to link the
# following two libraries statically for all targets, and the unit
# test library dynamically, but only on test targets.
list(APPEND GRAPPA_LIBS
  libboost_system.a
  libboost_filesystem.a)

message("Boost_LIBRARIES: ${Boost_LIBRARIES}")
message("GRAPPA_LIBS: ${GRAPPA_LIBS}")

####################################
# Load all third party libraries
add_subdirectory(third-party)




if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # clang doesn't have this flag it seems, needed for Relation_io_tests.cpp in Travis
  add_definitions( -D__extern_always_inline=inline )
endif()

macro(add_grappa_exe target exe )
  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_NAME "${exe}")
  target_link_libraries(${target} 
    Grappa
  )
endmacro(add_grappa_exe)

macro(add_grappa_application name)
  add_grappa_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications") # For organization in Xcode project
endmacro(add_grappa_application)

set(GRAPPA_TESTS "")

macro(add_grappa_test name nnode ppn)
  get_filename_component(base ${name} NAME_WE) # name without extension
  
  add_grappa_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Tests") # For organization in Xcode project
  target_link_libraries(${name} boost_unit_test_framework)
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${base}"
  )
  
  math(EXPR n "${nnode}*${ppn}")
  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${base}"
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_run --nnode=1 --ppn=${n} --verbose -- ${base}.test
    DEPENDS Grappa ${name}
  )
  
  set_property(TARGET "check-${base}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${base}")
  
  # Add test target with dependent build command
  set(target "test-${name}")
  add_test(
    NAME ${target}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND "${CMAKE_BINARY_DIR}/bin/grappa_run" -n1 -p${n} -- ${name}
  )
  add_test(${target}.build "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${name})
  set_tests_properties(${target} PROPERTIES DEPENDS ${target}.build)
  
endmacro(add_grappa_test)

####################
# find igor scripts
file(GLOB_RECURSE igor_scripts
  RELATIVE ${CMAKE_SOURCE_DIR}
  igor_*
)
string(REGEX REPLACE "build/[^;]+;?" "" igor_scripts "${igor_scripts}") # filter out build directory

foreach(script ${igor_scripts})
  get_filename_component(out_dir "${script}" PATH)
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${out_dir})
  execute_process(COMMAND ln -sf ${CMAKE_SOURCE_DIR}/${script} ${CMAKE_BINARY_DIR}/${script})
endforeach()
# message("igor scripts: ${igor_scripts}")


# put Grappa system directory on include path for everything following this
include_directories(system)
include_directories(system/tasks)

####################################
# Grappa subdirectories
add_subdirectory(system)
add_subdirectory(applications)
add_subdirectory(bin)
add_subdirectory(scratch)
add_subdirectory(doc)
add_subdirectory(util)
