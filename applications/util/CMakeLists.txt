
file(GLOB UTILS
  "*/*.cpp"
)

# make separate build targets for each utility
foreach(file ${UTILS})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(util-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications")      
endforeach()
