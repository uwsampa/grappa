
file(GLOB tests "*.cpp")

macro(add_grappaclang_test file flags suffix)
  get_filename_component(base ${file} NAME_WE)
  
  set(target "cl.test.${base}.${suffix}")
  set(exe "${base}.${suffix}.exe")
  
  add_grappaclang_exe(${target} ${exe} ${file})
  
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flags}")
  
  add_test(NAME ${target}.test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND "${CMAKE_BINARY_DIR}/bin/grappa_srun" -n1 -p2 -- "${CMAKE_CURRENT_BINARY_DIR}/${exe}"
  )
  add_test(${target}.build "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${target})
  set_tests_properties(${target}.test PROPERTIES DEPENDS ${target}.build)
  
  # add to 'all' lists
  set(compiler.all.${suffix}.compile ${compiler.all.${suffix}.compile} ${target} PARENT_SCOPE)
  set(compiler.all.compile ${compiler.all.compile} ${target} PARENT_SCOPE)
  set(compiler.all.check   ${compiler.all.check}   ${target}.check PARENT_SCOPE)
endmacro()

foreach(file ${tests})
  add_grappaclang_test(${file} "" n)
  add_grappaclang_test(${file} "-mllvm -grappa-extractor" ext)
endforeach()
