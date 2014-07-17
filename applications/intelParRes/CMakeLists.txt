
file(GLOB PARRES
  "*.cpp"
  "*/*.cpp"
)

# make separate build targets for each BFS variant
foreach(file ${PARRES})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(parres-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications")      
endforeach()
