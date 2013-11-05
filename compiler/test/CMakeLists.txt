
file(GLOB tests "*.cpp")

foreach(file ${tests})
  get_filename_component(base ${file} NAME_WE)
  
  # set(out "${CMAKE_CURRENT_BINARY_DIR}/${base}.exe")
  # add_custom_command(
  #   OUTPUT "${out}"
  #   COMMAND "${CMAKE_CURRENT_BINARY_DIR}/../bin/grappaclang" ${file} -o "${out}"
  #   DEPENDS compiler Grappa ${compiler_includes}
  # )
  # set(target_name "compiler-test-${base}")
  # add_custom_target("${target_name}"
  #   DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${base}.exe"
  #   SOURCES "${file}"
  # )
  
  set(target_name "compiler-test-${base}")
  add_grappaclang_exe("${target_name}" "${base}.exe" ${file})
  set_property(TARGET "${target_name}" PROPERTY FOLDER "Compiler")
  
endforeach()
