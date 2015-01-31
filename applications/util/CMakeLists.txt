
file(GLOB UTILS
  "*/*.cpp"
)

# make separate build targets for each utility
foreach(file ${UTILS})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(util-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications")      
endforeach()



# TODO: this should really just enabled with tracing

macro(add_vampir_exe target exe )
  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_NAME "${exe}")
  target_link_libraries(${target} 
    Grappa
    open-trace-format
    sqlite3
  )
endmacro(add_vampir_exe)

macro(add_vampir_application name)
  add_vampir_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications") # For organization in Xcode project
endmacro(add_vampir_application)

add_vampir_application(otf2sqlite.exe "otf2sqlite.cpp")
