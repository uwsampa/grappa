
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
  CallbackStatistic.cpp
  Collective.cpp
  Communicator.cpp
  Delegate.cpp
  FlatCombiner.cpp
  GlobalAllocator.cpp
  GlobalCompletionEvent.cpp
  GlobalHashMap.cpp
  GlobalHashSet.cpp
  GlobalMemory.cpp
  GlobalMemoryChunk.cpp
  GlobalVector.cpp
  Grappa.cpp
  HistogramStatistic.cpp
  IncoherentAcquirer.cpp
  IncoherentReleaser.cpp
  LocaleSharedMemory.cpp
  MaxStatistic.cpp
  MessageBase.cpp
  MessagePool.cpp
  ParallelLoop.cpp
  PerformanceTools.cpp
  RDMAAggregator.cpp
  SharedMessagePool.cpp
  SimpleStatistic.cpp
  StateTimer.cpp
  Statistics.cpp
  SummarizingStatistic.cpp
  ThreadQueue.cpp
  Timestamp.cpp
  Worker.cpp
  Addressing.hpp
  Aggregator.hpp
  Allocator.hpp
  Array.hpp
  AsyncDelegate.hpp
  Barrier.hpp
  boost_helpers.hpp
  Cache.hpp
  CallbackStatistic.hpp
  CallbackStatisticImpl.hpp
  Collective.hpp
  common.hpp
  Communicator.hpp
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
  HistogramStatistic.hpp
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
  SimpleStatistic.hpp
  SimpleStatisticImpl.hpp
  StateTimer.hpp
  StatisticBase.hpp
  Statistics.hpp
  StatisticsTools.hpp
  SummarizingStatistic.hpp
  SummarizingStatisticImpl.hpp
  SuspendedDelegate.hpp
  Synchronization.hpp
  Tasking.hpp
  ThreadQueue.hpp
  Timestamp.hpp
  Worker.hpp
  gasnet_helpers.h
  stack.h
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


if( GOOGLE_PROFILER )
  list(APPEND GRAPPA_LIBS "profiler")
endif()

enable_language(ASM)
set_source_files_properties( stack.S PROPERTIES LANGUAGE ASM )

add_library(Grappa
  ${SYSTEM_SOURCES}
  ${SYSTEM_HEADERS}
)
target_link_libraries(Grappa ${GRAPPA_LIBS})
add_dependencies(Grappa all-tools)

add_grappa_application(ContextSwitchRate_bench.exe "ContextSwitchRate_bench.cpp")

macro(add_check test_cpp nnode ppn target)
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} ${nnode} ${ppn} ${test_cpp})
  add_dependencies( check-all-${target} check-${test_name} )
  add_dependencies( check-all-${target}-compile-only ${test})
endmacro()

# create separate targets for compiling/runnning all passing/failing tests
foreach(target  pass pass-compile-only fail fail-compile-only)
  add_custom_target(check-all-${target})
  set_property(TARGET check-all-${target} PROPERTY FOLDER "Tests")
endforeach()


add_check( Addressing_tests.cpp              2 2  pass )
add_check( Allocator_tests.cpp               2 2  pass )
add_check( Array_tests.cpp                   2 2  pass )
add_check( BufferVector_tests.cpp            2 2  pass )
add_check( Cache_tests.cpp                   2 1  pass )
add_check( Collective_tests.cpp              2 2  pass )
add_check( CompletionEvent_tests.cpp         2 2  pass )
add_check( ContextSwitchLatency_tests.cpp    1 1  pass )
add_check( Delegate_tests.cpp                2 1  pass )
add_check( FileIO_tests.cpp                  2 1  pass )
add_check( FlatCombiner_tests.cpp            2 2  pass )
add_check( FullEmpty_tests.cpp               2 2  pass )
add_check( GlobalAllocator_tests.cpp         1 1  pass )
add_check( GlobalHash_tests.cpp              2 1  pass )
add_check( GlobalMemoryChunk_tests.cpp       2 1  pass )
add_check( GlobalMemory_tests.cpp            2 1  pass )
add_check( GlobalVector_tests.cpp            2 1  pass )
add_check( Gups_tests.cpp                    2 1  pass )
add_check( LocaleSharedMemory_tests.cpp      2 1  pass )
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
add_check( Statistics_tests.cpp              2 1  pass )
add_check( Stealing_tests.cpp                2 1  fail ) # deprecated?
add_check( Tasking_tests.cpp                 2 1  pass )
add_check( ThreadQueue_tests.cpp             2 1  pass )


# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
  get_property(CMAKE_CURRENT_INCLUDES DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
  
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
