
file(GLOB miniapp_srcs "*.cpp")

macro(add_grappaclang_app file flags suffix)
  get_filename_component(base ${file} NAME_WE)
  
  set(target "cl.app.${base}")
  if(NOT ${suffix} STREQUAL "")
    set(target "${target}.${suffix}")
    set(base "${base}.${suffix}")
  endif()
  
  add_grappaclang_exe(${target} "${base}.exe" ${file})
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flags}")
endmacro()

macro(add_hand_app file flags suffix)
  get_filename_component(base ${file} NAME_WE)
  set(target cl.app.${base}.${suffix})
  add_grappa_exe(${target} ${base}.${suffix}.exe "${file}")
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flags}")
endmacro()

foreach(file ${miniapp_srcs})
  add_grappaclang_app(${file} "-mllvm -grappa-extractor -O3" ext)
  add_grappaclang_app(${file} "-O3" putget)
endforeach()

add_hand_app("hand/gups.cpp" "" hand)
add_hand_app("hand/gups.cpp" "-DBLOCKING" blocking.hand)

add_hand_app("hand/bfs.cpp" "" hand)
