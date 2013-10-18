# don't clean custom targets in this directory
SET_DIRECTORY_PROPERTIES(PROPERTIES CLEAN_NO_CUSTOM 1)

###########################################
# External projects
###########################################
set(EXTERN_BUILD_DIR "${GRAPPA_DEPENDENCY_DIR}/build")
set(EXTERN_STAMP_DIR "${EXTERN_BUILD_DIR}/stamps")

ExternalProject_Add( extern_gasnet
  PREFIX             ${EXTERN_BUILD_DIR}/gasnet
  STAMP_DIR          ${EXTERN_STAMP_DIR}/gasnet
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gasnet
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/GASNet-1.18.2
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-segment-everything --disable-par --disable-parsync
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/gflags
# URL_HASH           SHA1=bfa0b399e03147b20cafcae62b59ee9163309821 # not supported by older versions of CMake
ExternalProject_Add( extern_gflags
  URL                "http://gflags.googlecode.com/files/gflags-2.0-no-svn-files.tar.gz"
  SOURCE_DIR         ${EXTERN_BUILD_DIR}/src/gflags-2.0
  PREFIX             ${EXTERN_BUILD_DIR}/gflags
  STAMP_DIR          ${EXTERN_STAMP_DIR}/gflags
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gflags
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# (can't use release version because we customized it)
# TODO: find out if latest version supports what we customized it for so we don't have to distribute it
# URL                http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz
# URL_HASH           SHA1=ed40c26ecffc5ad47c618684415799ebaaa30d65
ExternalProject_Add( extern_glog
  SOURCE_DIR         ${CMAKE_SOURCE_DIR}/tools/google-glog
  PREFIX             ${EXTERN_BUILD_DIR}/glog
  STAMP_DIR          ${EXTERN_STAMP_DIR}/glog
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
  STAMP_DIR          ${EXTERN_STAMP_DIR}/gperftools
  BINARY_DIR         ${EXTERN_BUILD_DIR}/gperftools
  INSTALL_DIR        ${GRAPPA_DEPENDENCY_DIR}
  CONFIGURE_COMMAND  
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} CFLAGS=-g <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --enable-frame-pointers
  BUILD_COMMAND      make -j
  INSTALL_COMMAND    make install
)

# required
set(externs extern_gasnet extern_gflags extern_glog)
# optional
if(TRACING)
  # TODO: add VampirTrace here, too
  set(externs ${externs} extern_gperftools)
endif()

# set(extern_targets  extern_gasnet extern_gflags extern_glog extern_gperftools )
add_custom_target(all_externs DEPENDS ${externs} )

add_custom_target(clean-extern
  rm -rf ${GRAPPA_DEPENDENCY_DIR}
)
