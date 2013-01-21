project(Grappa)

# (2.8 has ExternalProject support)
cmake_minimum_required(VERSION 2.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# global C++ flags
list(APPEND CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

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

if( GOOGLE_PROFILER )
  list(APPEND ENV_VARIABLES "CPUPROFILE_FREQUENCY=100")
  list(APPEND CMAKE_CXX_FLAGS -fno-omit-frame-pointer)
  add_definitions( -DGOOGLE_PROFILER )
endif()

add_subdirectory(system)
