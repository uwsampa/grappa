
file(GLOB DEMOS
  "*/*.cpp"
)

# make separate build targets for each BFS variant
foreach(file ${DEMOS})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(demo-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Applications")      
endforeach()
