# - Test CXX compiler for a flag
# Check if the CXX compiler accepts a flag
#
#  Macro CHECK_CXX_ACCEPTS_FLAG(FLAGS VARIABLE) -
#     checks if the function exists
#  FLAGS - the flags to try
#  VARIABLE - variable to store the result
#

MACRO(CHECK_CXX_ACCEPTS_FLAG FLAGS  VARIABLE)
  IF(NOT DEFINED ${VARIABLE})
    MESSAGE(STATUS "Checking to see if CXX compiler accepts flag ${FLAGS}")
    TRY_COMPILE(${VARIABLE}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_ROOT}/Modules/DummyCXXFile.cxx
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${FLAGS}
      OUTPUT_VARIABLE OUTPUT) 
    IF(${VARIABLE})
      MESSAGE(STATUS "Checking to see if CXX compiler accepts flag ${FLAGS} - yes")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Determining if the CXX compiler accepts the flag ${FLAGS} passed with "
        "the following output:\n${OUTPUT}\n\n")
    ELSE(${VARIABLE})
      MESSAGE(STATUS "Checking to see if CXX compiler accepts flag ${FLAGS} - no")
      FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Determining if the CXX compiler accepts the flag ${FLAGS} failed with "
        "the following output:\n${OUTPUT}\n\n")
    ENDIF(${VARIABLE})
  ENDIF(NOT DEFINED ${VARIABLE})
ENDMACRO(CHECK_CXX_ACCEPTS_FLAG)
