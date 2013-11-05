# file(COPY . DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(sysdir "${CMAKE_SOURCE_DIR}/system")

get_property( GRAPPA_GEN_LOCATION TARGET LLVMGrappaGen  PROPERTY LOCATION )
get_property( GRAPPA_LIBRARIES TARGET Grappa         PROPERTY LOCATION )

set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -L${TOOLS_INSTALL_DIR}/lib")
set(myrpath "${TOOLS_INSTALL_DIR}/lib")

foreach(lib ${MPI_CXX_LIBRARIES} ${Boost_LIBRARIES})
  set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} ${lib}")
  
  # get_filename_component( libdir ${lib} PATH )
  # set(myrpath "${myrpath}:${libdir}")
endforeach()
foreach(lib ${GRAPPA_LIBS})
  set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -l${lib}")
endforeach()

foreach(lib ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES})
  get_filename_component( libdir ${lib} PATH )
  list(APPEND rpath_dirs ${libdir})
endforeach()
list(REMOVE_DUPLICATES rpath_dirs)
foreach(dir ${rpath_dirs})
  set(myrpath "${myrpath}:${dir}")
endforeach()

set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -Wl,--enable-new-dtags -Wl,-rpath,${myrpath}")

get_property( defs  DIRECTORY "${sysdir}" PROPERTY COMPILE_DEFINITIONS )
foreach(d ${defs})
  set(GRAPPA_DEFINITIONS "${GRAPPA_DEFINITIONS} -D${d}")
endforeach()

get_property( include_dirs  DIRECTORY "${sysdir}" PROPERTY INCLUDE_DIRECTORIES )
foreach(d ${include_dirs})
  set(GRAPPA_INCLUDES "${GRAPPA_INCLUDES} -I${d}")
endforeach()

# add_custom_command(
#   OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/grappaclang"
#   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/grappaclang.in" LLVMGrappaGen
#   COMMAND ""
# )

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/grappaclang.in"
  "${CMAKE_CURRENT_BINARY_DIR}/grappaclang"
  @ONLY )
