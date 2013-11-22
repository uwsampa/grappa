# don't clean custom targets in this directory
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)

###########################################
# External projects
###########################################
set(EXTERN_BUILD_DIR "${TOOLS_INSTALL_DIR}/build")
set(EXTERN_STAMP_DIR "${EXTERN_BUILD_DIR}/stamps")

if(NOT Boost_FOUND)
  # Download & build Boost
  ExternalProject_Add(tool-boost
    SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/boost
    PREFIX             ${EXTERN_BUILD_DIR}/boost
    STAMP_DIR          ${EXTERN_STAMP_DIR}/boost
    INSTALL_DIR        ${TOOLS_INSTALL_DIR}
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
  set(Boost_LIBRARIES boost_filesystem boost_system boost_unit_test_framework )
  list(APPEND tool_list tool-boost)
endif()

ExternalProject_Add( tool-gasnet
  PREFIX             ${EXTERN_BUILD_DIR}/gasnet
  STAMP_DIR          ${EXTERN_STAMP_DIR}/gasnet
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gasnet
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/GASNet-1.18.2
  INSTALL_DIR        ${TOOLS_INSTALL_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-segment-everything --disable-par --disable-parsync
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/gflags
# URL_HASH           SHA1=bfa0b399e03147b20cafcae62b59ee9163309821 # not supported by older versions of CMake

if(APPLE)
  set(APPLE_CXXFLAGS CXXFLAGS=-stdlib=libstdc++)
endif()

ExternalProject_Add( tool-gflags
  URL                "http://gflags.googlecode.com/files/gflags-2.0-no-svn-files.tar.gz"
  SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gflags-2.0
  PREFIX             ${EXTERN_BUILD_DIR}/gflags
  STAMP_DIR          ${EXTERN_STAMP_DIR}/gflags
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gflags
  INSTALL_DIR        ${TOOLS_INSTALL_DIR}
  CONFIGURE_COMMAND
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g ${APPLE_CXXFLAGS} <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# (can't use release version because we customized it)
# TODO: find out if latest version supports what we customized it for so we don't have to distribute it
# URL                http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz
# URL_HASH           SHA1=ed40c26ecffc5ad47c618684415799ebaaa30d65
ExternalProject_Add( tool-glog
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/google-glog
  PREFIX             ${EXTERN_BUILD_DIR}/glog
  STAMP_DIR          ${EXTERN_STAMP_DIR}/glog
  BINARY_DIR         ${EXTERN_BUILD_DIR}/glog
  INSTALL_DIR        ${TOOLS_INSTALL_DIR}
  CONFIGURE_COMMAND
  CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g ${APPLE_CXXFLAGS} <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> CFLAGS=-I${TOOLS_INSTALL_DIR}/include LDFLAGS=-L${TOOLS_INSTALL_DIR}/lib
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
  DEPENDS tool-gflags
)

# required
# optional
if(TRACING)
  # TODO: add VampirTrace here, too
  
  ExternalProject_Add( tool-gperftools
    URL                http://gperftools.googlecode.com/files/gperftools-2.1.tar.gz
    # URL_HASH           SHA1=b799b99d9f021988bbc931db1c21b2f94826d4f0
    SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gperftools-2.1
    PREFIX             ${EXTERN_BUILD_DIR}/gperftools
    STAMP_DIR          ${EXTERN_STAMP_DIR}/gperftools
    BINARY_DIR         ${EXTERN_BUILD_DIR}/gperftools
    INSTALL_DIR        ${TOOLS_INSTALL_DIR}
    CONFIGURE_COMMAND  
      CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-frame-pointers
    BUILD_COMMAND      make -j
    INSTALL_COMMAND    make install
  )
  set_property(TARGET tool-gperftools PROPERTY FOLDER "Third Party")
  
  set(tool_list ${tool_list} tool-gperftools)
endif()

# set(tool-targets  tool-gasnet tool-gflags tool-glog tool-gperftools )
add_custom_target(all-tools DEPENDS ${tool_list} )
set_property(TARGET all-tools PROPERTY FOLDER "Third Party")

foreach(tool_target ${tool_list})
  set_property(TARGET ${tool_target} PROPERTY FOLDER "Third Party")
endforeach()

add_custom_target(clean-tools
  rm -rf ${TOOLS_INSTALL_DIR}
)
set_property(TARGET clean-tools PROPERTY FOLDER "Third Party")

