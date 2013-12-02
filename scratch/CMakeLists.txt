# make separate build targets for each cpp file in scratch/ (must reconfigure after adding file)
# ex:
# > touch scratch/test.cpp
# > make rebuild_cache
# > make scratch-test
# > grappa_srun -- scratch/test.exe

file(GLOB SCRATCHES
  "*.cpp"
)

foreach(file ${SCRATCHES})
  get_filename_component(base ${file} NAME_WE)
  add_grappa_exe(scratch-${base} ${base}.exe ${file})
  set_property(TARGET ${name} PROPERTY FOLDER "Scratch")
endforeach()
