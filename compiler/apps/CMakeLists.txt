
file(GLOB miniapp_srcs "*.cpp")

macro(add_app compiler file flags suffix)
  get_filename_component(fbase ${file} NAME_WE)
  set(base ${fbase})
  
  set(target "cl.app.${base}")
  
  if(NOT ${suffix} STREQUAL "")
    set(target "${target}.${suffix}")
    set(base "${base}.${suffix}")
  endif()
  
  if (${compiler} STREQUAL "grappaclang")
    add_grappaclang_exe(${target} "${base}.exe" ${file})
  else()
    add_grappa_exe(${target} "${base}.exe" ${file})
  endif()
  
  set_property(TARGET ${target} PROPERTY FOLDER "Compiler")
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " ${flags}")
  
  list(APPEND apps ${fbase})
  list(APPEND apps.${fbase} ${target})
endmacro()

foreach(file ${miniapp_srcs})
  add_app(grappaclang ${file} "-mllvm -grappa-extractor -O3" ext)
  add_app(grappaclang ${file} "-mllvm -grappa-extractor -mllvm -disable-async -O3" ext.noasync)
  add_app(grappaclang ${file} "-O3" putget)
endforeach()

add_app(default "bfs.cpp" "" hand)

add_app(default "gups.cpp" "" hand)
add_app(default "gups.cpp" "-DBLOCKING=1" blocking.hand)
add_app(grappaclang "gups.cpp" "-DVOID_INTERFACE=1" void)

list(REMOVE_DUPLICATES apps)
foreach(app ${apps})
  add_custom_target(cl.app.${app}._ DEPENDS ${apps.${app}})
endforeach()
