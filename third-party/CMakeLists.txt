# don't clean custom targets in this directory
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)

###########################################
# External projects
###########################################
set(EXTERN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")
set(EXTERN_STAMP_DIR "${EXTERN_BUILD_DIR}/stamps")

SET(THIRD_PARTY_TARFILE "" CACHE FILEPATH "Instead of downloading third-party dependences from the web, extract them from the specified tar file (available from Grappa website).")
SET(CMAKE_THIRD_PARTY_TARFILE "${THIRD_PARTY_TARFILE}" CACHE INTERNAL "Path to tarfile for third-party dependences." FORCE)
SET(THIRD_PARTY_DOWNLOADS "${EXTERN_BUILD_DIR}/third-party-downloads" CACHE INTERNAL "Path to untarred third-party dependence sources." FORCE)

if("${CMAKE_THIRD_PARTY_TARFILE}" STREQUAL "")
  message("-- Downloading third-party dependences from the web.")
else()
  message("-- Untarring third-party dependence sources from ${CMAKE_THIRD_PARTY_TARFILE} into ${THIRD_PARTY_DOWNLOADS}.")
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${THIRD_PARTY_DOWNLOADS}")
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_THIRD_PARTY_TARFILE}"
                  WORKING_DIRECTORY "${THIRD_PARTY_DOWNLOADS}")
endif()

message("-- Using {${BASE_C_COMPILER},${BASE_CXX_COMPILER}} to build third-party dependencies (if needed).")

######################################################################
# Graph500 source for Kronecker generator
######################################################################
add_subdirectory(graph500-generator)
list(APPEND tool_list graph500-generator)

# if we found a pre-installed version, add its paths to our build environment
if(NOT Boost_FOUND)
  if(NOT "${CMAKE_THIRD_PARTY_TARFILE}" STREQUAL "")
    set(URL "${THIRD_PARTY_DOWNLOADS}/boost_1_54_0.tar.bz2")
  else()
    set(URL "http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.bz2")
  endif()

  message("     boost: <building>")
  # Download & build Boost
  # TODO: detect which stdlib is being used and configure Boost to use it (i.e. on Mac, need to compile Boost with: toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" if using a version of Clang built with 'libc++')
  ExternalProject_Add(third-party-boost
    URL                ${URL}
    ##URL_HASH           SHA1=230782c7219882d0fab5f1effbe86edb85238bf4  # ignored to support older cmake
    SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/boost
    PREFIX             ${EXTERN_BUILD_DIR}/boost
    STAMP_DIR          ${EXTERN_STAMP_DIR}/boost
    INSTALL_DIR        ${THIRD_PARTY_ROOT}
    BUILD_IN_SOURCE 1
    CONFIGURE_COMMAND
    ./bootstrap.sh 
    --with-libraries=filesystem
    --with-libraries=system
    --with-libraries=test
    --with-libraries=random
    --prefix=<INSTALL_DIR>
    BUILD_COMMAND
    ./b2 install --link=static --variant=release --threading=single --runtime-link=static
    INSTALL_COMMAND ""
    )
  list(APPEND tool_list third-party-boost)
else()
  add_custom_target(third-party-boost)
  message("     boost: ${Boost_INCLUDE_DIRS} and ${Boost_LIBRARY_DIRS}")
endif()

if(APPLE)
  # set(APPLE_CXXFLAGS CXXFLAGS=-stdlib=libstdc++)
endif()

if( NOT GFLAGS_FOUND )
  if(NOT "${CMAKE_THIRD_PARTY_TARFILE}" STREQUAL "")
      set(URL "${THIRD_PARTY_DOWNLOADS}/gflags-2.0-no-svn-files.tar.gz")
  else()
      set(URL "https://github.com/gflags/gflags/archive/v2.0.tar.gz")
  endif()
  message("    gflags: <building>")
  ExternalProject_Add( third-party-gflags
    URL                ${URL}
    ##URL_HASH           SHA1=bfa0b399e03147b20cafcae62b59ee9163309821  # ignored to support older cmake
    SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gflags-2.0
    PREFIX             ${EXTERN_BUILD_DIR}/gflags
    STAMP_DIR          ${EXTERN_STAMP_DIR}/gflags
    BINARY_DIR         ${EXTERN_BUILD_DIR}/gflags
    INSTALL_DIR        ${THIRD_PARTY_ROOT}
    CONFIGURE_COMMAND
      CC=${BASE_C_COMPILER} CXX=${BASE_CXX_COMPILER} CFLAGS=-g ${APPLE_CXXFLAGS} <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND      make -j 4
    INSTALL_COMMAND    make install
  )
  list(APPEND tool_list third-party-gflags)
else()
  add_custom_target(third-party-gflags)
  message("    gflags: ${GFLAGS_FOUND}")
endif()

if( NOT GLOG_FOUND )
  message("      glog: <building>")
  # (can't use release version because we customized it)
  # TODO: find out if latest version supports what we customized it for so we don't have to distribute it
  ExternalProject_Add( third-party-glog
    SOURCE_DIR         ${CMAKE_CURRENT_SOURCE_DIR}/google-glog
    PREFIX             ${EXTERN_BUILD_DIR}/glog
    STAMP_DIR          ${EXTERN_STAMP_DIR}/glog
    BINARY_DIR         ${EXTERN_BUILD_DIR}/glog
    INSTALL_DIR        ${THIRD_PARTY_ROOT}
    CONFIGURE_COMMAND
      CC=${BASE_C_COMPILER} CXX=${BASE_CXX_COMPILER} ${APPLE_CXXFLAGS} CFLAGS=-g CXXFLAGS=-g CFLAGS=-I${THIRD_PARTY_ROOT}/include CXXFLAGS=-I${THIRD_PARTY_ROOT}/include LDFLAGS=-L${THIRD_PARTY_ROOT}/lib <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
    BUILD_COMMAND      make -j 4
    INSTALL_COMMAND    make install
    DEPENDS third-party-gflags
  )
  list(APPEND tool_list third-party-glog)
else()
  message("      glog: ${GLOG_FOUND}")
endif()



if(TRACING OR PROFILING)

  # TODO: Add find_package() call so we don't build this if we don't have to!  
  find_library(THIRD_PARTY_GPERFTOOLS "profiler" "${THIRD_PARTY_ROOT}/lib")
  if( NOT THIRD_PARTY_GPERFTOOLS )
  
    if(NOT "${CMAKE_THIRD_PARTY_TARFILE}" STREQUAL "")
        set(URL "${THIRD_PARTY_DOWNLOADS}/gperftools-2.1.tar.gz")
    else()
        set(URL "http://gperftools.googlecode.com/files/gperftools-2.1.tar.gz")
    endif()
    message("     gperftools: <building>")
    ExternalProject_Add( third-party-gperftools
      URL                ${URL}
      ##URL_HASH           SHA1=b799b99d9f021988bbc931db1c21b2f94826d4f0   # ignored to support older cmake
      SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gperftools-2.1
      PREFIX             ${EXTERN_BUILD_DIR}/gperftools
      STAMP_DIR          ${EXTERN_STAMP_DIR}/gperftools
      BINARY_DIR         ${EXTERN_BUILD_DIR}/gperftools
      INSTALL_DIR        ${THIRD_PARTY_ROOT}
      CONFIGURE_COMMAND  
        CC=${BASE_C_COMPILER} CXX=${BASE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-frame-pointers
      BUILD_COMMAND      make -j 4
      INSTALL_COMMAND    make install
    )
    list(APPEND tool_list third-party-gperftools)
  else()
    message("     gperftools: ${THIRD_PARTY_GPERFTOOLS}")
  endif()
  
endif()

# set(third-party-targets  third-party-gasnet third-party-gflags third-party-glog third-party-gperftools )
add_custom_target(all-third-party DEPENDS ${tool_list} )
set_property(TARGET all-third-party PROPERTY FOLDER "Third Party")

foreach(tool_target ${tool_list})
  set_property(TARGET ${tool_target} PROPERTY FOLDER "Third Party")
endforeach()

add_custom_target(clean-third-party
  rm -rf ${THIRD_PARTY_ROOT}
)
set_property(TARGET clean-third-party PROPERTY FOLDER "Third Party")

file(COPY bashflags DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#
# installation
#

install(DIRECTORY ${THIRD_PARTY_ROOT}/bin/     DESTINATION "bin")
install(DIRECTORY ${THIRD_PARTY_ROOT}/include/ DESTINATION "include")
install(DIRECTORY ${THIRD_PARTY_ROOT}/lib/     DESTINATION "lib")
install(DIRECTORY ${THIRD_PARTY_ROOT}/share/   DESTINATION "share")
