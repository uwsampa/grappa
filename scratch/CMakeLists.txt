
file(GLOB SCRATCHES
  "*.cpp"
)

# make separate build targets for each BFS variant
foreach(file ${SCRATCHES})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_application(${base}.exe ${file})
endforeach()
