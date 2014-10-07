# don't clean custom targets in this directory
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)

###########################################
# External projects
###########################################
set(EXTERN_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")
set(EXTERN_STAMP_DIR "${EXTERN_BUILD_DIR}/stamps")

message("-- Using {${BASE_C_COMPILER},${BASE_CXX_COMPILER}} to build third-party dependencies (if needed).")

add_subdirectory(graph500-generator)

if(NOT Boost_FOUND)

  # check if we have built boost in third-party dir
  find_library(THIRD_PARTY_BOOST "boost_filesystem" PATHS "${THIRD_PARTY_ROOT}/lib" NO_DEFAULT_PATH)
  
  if( NOT THIRD_PARTY_BOOST )
  
    # Download & build Boost
    # TODO: detect which stdlib is being used and configure Boost to use it (i.e. on Mac, need to compile Boost with: toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" if using a version of Clang built with 'libc++')
    ExternalProject_Add(third-party-boost
      SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/boost
      PREFIX             ${EXTERN_BUILD_DIR}/boost
      STAMP_DIR          ${EXTERN_STAMP_DIR}/boost
      INSTALL_DIR        ${THIRD_PARTY_ROOT}
      URL "http://sourceforge.net/projects/boost/files/boost/1.54.0/boost_1_54_0.tar.bz2"
      # URL_MD5 dbc07ab0254df3dda6300fd737b3f264 
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
    set(GRAPPA_LIBS
      ${GRAPPA_LIBS}
      boost_system
      boost_filesystem
      boost_unit_test_framework
    PARENT_SCOPE)
    list(APPEND tool_list third-party-boost)
  else()
    message("     boost:  ${THIRD_PARTY_BOOST}")
  endif()
  
endif()

# SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/gflags
# URL_HASH           SHA1=bfa0b399e03147b20cafcae62b59ee9163309821 # not supported by older versions of CMake

if(APPLE)
  # set(APPLE_CXXFLAGS CXXFLAGS=-stdlib=libstdc++)
endif()

find_library(THIRD_PARTY_GFLAGS "gflags" "${THIRD_PARTY_ROOT}/lib" NO_CMAKE_SYSTEM_PATH)
if( NOT THIRD_PARTY_GFLAGS )
  message("     gflags: <building>")
  ExternalProject_Add( third-party-gflags
    URL                "http://gflags.googlecode.com/files/gflags-2.0-no-svn-files.tar.gz"
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
  message("     gflags: ${THIRD_PARTY_GFLAGS}")
endif()

find_library(THIRD_PARTY_GLOG "glog" "${THIRD_PARTY_ROOT}/lib" NO_CMAKE_SYSTEM_PATH)
if( NOT THIRD_PARTY_GLOG )
  message("     glog:   <building>")
  # (can't use release version because we customized it)
  # TODO: find out if latest version supports what we customized it for so we don't have to distribute it
  # URL                http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz
  # URL_HASH           SHA1=ed40c26ecffc5ad47c618684415799ebaaa30d65
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
  message("     glog:   ${THIRD_PARTY_GLOG}")
endif()

# required
# optional
if(TRACING)
  
  find_library(THIRD_PARTY_GPERFTOOLS "profiler" "${THIRD_PARTY_ROOT}/lib")

if( NOT THIRD_PARTY_GPERFTOOLS )
    message("     gperftools: <building>")
    ExternalProject_Add( third-party-gperftools
      URL                http://gperftools.googlecode.com/files/gperftools-2.1.tar.gz
      # URL_HASH           SHA1=b799b99d9f021988bbc931db1c21b2f94826d4f0
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
