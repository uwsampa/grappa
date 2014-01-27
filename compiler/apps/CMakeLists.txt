
file(GLOB miniapp_srcs "*.cpp")

foreach(file ${miniapp_srcs})
  get_filename_component(base ${file} NAME_WE)
  set(target_name "compiler-app-${base}")
  add_grappaclang_exe(${target_name} "${base}.exe" ${file})
  set_property(TARGET ${target_name} PROPERTY FOLDER "Compiler")
endforeach()
