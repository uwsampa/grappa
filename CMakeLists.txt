
# (2.8 has ExternalProject support; we've had problems with package detection for versions < 2.8.12)
cmake_minimum_required(VERSION 2.8.12)
include(ExternalProject)
include(util/grappa.cmake)

option(TRACING "Sample statistics with VTrace at regular intervals." OFF)
option(PROFILING "Sample profile with GPerftools at regular intervals." OFF)

SET(GRAPPA_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Prefix prepended to install directories")
SET(CMAKE_INSTALL_PREFIX "${GRAPPA_INSTALL_PREFIX}" CACHE INTERNAL "Prefix prepended to install directories" FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# if( NOT DEFINED ENV{CXX} )
#   message(FATAL_ERROR "you must set CC & CXX environment variables!")
# endif()

# set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.clang.3_4")

project(Grappa)

set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")

set(CMAKE_C_FLAGS_DEBUG "-O0 -DDEBUG -g -DGUARD_PAGES_ON_STACK")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DDEBUG -g -DGUARD_PAGES_ON_STACK")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
enable_testing()
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
site_name(MACHINENAME)

# Check GCC version
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7.2)
    message(FATAL_ERROR "When using GCC, the version must be at least 4.7.2. Found ${CMAKE_CXX_COMPILER_VERSION} instead.")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
    message(FATAL_ERROR "When using Clang, the version must be at least 3.4. Found ${CMAKE_CXX_COMPILER_VERSION} instead.")
  endif()
else()
  message(WARNING "You are using an unsupported compiler! Compilation has only been tested with Clang and GCC.")
endif()

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
  
  # list(APPEND GRAPPA_DYNAMIC_LIBS vt otf z papi dl m)
  
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


#
# External libraries and third-party tools
#
# GRAPPA_STATIC_LIBS is a list of full paths to .a files
# GRAPPA_DYNAMIC_LIBS is a list of full paths to .so files
# Include directories should be set explicitly here
# Link directories should not; they will be extracted from the full lib paths
#

######################################################################
# MPI
######################################################################
find_package(MPI REQUIRED)
if(VERBOSE)
  message("MPI_CXX_INCLUDE_PATH: ${MPI_CXX_INCLUDE_PATH}")
  message("MPI_CXX_COMPILE_FLAGS: ${MPI_CXX_COMPILE_FLAGS}")
  message("MPI_CXX_LINK_FLAGS: ${MPI_CXX_LINK_FLAGS}")
  message("MPI_CXX_LIBRARIES: ${MPI_CXX_LIBRARIES}")
endif()
include_directories(${MPI_INCLUDE_PATH})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_COMPILE_FLAGS}")
set(LINK_FLAGS "${LINK_FLAGS} ${MPI_CXX_LINK_FLAGS}")
list(APPEND GRAPPA_DYNAMIC_LIBS ${MPI_CXX_LIBRARIES})

######################################################################
# PMI: helpful for job launch on some systems with Slurm.
# MPI may already include it, but we ignore that here.
######################################################################
find_library(PMI_FOUND pmi)
if(PMI_FOUND)
  list(APPEND GRAPPA_DYNAMIC_LIBS ${PMI_FOUND})
endif()

######################################################################
# Pthreads
######################################################################
if(NOT APPLE)
  find_library(PTHREADS_FOUND pthread REQUIRED)
  list(APPEND GRAPPA_DYNAMIC_LIBS ${PTHREADS_FOUND})
endif()

######################################################################
# POSIX realtime extensions
######################################################################
if(NOT APPLE)
  find_library(RT_FOUND rt REQUIRED)
  list(APPEND GRAPPA_DYNAMIC_LIBS ${RT_FOUND})
endif()

######################################################################
# Google logging (Grappa-customized)
######################################################################
find_library(GLOG_FOUND libglog.a glog ${THIRD_PARTY_ROOT}/lib NO_CMAKE_SYSTEM_PATH)
if(VERBOSE)
  message("GLOG_FOUND: ${GLOG_FOUND}")
endif()
if(GLOG_FOUND)
  list(APPEND GRAPPA_STATIC_LIBS ${GLOG_FOUND})
else()
  # assume it will be built in third-party
  list(APPEND GRAPPA_STATIC_LIBS ${THIRD_PARTY_ROOT}/lib/libglog.a)
endif()
list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)

######################################################################
# Google flags
# in theory, it may come from third-party or from system directories,
# but for simplicity now just use the one in third-party.
######################################################################
find_library(GFLAGS_FOUND libgflags.a gflags ${THIRD_PARTY_ROOT}/lib NO_CMAKE_SYSTEM_PATH)
if(VERBOSE)
  message("GFLAGS_FOUND: ${GFLAGS_FOUND}")
endif()
if(GFLAGS_FOUND)
  list(APPEND GRAPPA_STATIC_LIBS ${GFLAGS_FOUND})
else()
  # assume it will be built in third-party
  list(APPEND GRAPPA_STATIC_LIBS ${THIRD_PARTY_ROOT}/lib/libgflags.a)
endif()

######################################################################
# Graph500 Kronecker generator (Grappa-customized)
######################################################################
find_library(GRAPH500_FOUND libgraph500-generator.a graph500-generator ${THIRD_PARTY_ROOT}/graph500-generator NO_CMAKE_SYSTEM_PATH)
if(VERBOSE)
  message("GRAPH500_FOUND: ${GRAPH500_FOUND}")
endif()
include_directories("${CMAKE_SOURCE_DIR}/third-party/graph500-generator")
if(GRAPH500_FOUND)
  list(APPEND GRAPPA_STATIC_LIBS ${GRAPH500_FOUND})
else()
  # assume it will be built in third-party
  list(APPEND GRAPPA_STATIC_LIBS ${CMAKE_BINARY_DIR}/third-party/graph500-generator/libgraph500-generator.a)
endif()

######################################################################
# Google profiler
######################################################################
if(GOOGLE_PROFILER)
  find_library(PROFILER_FOUND libprofiler.a profiler REQUIRED)
  list(APPEND GRAPPA_STATIC_LIBS ${PROFILER_FOUND})
endif()

######################################################################
# Boost (everything but unit tests)
######################################################################
set(BOOST_STATIC_OR_DYNAMIC_COMPONENTS system filesystem)

if(VERBOSE)
  set(Boost_DETAILED_FAILURE_MSG ON)
  set(Boost_DEBUG ON)
endif()

# try single-threaded first
set(Boost_USE_MULTITHREADED OFF)

# CMake 2.8 doesn't list most recent Boost versions, so add them ourselves
set(Boost_ADDITIONAL_VERSIONS "1.59.0" "1.59" "1.58.0" "1.58" "1.57.0" "1.57" "1.56.0" "1.56"
  "1.55.0" "1.55" "1.54.0" "1.54" "1.53.0" "1.53" "1.52.0" "1.52" "1.51.0" "1.51")

# first, search for things that we'd prefer to be static
if(VERBOSE)
  message("-- Searching for static Boost components: ${BOOST_STATIC_OR_DYNAMIC_COMPONENTS}")
endif()
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_COMPONENTS_TO_FIND ${BOOST_STATIC_OR_DYNAMIC_COMPONENTS})
grappa_search_for_boost()
unset(Boost_USE_STATIC_LIBS)
if(Boost_FOUND)
  set(Boost_STATIC_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
  set(Boost_STATIC_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}")
  set(Boost_STATIC_LIBRARIES "${Boost_LIBRARIES}")
else()
  # if that didn't work, re-search for dynamic versions
  message("-- Searching for dynamic Boost components: ${BOOST_STATIC_OR_DYNAMIC_COMPONENTS}")
  # we turned off Boost_USE_STATIC_LIBS above
  set(BOOST_COMPONENTS_TO_FIND ${BOOST_STATIC_OR_DYNAMIC_COMPONENTS})
  grappa_search_for_boost()
  if(Boost_FOUND)
    set(Boost_DYNAMIC_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
    set(Boost_DYNAMIC_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}")
    set(Boost_DYNAMIC_LIBRARIES "${Boost_LIBRARIES}")
  endif()
endif()
set(BOOST_STATIC_OR_DYNAMIC_FOUND ${Boost_FOUND})

# if we did find a pre-installed version, add its paths to our build environment
if(Boost_FOUND)
  message("-- Boost found:  ${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION} -- ${BOOST_PREFIX}")
  include_directories(${Boost_STATIC_INCLUDE_DIRS} ${Boost_DYNAMIC_INCLUDE_DIRS})
  list(APPEND GRAPPA_STATIC_LIBS ${Boost_STATIC_LIBRARIES})
  list(APPEND GRAPPA_DYNAMIC_LIBS ${Boost_DYNAMIC_LIBRARIES})
  get_filename_component(BOOST_PREFIX ${Boost_INCLUDE_DIR} PATH)
else()
  # we will deal with the rest of the not found case after searching
  # for the unit test libraries, since setting some boost variables
  # screws up the search.
  list(APPEND GRAPPA_STATIC_LIBS
    ${THIRD_PARTY_ROOT}/lib/libboost_system.a
    ${THIRD_PARTY_ROOT}/lib/libboost_filesystem.a)
endif()

######################################################################
# Boost (unit tests only)
######################################################################
set(BOOST_COMPONENTS_TO_FIND unit_test_framework)
grappa_search_for_boost()
if(Boost_FOUND)
  set(Boost_TEST_INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
  set(Boost_TEST_LIBRARY_DIRS "${Boost_LIBRARY_DIRS}")
  set(Boost_TEST_LIBRARIES "${Boost_LIBRARIES}")
else()
  # pick up from third-party
  set(Boost_TEST_LIBRARIES ${THIRD_PARTY_ROOT}/lib/libboost_unit_test_framework.so)
endif()
set(BOOST_TEST_FOUND ${Boost_FOUND})

# if we did find a pre-installed version, add its paths to our build environment
if(NOT (${BOOST_STATIC_OR_DYNAMIC_FOUND} AND ${BOOST_TEST_FOUND}))
  message("-- Boost not found. Building.")
  set(Boost_USE_MULTITHREADED OFF)
  # build is specified in third-party/CMakeLists.txt

  # now we need to set flags appropriately.
  # include path is fine since we already include THIRD_PARTY_ROOT
  # link path is fine since we already include THIRD_PARTY_ROOT
  # RPATH is fine since we already include THIRD_PARTY_ROOT
  set(Boost_INCLUDE_DIR ${THIRD_PARTY_ROOT}/include)
  set(Boost_LIBRARY_DIR ${THIRD_PARTY_ROOT}/lib)
  list(APPEND tool_list third-party-boost)
endif()


if(VERBOSE)
  message("BOOST_STATIC_COMPONENTS: ${BOOST_STATIC_COMPONENTS}")
  message("BOOST_STATIC_OR_DYNAMIC_COMPONENTS: ${BOOST_STATIC_OR_DYNAMIC_COMPONENTS}")
  message("Boost_STATIC_INCLUDE_DIRS: ${Boost_STATIC_INCLUDE_DIRS}")
  message("Boost_STATIC_LIBRARY_DIRS: ${Boost_STATIC_LIBRARY_DIRS}")
  message("Boost_STATIC_LIBRARIES: ${Boost_STATIC_LIBRARIES}")
  message("Boost_DYNAMIC_COMPONENTS: ${Boost_DYNAMIC_COMPONENTS}")
  message("Boost_DYNAMIC_INCLUDE_DIRS: ${Boost_DYNAMIC_INCLUDE_DIRS}")
  message("Boost_DYNAMIC_LIBRARY_DIRS: ${Boost_DYNAMIC_LIBRARY_DIRS}")
  message("Boost_DYNAMIC_LIBRARIES: ${Boost_DYNAMIC_LIBRARIES}")
  message("Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
  message("Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
  message("Boost_LIBRARIES: ${Boost_LIBRARIES}")
  message("Boost_TEST_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
  message("Boost_TEST_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
  message("Boost_TEST_LIBRARIES: ${Boost_LIBRARIES}")
  message("GRAPPA_STATIC_LIBS: ${GRAPPA_STATIC_LIBS}")
  message("GRAPPA_DYNAMIC_LIBS: ${GRAPPA_DYNAMIC_LIBS}")
endif()

####################################
# Load all third party libraries
add_subdirectory(third-party)



# RPATH
# not sure if this is useful any more, but what the heck.
foreach(lib ${Boost_LIBRARY_DIRS})
  set(CMAKE_INSTALL_RPATH "${lib}:${CMAKE_INSTALL_RPATH}")
endforeach()



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
  include_directories(${Boost_TEST_INCLUDE_DIRS})
  link_directories(${Boost_TEST_LIBRARY_DIRS})
  target_link_libraries(${name} ${Boost_TEST_LIBRARIES})
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

#############################################################################
# Generate GNU Make include file with appropriate definitions for
# standalone builds.
#############################################################################

# Figure out what type of build we're installing
string(TOUPPER "${CMAKE_BUILD_TYPE}" GMAKE_BUILD_TYPE)

# combine generic CXX flags with flags for current build type
set(GMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${GMAKE_BUILD_TYPE}}")
set(GMAKE_LINK_FLAGS "${LINK_FLAGS} ${LINK_FLAGS_${GMAKE_BUILD_TYPE}}")

# Get definitions and convert from CMake-formatted list to Gnu compiler argument list
get_directory_property(CMAKE_CXX_DEFINES DIRECTORY ${CMAKE_SOURCE_DIR}/applications/demos COMPILE_DEFINITIONS)
if(CMAKE_CXX_DEFINES)
  string(REPLACE ";" " -D" GMAKE_CXX_DEFINES ";${CMAKE_CXX_DEFINES}")
endif()

# get include dirs and convert from CMake-formatted list to compiler argument list
get_directory_property(CMAKE_CXX_INCLUDE_DIRS DIRECTORY ${CMAKE_SOURCE_DIR}/applications/demos INCLUDE_DIRECTORIES)
if(CMAKE_CXX_INCLUDE_DIRS)
  foreach(dir ${CMAKE_CXX_INCLUDE_DIRS})
    string(FIND "${dir}" "${CMAKE_SOURCE_DIR}" INCLUDE_IN_SOURCE_DIR) # -1 if not found, 0 if found
    string(FIND "${dir}" "${CMAKE_BINARY_DIR}" INCLUDE_IN_BUILD_DIR)  # -1 if not found, 0 if found
    string(FIND "${dir}" "${THIRD_PARTY_ROOT}" INCLUDE_IN_TPR_DIR)  # -1 if not found, 0 if found
    if(NOT INCLUDE_IN_SOURCE_DIR EQUAL 0 AND NOT INCLUDE_IN_BUILD_DIR EQUAL 0 AND NOT INCLUDE_IN_TPR_DIR EQUAL 0)
      list(APPEND GRAPPA_INCLUDE_DIRS_INSTALLED ${dir})
    endif()
  endforeach()
  
  string(REPLACE ";" " \\\n\t${CMAKE_INCLUDE_FLAG_CXX}" GMAKE_CXX_INCLUDE_DIRS ";${CMAKE_CXX_INCLUDE_DIRS}")
  string(REPLACE ";" " \\\n\t${CMAKE_INCLUDE_FLAG_CXX}" GMAKE_CXX_INCLUDE_DIRS_INSTALLED ";${GRAPPA_INCLUDE_DIRS_INSTALLED}")
endif()

# Get link dirs and convert from CMake-formatted list to compiler argument list (including system for libGrappa.a)
get_directory_property(CMAKE_CXX_LINK_DIRS DIRECTORY ${CMAKE_SOURCE_DIR}/applications/demos LINK_DIRECTORIES)
if(CMAKE_CXX_LINK_DIRS)
  string(REPLACE ";" " \\\n\t${CMAKE_LIBRARY_PATH_FLAG}" GMAKE_LINK_DIRS ";${CMAKE_BINARY_DIR}/system;${CMAKE_CXX_LINK_DIRS}")
endif()

####################################
# Build lists of library names and paths

# static libs
foreach(lib ${CMAKE_BINARY_DIR}/system/libGrappa.a ${GRAPPA_STATIC_LIBS})

  # add directory to search paths
  get_filename_component(dir ${lib} PATH)      # get directory for libs
  list(APPEND GRAPPA_LIB_PATHS ${dir})
  string(FIND "${dir}" "${CMAKE_BINARY_DIR}" STATIC_IN_BUILD_DIR)  # -1 if not found, 0 if found
  string(FIND "${dir}" "${THIRD_PARTY_ROOT}" STATIC_IN_TPR_DIR)    # -1 if not found, 0 if found
  if(NOT STATIC_IN_BUILD_DIR EQUAL 0 AND NOT STATIC_IN_TPR_DIR EQUAL 0)
    list(APPEND GRAPPA_LIB_PATHS_INSTALLED ${dir})
  endif()

  # add library to link list
  get_filename_component(name ${lib} NAME_WE)  # remove any extension
  string(SUBSTRING ${name} 3 -1 name)          # chop off "lib"
  list(APPEND GRAPPA_STATIC_LIB_NAMES ${name})

  # check for reasonable extension
  get_filename_component(ext ${lib} EXT)
  if(NOT ${ext} STREQUAL ".a")
    message(WARNING "Static library found didn't have .a extension: ${lib}")
  endif()
endforeach()

# dynamic libs
foreach(lib ${GRAPPA_DYNAMIC_LIBS})

  # add directory to search paths
  get_filename_component(dir ${lib} PATH)
  list(APPEND GRAPPA_LIB_PATHS ${dir})
  string(FIND "${dir}" "${CMAKE_BINARY_DIR}" DYNAMIC_IN_BUILD_DIR)  # -1 if not found, 0 if found
  string(FIND "${dir}" "${THIRD_PARTY_ROOT}" DYNAMIC_IN_TPR_DIR)    # -1 if not found, 0 if found
  if(NOT DYNAMIC_IN_BUILD_DIR EQUAL 0 AND NOT DYNAMIC_IN_TPR_DIR EQUAL 0)
    list(APPEND GRAPPA_LIB_PATHS_INSTALLED ${dir})
  endif()

  # add library to link list
  get_filename_component(name ${lib} NAME_WE)
  string(SUBSTRING ${name} 3 -1 name)
  list(APPEND GRAPPA_DYNAMIC_LIB_NAMES ${name})

  # check for reasonable extension
  get_filename_component(ext ${lib} EXT)
  if(NOT ${ext} STREQUAL ".so")
    message(WARNING "Dynamic library found didn't have .so extension: ${lib}")
  endif()
endforeach()

if(VERBOSE)
  message("GRAPPA_LIB_PATHS: ${GRAPPA_LIB_PATHS}")
  message("GRAPPA_STATIC_LIB_NAMES: ${GRAPPA_STATIC_LIB_NAMES}")
  message("GRAPPA_DYNAMIC_LIB_NAMES: ${GRAPPA_DYNAMIC_LIB_NAMES}")
endif()

# remove duplicates
list(REMOVE_DUPLICATES GRAPPA_LIB_PATHS)
string(REPLACE ";" " \\\n\t${CMAKE_LIBRARY_PATH_FLAG}" GMAKE_LIB_PATHS ";${GRAPPA_LIB_PATHS}")

list(REMOVE_DUPLICATES GRAPPA_LIB_PATHS_INSTALLED)
string(REPLACE ";" " \\\n\t${CMAKE_LIBRARY_PATH_FLAG}" GMAKE_LIB_PATHS_INSTALLED ";${GRAPPA_LIB_PATHS_INSTALLED}")

list(REMOVE_DUPLICATES GRAPPA_STATIC_LIB_NAMES)
string(REPLACE ";" " \\\n\t${CMAKE_LINK_LIBRARY_FLAG}" GMAKE_STATIC_LIBS ";${GRAPPA_STATIC_LIB_NAMES}")

list(REMOVE_DUPLICATES GRAPPA_DYNAMIC_LIB_NAMES)
string(REPLACE ";" " \\\n\t${CMAKE_LINK_LIBRARY_FLAG}" GMAKE_DYNAMIC_LIBS ";${GRAPPA_DYNAMIC_LIB_NAMES}")

# fill in prototype file and place in build directory
configure_file(util/grappa.mk util/grappa.mk)

#
# Installation
#
install(FILES ${CMAKE_BINARY_DIR}/util/grappa.mk         DESTINATION "share/Grappa/")

