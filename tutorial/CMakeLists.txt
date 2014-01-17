file(GLOB TUTORIAL_SOURCES
  "*.cpp"
)

foreach(file ${TUTORIAL_SOURCES})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(tutorial-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Tutorial")
endforeach()
