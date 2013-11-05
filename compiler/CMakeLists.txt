message("@@ LLVM_PREFIX: ${LLVM_PREFIX}")
set( LLVM_DIR "${LLVM_PREFIX}/share/llvm/cmake" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LLVM_DIR} )
find_package(LLVM)
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
message("@@ LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
message("@@ LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message("@@ LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")

# # A bit of a sanity check:
# if(NOT EXISTS "${LLVM_PREFIX}/include/llvm")
#   message(FATAL_ERROR "LLVM_PREFIX (${LLVM_PREFIX}) is not a valid LLVM install")
# endif()
# # We incorporate the CMake features provided by LLVM:
# # set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_INSTALL_DIR}/share/llvm/cmake")
# include(AddLLVM)
# include(LLVMConfig)
# # Now set the header and library paths:
# include_directories(${LLVM_INCLUDE_DIRS})
# link_directories(${LLVM_LIBRARY_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

add_subdirectory(GrappaGen)
add_subdirectory(bin)

add_custom_target( compiler
  DEPENDS LLVMGrappaGen )
set_property(TARGET compiler PROPERTY FOLDER "Compiler")

