message("-- LLVM_ROOT: ${LLVM_ROOT}")
set( LLVM_DIR "${LLVM_ROOT}/share/llvm/cmake" )
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${LLVM_DIR} )
find_package(LLVM)
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
message("-- LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message("-- LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")

# # A bit of a sanity check:
# if(NOT EXISTS "${LLVM_ROOT}/include/llvm")
#   message(FATAL_ERROR "LLVM_ROOT (${LLVM_ROOT}) is not a valid LLVM install")
# endif()
# # We incorporate the CMake features provided by LLVM:
# # set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_INSTALL_DIR}/share/llvm/cmake")
# include(AddLLVM)
# include(LLVMConfig)
# # Now set the header and library paths:
# include_directories(${LLVM_INCLUDE_DIRS})
# link_directories(${LLVM_LIBRARY_DIRS})
# add_definitions(${LLVM_DEFINITIONS})

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )
file(GLOB compiler_includes "${CMAKE_CURRENT_SOURCE_DIR}/include/*")

# add_subdirectory( GrappaGen )
set(LLVM_SRCS
  GrappaGen/Passes.h
  GrappaGen/GrappaGen.cpp
  GrappaGen/PrePass.cpp
  GrappaGen/ExtractorPass.cpp
  GrappaGen/ProvenanceProp.cpp
  GrappaGen/DelegateExtractor.hpp
  GrappaGen/DelegateExtractor.cpp
)
add_llvm_loadable_module( LLVMGrappaGen ${LLVM_SRCS})
set_target_properties( LLVMGrappaGen PROPERTIES COMPILE_FLAGS "-fno-rtti -O0" )
set_property(TARGET LLVMGrappaGen PROPERTY FOLDER "Compiler")


get_property( GRAPPA_GEN_LOCATION TARGET LLVMGrappaGen  PROPERTY LOCATION )

add_subdirectory( bin )

add_custom_target( compiler
  DEPENDS LLVMGrappaGen
  SOURCES ${compiler_includes}
)
set_property(TARGET compiler PROPERTY FOLDER "Compiler")
add_dependencies(compiler Grappa)

# macro that runs the Grappa optimization pass when compiling
macro(add_grappaclang_exe target exe)
  add_grappa_exe(${target} "${exe}" ${ARGN})
  add_dependencies(${target} LLVMGrappaGen)
  set_source_files_properties(${ARGN} PROPERTIES OBJECT_DEPENDS compiler)
  set_target_properties(${target} PROPERTIES
    COMPILE_FLAGS       "-Xclang -load -Xclang ${GRAPPA_GEN_LOCATION}"
    COMPILE_DEFINITIONS "__GRAPPA_CLANG__"
  )
endmacro()

add_subdirectory( test )
add_subdirectory( apps )
