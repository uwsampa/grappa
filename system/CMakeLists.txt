
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
  CurrentThread.cpp
  Delegate.cpp
  FlatCombiner.cpp
  Future.cpp
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
  LegacySignaler.cpp
  LocaleSharedMemory.cpp
  MessageBase.cpp
  MessagePool.cpp
  ParallelLoop.cpp
  PerformanceTools.cpp
  ProfilerGroups.cpp
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
  CurrentThread.hpp
  Delegate.hpp
  Descriptor.hpp
  ExternalCountPayloadMessage.hpp
  FileIO.hpp
  FlatCombiner.hpp
  FullEmpty.hpp
  FullEmptyLocal.hpp
  function_traits.hpp
  Future.hpp
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
  LegacySignaler.hpp
  LocaleSharedMemory.hpp
  Message.hpp
  MessageBase.hpp
  MessageBaseImpl.hpp
  MessagePool.hpp
  MutableHeap.hpp
  Mutex.hpp
  ParallelLoop.hpp
  ParallelLoop_future.hpp
  PerformanceTools.hpp
  PoolAllocator.hpp
  ProfilerConfig.hpp
  ProfilerGroups.hpp
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
  thread.h
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

# (a bit of a hack) without this, stack.o isn't generated
# ... could also add custom command, but this seems easier
set_source_files_properties( stack.S PROPERTIES LANGUAGE C )

add_library(Grappa
  ${SYSTEM_SOURCES}
  ${SYSTEM_HEADERS}
)
target_link_libraries(Grappa ${GRAPPA_LIBS})
add_dependencies(Grappa all-tools)

# create targets for each test
foreach(test_cpp ${TEST_SOURCES})  
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")
  add_grappa_test(${test} ${test_cpp})
  
  # include Boost test automagic main() generation
  # add an additional rule if you need to include multiple components
  
endforeach()

add_custom_target(check-all DEPENDS ${GRAPPA_TESTS})
set_property(TARGET check-all PROPERTY FOLDER "Tests")


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
