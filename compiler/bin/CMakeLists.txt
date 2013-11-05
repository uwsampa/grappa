# file(COPY . DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

get_property(
  GRAPPA_GEN_FILE 
  TARGET LLVMGrappaGen
  PROPERTY LOCATION )

add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/grappaclang"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/grappaclang.in" LLVMGrappaGen
  COMMAND ""
)

# configure_file(
#   "${CMAKE_CURRENT_SOURCE_DIR}/grappaclang.in"
#   "${CMAKE_CURRENT_BINARY_DIR}/grappaclang"
#   @ONLY )
