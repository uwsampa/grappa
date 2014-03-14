
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
  set_property(TARGET ${target} APPEND_STRING PROPERTY COMPILE_FLAGS " -O3 ${flags}")
  
  list(APPEND apps ${fbase})
  list(APPEND apps.${fbase} ${target})
endmacro()

foreach(file ${miniapp_srcs})
  add_app(grappaclang ${file} "-mllvm -grappa-extractor" ext)
  add_app(grappaclang ${file} "-mllvm -grappa-extractor -mllvm -disable-async" ext.noasync)
  add_app(grappaclang ${file} "" putget)
endforeach()

add_app(default "bfs.cpp" "" hand)

############
# Gups
add_app(default     "gups.cpp" "" hand)
# measure benefit of async:
add_app(default     "gups.cpp" "-DBLOCKING=1" blocking.hand)
# verify overhead of void interface
add_app(grappaclang "gups.cpp" "-DVOID_INTERFACE=1" void)
# multihop:
add_app(grappaclang "gups.cpp" "-mllvm -grappa-extractor -DMULTIHOP=1" hop.ext)
add_app(default     "gups.cpp" "-DMULTIHOP=1 -DBLOCKING=1" hop.base)
add_app(default     "gups.cpp" "-DMULTIHOP=1" hop.hand)

########################
# Connected components
add_app(default "cc.cpp" "" hand)


list(REMOVE_DUPLICATES apps)
foreach(app ${apps})
  add_custom_target(cl.app.${app}._ DEPENDS ${apps.${app}})
endforeach()
