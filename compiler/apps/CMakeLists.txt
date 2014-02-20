
file(GLOB miniapp_srcs "*.cpp")

foreach(file ${miniapp_srcs})
  get_filename_component(base ${file} NAME_WE)
  set(target "cl.app.${base}")
  add_grappaclang_exe(${target} "${base}.exe" ${file})
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " -mllvm -grappa-extractor")
endforeach()
