project(Grappa)

# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

site_name(MACHINENAME)

# global C++ flags
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -Winline -Wno-inline -mno-red-zone")
# TODO: use -stdlib=libc++ too?

# TODO: verify that this is not a problem and remove
# list(APPEND CMAKE_CXX_FLAGS "-mno-red-zone")

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

# set runpath, too
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--enable-new-dtags")

set(RUN_CMD srun)
set(RUN_FLAGS "")
list(APPEND RUN_FLAGS
  --partition=grappa
  --nodes=2
  --ntasks-per-node=1
  --resv-ports
  --cpu_bind=verbose,rank
  --exclusive
  --label
  --kill-on-bad-exit
)

set(GRAPPA_ENV "")

set(GASNET_SETTINGS
  GASNET_BACKTRACE=1
  GASNET_FREEZE_SIGNAL=SIGUSR1
  GASNET_FREEZE_ON_ERROR=1
  GASNET_FREEZE=0
  GASNET_NETWORKDEPTH_PP=96
  GASNET_NETWORKDEPTH_TOTAL=1024
  GASNET_AMCREDITS_PP=48
  GASNET_PHYSMEM_MAX=1024M
)
list(APPEND GRAPPA_ENV ${GASNET_SETTINGS})

list(APPEND GRAPPA_ENV
  GLOG_logtostderr=1
  GLOG_v=1
)

add_definitions("-DSHMMAX=12884901888")

if( GOOGLE_PROFILER )
  list(APPEND ENV_VARIABLES "CPUPROFILE_FREQUENCY=100")
  list(APPEND CMAKE_CXX_FLAGS -fno-omit-frame-pointer)
  add_definitions( -DGOOGLE_PROFILER )
endif()

set(GRAPPA_LIBS)

get_filename_component(CC_PATH ${CMAKE_C_COMPILER} PATH)
link_directories("${CC_PATH}/../lib64")
list(APPEND GRAPPA_LIBS pthread rt)

# pre-compiled external dependencies
# TODO: make CMake build these (see GraphLab's CMake)
set(GRAPPA_DEPENDENCY_DIR "${CMAKE_SOURCE_DIR}/tools/built_deps")
include_directories("${GRAPPA_DEPENDENCY_DIR}/include")
link_directories("${GRAPPA_DEPENDENCY_DIR}/lib")
list(APPEND GRAPPA_LIBS
  gflags
  glog
)

# boost is separate
set(BOOST_BASE "/sampa/share/gcc-4.7.2/src/boost_1_51_0")
include_directories("${BOOST_BASE}/boost")
link_directories("${BOOST_BASE}/stage/lib")
list(APPEND GRAPPA_LIBS
  boost_unit_test_framework
  boost_filesystem
  boost_system
)
# TODO: find_library(BOOST boost PATHS ${BOOST_BASE} )

# MPI (for booting only)
# TODO: discover with find_library()
include_directories("/usr/include/openmpi-x86_64")
link_directories("/usr/lib64/openmpi/lib")
list(APPEND GRAPPA_LIBS mpi_cxx mpi dl)

# put Grappa system directory on include path for everything following this
# include_directories("${CMAKE_CURRENT_SOURCE_DIR}/system")
include_directories(system)
include_directories(system/tasks)

add_subdirectory(system)
