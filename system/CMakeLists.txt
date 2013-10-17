
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
  AsyncParallelFor.cpp
  Barrier.cpp
  Cache.cpp
  Collective.cpp
  Communicator.cpp
  ContextSwitchRate_bench.cpp
  CurrentThread.cpp
  Delegate.cpp
  FlatCombiner.cpp
  ForkJoin.cpp
  Future.cpp
  GlobalAllocator.cpp
  GlobalCompletionEvent.cpp
  GlobalHashMap.cpp
  GlobalHashSet.cpp
  GlobalMemory.cpp
  GlobalMemoryChunk.cpp
  GlobalTaskJoiner.cpp
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
  Uid.cpp
  Worker.cpp
  Addressing.hpp
  Aggregator.hpp
  Allocator.hpp
  Array.hpp
  AsyncDelegate.hpp
  AsyncParallelFor.hpp
  Barrier.hpp
  boost_helpers.hpp
  Cache.hpp
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
  ForkJoin.hpp
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
  GlobalTaskJoiner.hpp
  GlobalVector.hpp
  Grappa.hpp
  HistogramStatistic.hpp
  IncoherentAcquirer.hpp
  IncoherentReleaser.hpp
  LegacyDelegate.hpp
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
  Uid.hpp
  Worker.hpp
  gasnet_helpers.h
  stack.h
  thread.h
  tasks/BasicScheduler.hpp
  tasks/DictOut.hpp
  tasks/GlobalQueue.hpp
  tasks/Scheduler.hpp
  tasks/StealQueue.hpp
  tasks/Task.hpp
  tasks/TaskingScheduler.hpp
  tasks/BasicScheduler.cpp
  tasks/GlobalQueue.cpp
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

set(GRAPPA_TESTS "")

# create targets for each test
foreach(test_cpp ${TEST_SOURCES})
  get_filename_component(test_name ${test_cpp} NAME_WE) # name without extension
  set(test "${test_name}.test")

  # include Boost test automagic main() generation
  # add an additional rule if you need to include multiple components

  add_executable(${test} EXCLUDE_FROM_ALL ${test_cpp})
  target_link_libraries(${test} Grappa)
  set_property(TARGET ${test} PROPERTY FOLDER "Tests") # For organization in Xcode project
  set_target_properties(${test} PROPERTIES
    COMPILE_FLAGS "-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${test_name}"
  )

  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${test_name}"
    COMMAND ${CMAKE_SOURCE_DIR}/bin/grappa_srun.sh -- "${test_name}.test" --log_level=test_suite --report_level=confirm --run_test="${test_name}"
    DEPENDS Grappa "${test_name}.test"
  )
  set_property(TARGET "check-${test_name}" PROPERTY FOLDER "Tests")
  list(APPEND GRAPPA_TESTS "check-${test_name}")
endforeach()

add_custom_target("check-all" DEPENDS ${GRAPPA_TESTS})
