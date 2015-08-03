
file(GLOB TEST_SOURCES
  "*_tests.cpp"
)

# file(GLOB SYSTEM_SOURCES
#   "*(!_tests).cpp"
#   "tasks/*.cpp"
# )
# file(GLOB SYSTEM_HEADERS
#   "*(_tests)!.hpp"
#   "tasks/*.hpp"
# )

set(SYSTEM_SOURCES
  Aggregator.cpp
  Allocator.cpp
  AsyncDelegate.cpp
  Barrier.cpp
  Cache.cpp
  ChunkAllocator.cpp
  CallbackMetric.cpp
  Collective.cpp
  Communicator.cpp
  Delegate.cpp
  FileIO.cpp
  FlatCombiner.cpp
  GlobalAllocator.cpp
  GlobalCompletionEvent.cpp
  GlobalHashMap.cpp
  GlobalHashSet.cpp
  GlobalMemory.cpp
  GlobalMemoryChunk.cpp
  GlobalVector.cpp
  Grappa.cpp
  HistogramMetric.cpp
  IncoherentAcquirer.cpp
  IncoherentReleaser.cpp
  LocaleSharedMemory.cpp
  MaxMetric.cpp
  MessageBase.cpp
  MessagePool.cpp
  ParallelLoop.cpp
  PerformanceTools.cpp
  RDMAAggregator.cpp
  SharedMessagePool.cpp
  SimpleMetric.cpp
  StringMetric.cpp
  StateTimer.cpp
  Metrics.cpp
  SummarizingMetric.cpp
  ThreadQueue.cpp
  Timestamp.cpp
  Worker.cpp
  Addressing.hpp
  Aggregator.hpp
  Allocator.hpp
  Array.hpp
  AsyncDelegate.hpp
  Barrier.hpp
  BufferVector.hpp
  boost_helpers.hpp
  Cache.hpp
  ChunkAllocator.hpp
  CallbackMetric.hpp
  CallbackMetricImpl.hpp
  Collective.hpp
  common.hpp
  Communicator.hpp
  CommunicatorImpl.hpp
  CompletionEvent.hpp
  ConditionVariable.hpp
  ConditionVariableLocal.hpp
  CountingSemaphoreLocal.hpp
  Delegate.hpp
  DelegateBase.hpp
  ExternalCountPayloadMessage.hpp
  FileIO.hpp
  FlatCombiner.hpp
  FullEmpty.hpp
  FullEmptyLocal.hpp
  function_traits.hpp
  GlobalAllocator.hpp
  GlobalCompletionEvent.hpp
  GlobalCounter.hpp
  GlobalHashMap.hpp
  GlobalHashSet.hpp
  GlobalMemory.hpp
  GlobalMemoryChunk.hpp
  GlobalVector.hpp
  Grappa.hpp
  HistogramMetric.hpp
  IncoherentAcquirer.hpp
  IncoherentReleaser.hpp
  LocaleSharedMemory.hpp
  Message.hpp
  MessageBase.hpp
  MessageBaseImpl.hpp
  MessagePool.hpp
  Mutex.hpp
  ParallelLoop.hpp
  PerformanceTools.hpp
  PoolAllocator.hpp
  PushBuffer.hpp
  RDMAAggregator.hpp
  RDMABuffer.hpp
  Reducer.hpp
  ReuseList.hpp
  ReuseMessage.hpp
  ReuseMessageList.hpp
  ReusePool.hpp
  Semaphore.hpp
  SharedMessagePool.hpp
  SimpleMetric.hpp
  SimpleMetricImpl.hpp
  StringMetric.hpp
  StringMetricImpl.hpp
  StateTimer.hpp
  MetricBase.hpp
  Metrics.hpp
  MetricsTools.hpp
  SummarizingMetric.hpp
  SummarizingMetricImpl.hpp
  SuspendedDelegate.hpp
  Synchronization.hpp
  Tasking.hpp
  ThreadQueue.hpp
  Timestamp.hpp
  Worker.hpp
  stack.h
  NTBuffer.cpp
  NTMessage.cpp
  tasks/BasicScheduler.hpp
  tasks/DictOut.hpp
  # tasks/GlobalQueue.hpp
  tasks/Scheduler.hpp
  tasks/StealQueue.hpp
  tasks/Task.hpp
  tasks/TaskingScheduler.hpp
  tasks/BasicScheduler.cpp
  # tasks/GlobalQueue.cpp
  tasks/StealQueue.cpp
  tasks/Task.cpp
  tasks/TaskingScheduler.cpp
  stack.S
)

################
# Graph sources
list(APPEND SYSTEM_SOURCES
  graph/Graph.hpp
  graph/Graph.cpp
  graph/TupleGraph.cpp
  graph/TupleGraph.hpp
  graph/KroneckerGenerator.cpp
)

enable_language(ASM)
set_source_files_properties( stack.S PROPERTIES LANGUAGE ASM )

add_library(Grappa
  ${SYSTEM_SOURCES}
  ${SYSTEM_HEADERS}
)
target_link_libraries(Grappa ${GRAPPA_DYNAMIC_LIBS} ${GRAPPA_STATIC_LIBS})
add_dependencies(Grappa all-third-party)

#
# installation
#

install(TARGETS Grappa DESTINATION "lib")

install(DIRECTORY .     DESTINATION "include/Grappa" FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY .     DESTINATION "include/Grappa" FILES_MATCHING PATTERN "*.h")
install(DIRECTORY graph DESTINATION "include/Grappa" FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY tasks DESTINATION "include/Grappa" FILES_MATCHING PATTERN "*.hpp")

install(FILES grappa_gdb.macros DESTINATION "share/Grappa")


#
# tests
#

add_grappa_application(ContextSwitchRate_bench.exe "ContextSwitchRate_bench.cpp")

# create a test, which will be run with the given number of nodes (nnode),
# and processors per node (ppn), and added to the aggregate targets for 
# passing or failing tests
macro(add_check test_cpp nnode ppn target)
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} ${nnode} ${ppn} ${test_cpp} ${ARGN})
  add_dependencies( check-all-${target} check-${test_name} )
  add_dependencies( check-all-${target}-compile-only ${test})
endmacro()

# create separate targets for compiling/runnning all passing/failing tests
foreach(target  pass pass-compile-only fail fail-compile-only)
  add_custom_target(check-all-${target})
  set_property(TARGET check-all-${target} PROPERTY FOLDER "Tests")
endforeach()


add_check( Addressing_tests.cpp              2 2  pass )
add_check( Allocator_tests.cpp               1 1  pass )
add_check( Array_tests.cpp                   2 2  pass )
add_check( BufferVector_tests.cpp            2 2  pass )
add_check( Cache_tests.cpp                   2 1  pass )
add_check( Collective_tests.cpp              2 2  pass )
add_check( CompletionEvent_tests.cpp         2 2  pass )
add_check( ContextSwitchLatency_tests.cpp    1 1  pass )
add_check( Delegate_tests.cpp                2 1  pass )
add_check( FileIO_tests.cpp                  2 1  fail )
add_check( FlatCombiner_tests.cpp            2 2  pass )
add_check( FullEmpty_tests.cpp               2 2  pass )
add_check( GlobalAllocator_tests.cpp         1 1  pass )
add_check( GlobalHash_tests.cpp              2 1  pass )
add_check( GlobalMemoryChunk_tests.cpp       2 1  pass )
add_check( GlobalMemory_tests.cpp            2 1  pass )
add_check( GlobalVector_tests.cpp            2 1  pass )
add_check( Gups_tests.cpp                    2 1  pass )
add_check( LocaleSharedMemory_tests.cpp      1 2  pass )
add_check( Malloc_tests.cpp                  2 1  fail )
add_check( Message_tests.cpp                 2 1  fail )
add_check( Mutex_tests.cpp                   2 1  pass )
add_check( New_delegate_tests.cpp            2 2  pass )
add_check( New_loop_tests.cpp                2 2  pass )
add_check( PoolAllocator_tests.cpp           2 1  pass )
add_check( Public_tasks_tests.cpp            2 1  pass )
add_check( RDMAAggregator_tests.cpp          2 1  pass )
add_check( RateMeasure_tests.cpp             2 1  pass )
add_check( Reducer_tests.cpp                 2 1  pass )
add_check( Scheduler_benchmarking_tests.cpp  2 1  pass )
add_check( Semaphore_tests.cpp               2 1  pass )
add_check( Metrics_tests.cpp                 2 1  pass )
add_check( Stealing_tests.cpp                2 1  fail ) # deprecated?
add_check( Tasking_tests.cpp                 2 1  pass )
add_check( ThreadQueue_tests.cpp             2 1  pass )

add_check( graph/Graph_tests.cpp             2 1  pass )

add_check( NTMessage_tests.cpp               1 1  pass NTMessage.cpp )
add_check( NTBuffer_tests.cpp                1 1  pass NTBuffer.cpp )
add_check( NTMessage_aggregator_tests.cpp    2 1  pass )


#
# begin hack for dealing with communicator test
#

add_library(Communicator
        Communicator.cpp
        LocaleSharedMemory.cpp
)
set_target_properties(Communicator PROPERTIES COMPILE_FLAGS "-DCOMMUNICATOR_TEST")
add_dependencies(Communicator all-third-party)

macro(add_commtest_exe target exe )
  add_executable(${target} EXCLUDE_FROM_ALL ${ARGN})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_NAME "${exe}")
  target_link_libraries(${target}
    Communicator
    ${GRAPPA_STATIC_LIBS}
    ${GRAPPA_DYNAMIC_LIBS}
    ${MPI_CXX_LIBRARIES}
  )
endmacro(add_commtest_exe)

macro(add_commtest_test name nnode ppn)
  get_filename_component(base ${name} NAME_WE) # name without extension
  
  add_commtest_exe(${name} ${name} ${ARGN})
  set_property(TARGET ${name} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${base} -DCOMMUNICATOR_TEST"
  )
  
  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${base}"
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_srun --test=${base} --nnode=${nnode} --ppn=${ppn}
    DEPENDS Communicator ${name}
  )
  add_test(NAME "test-${base}" WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_BINARY_DIR}/bin/grappa_srun --test=${base} --nnode=${nnode} --ppn=${ppn}
  )
  
  set_property(TARGET "check-${base}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${base}")
endmacro(add_commtest_test)

macro(add_commtest_check test_cpp nnode ppn target)
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_commtest_test(${test} ${nnode} ${ppn} ${test_cpp})
endmacro()

add_commtest_check( Communicator_tests.cpp   2 1  pass )

#
# end hack for dealing with communicator test
#


# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  get_property(include_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  string(REPLACE ";" " " CMAKE_CURRENT_INCLUDES "${include_dirs}")
  
  set(DOXYGEN_OUTPUT "doxygen")
  set(DOXYGEN_BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(docs
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMAND echo "Documentation in ${DOXYGEN_BUILD_DIR}/${DOXYGEN_OUTPUT}"
    WORKING_DIRECTORY ${DOXYGEN_BUILD_DIR}
    COMMENT "Generating API documentation with Doxygen." VERBATIM
  )
endif(DOXYGEN_FOUND)
