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

set(MPIRUN srun)

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
