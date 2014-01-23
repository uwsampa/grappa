
set(symlinked_scripts
  distcc_make
  distcc_ninja
  grappa_srun
  launch_distcc.sh
  srun_epilog.sh
)

set(configured_scripts
  srun_prolog.rb
)

foreach(file ${symlinked_scripts})
  get_filename_component(name ${file} NAME)
  file(RELATIVE_PATH relative_file ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${file})
  execute_process(COMMAND
    ln -sf ${relative_file} ${CMAKE_CURRENT_BINARY_DIR}/${name}
  )
endforeach()

set(GRAPPA_LD_DIRS "")
if(VAMPIR_ROOT)
  set(GRAPPA_LD_DIRS "${GRAPPA_LD_DIRS}:${VAMPIR_ROOT}/lib")
endif()
if(THIRD_PARTY_ROOT)
  set(GRAPPA_LD_DIRS "${GRAPPA_LD_DIRS}:${THIRD_PARTY_ROOT}/lib")
else()
  set(GRAPPA_LD_DIRS "${GRAPPA_LD_DIRS}:${EXTERN_BUILD_DIR}/lib")  
endif()

foreach(f ${configured_scripts})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${f}.in ${CMAKE_CURRENT_BINARY_DIR}/${f} @ONLY)
endforeach()
