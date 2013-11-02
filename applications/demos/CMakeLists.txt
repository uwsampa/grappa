
file(GLOB DEMOS
  "*/*.cpp"
)

# make separate build targets for each BFS variant
foreach(file ${DEMOS})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_application(demo-${base}.exe ${file})
endforeach()
