# file(COPY . DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(sysdir "${CMAKE_SOURCE_DIR}/system")

get_property( GRAPPA_LIBRARIES TARGET Grappa    PROPERTY LOCATION )
get_property( gen_loc TARGET graph500-generator PROPERTY LOCATION )

set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -L${THIRD_PARTY_ROOT}/lib ${gen_loc}")
set(myrpath "${THIRD_PARTY_ROOT}/lib")

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

if(NOT APPLE)
  set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -Wl,--enable-new-dtags")
endif()
set(GRAPPA_LIBRARIES "${GRAPPA_LIBRARIES} -Wl,-rpath,${myrpath}")

get_property( defs DIRECTORY "${sysdir}" PROPERTY COMPILE_DEFINITIONS )
foreach(d ${defs} "__GRAPPA_CLANG__")
  set(GRAPPA_DEFINITIONS "${GRAPPA_DEFINITIONS} -D${d}")
endforeach()

get_property( include_dirs  DIRECTORY "${sysdir}" PROPERTY INCLUDE_DIRECTORIES )
foreach(d ${include_dirs} "${CMAKE_CURRENT_SOURCE_DIR}/../include")
  set(GRAPPA_INCLUDES "${GRAPPA_INCLUDES} -I${d}")
endforeach()

foreach(file "grappaclang" "grappaclang-no-opt")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${file}.in"
    "${CMAKE_BINARY_DIR}/bin/${file}"
    @ONLY
  )
endforeach()


# symlink scripts into <BUILD_DIR>/bin (without extensions)
foreach(file  ir_debug.rb)
  get_filename_component(name ${file} NAME)
  get_filename_component(base ${file} NAME_WE)
  execute_process(COMMAND ln -sf ${CMAKE_CURRENT_SOURCE_DIR}/${name} ${CMAKE_BINARY_DIR}/bin/${base})
endforeach()