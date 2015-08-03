# file(COPY . DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB scripts "*")
foreach(file ${scripts})
  get_filename_component(name ${file} NAME)
  file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_BINARY_DIR} ${file})
  execute_process(COMMAND
    ln -sf ${relative_file} ${CMAKE_CURRENT_BINARY_DIR}/${name}
  )
endforeach()

install(PROGRAMS settings.sh DESTINATION "bin")
