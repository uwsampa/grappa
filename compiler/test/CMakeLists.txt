
file(GLOB tests "*.cpp")

macro(add_grappaclang_test file flags suffix)
  get_filename_component(base ${file} NAME_WE)
  
  set(target "cl.test.${base}.${suffix}")
  set(exe "${base}.${suffix}.exe")
  
  add_grappaclang_exe(${target} ${exe} ${file})
  
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flags}")
  
  add_test(${target}.build "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target ${target})
  add_test(NAME ${target}.test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND "${CMAKE_BINARY_DIR}/bin/grappa_srun" -n1 -p2 "--no-freeze-on-error" "--no-backtrace" -- "${CMAKE_CURRENT_BINARY_DIR}/${exe}"
  )
  set_tests_properties(${target}.test PROPERTIES DEPENDS ${target}.build)
  
endmacro()

foreach(file ${tests})
  add_grappaclang_test(${file} "-O1" putget)
  add_grappaclang_test(${file} "-O3" putget.o3)
  add_grappaclang_test(${file} "-mllvm -grappa-extractor -O1" ext)
  add_grappaclang_test(${file} "-mllvm -grappa-extractor -O3" ext.o3)
endforeach()
