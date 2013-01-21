
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
  Addressing.hpp
  Aggregator.cpp
  Aggregator.hpp
  Allocator.cpp
  Allocator.hpp
  Array.hpp
  AsyncParallelFor.cpp
  AsyncParallelFor.hpp
  Cache.cpp
  Cache.hpp
  Collective.cpp
  Collective.hpp
  Communicator.cpp
  Communicator.hpp
  ConditionVariable.hpp
  ConditionVariableLocal.hpp
  CurrentThread.cpp
  CurrentThread.hpp
  Delegate.cpp
  Delegate.hpp
  Doxyfile
  FileIO.hpp
  ForkJoin.cpp
  ForkJoin.hpp
  FullEmpty.hpp
  Future.cpp
  Future.hpp
  GlobalAllocator.cpp
  GlobalAllocator.hpp
  GlobalMemory.cpp
  GlobalMemory.hpp
  GlobalMemoryChunk.cpp
  GlobalMemoryChunk.hpp
  GlobalTaskJoiner.cpp
  GlobalTaskJoiner.hpp
  Grappa.cpp
  Grappa.hpp
  IncoherentAcquirer.cpp
  IncoherentAcquirer.hpp
  IncoherentReleaser.cpp
  IncoherentReleaser.hpp
  Message.hpp
  MessageBase.hpp
  MutableHeap.hpp
  Mutex.hpp
  ParallelLoop.hpp
  ParallelLoop_future.hpp
  PerformanceTools.cpp
  PerformanceTools.hpp
  ProfilerConfig.hpp
  ProfilerGroups.cpp
  ProfilerGroups.hpp
  PushBuffer.hpp
  RDMAAggregator.cpp
  RDMAAggregator.hpp
  StateTimer.cpp
  StateTimer.hpp
  StatisticsTools.hpp
  Synchronization.hpp
  Tasking.hpp
  Timestamp.cpp
  Timestamp.hpp
  Uid.cpp
  Uid.hpp
  boost_helpers.hpp
  common.hpp
  coro.c
  coro.h
  stack.S
  stack.h
  tasks/BasicScheduler.cpp
  tasks/BasicScheduler.hpp
  tasks/DictOut.hpp
  tasks/Scheduler.hpp
  tasks/StealQueue.hpp
  tasks/Task.cpp
  tasks/Task.hpp
  tasks/TaskingScheduler.cpp
  tasks/TaskingScheduler.hpp
  tasks/Thread.cpp
  tasks/Thread.hpp
)

message("source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

include_directories(
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/tasks"
)

add_library(Grappa
  ${SYSTEM_SOURCES}
  ${SYSTEM_HEADERS}
)
if( GOOGLE_PROFILER )
  target_link_libraries(Grappa profiler)
endif()

set(ENV_VARIABLES "")
set(TEST_LOG_LEVEL "nothing")
set(TEST_REPORT_LEVEL "no")

# create targets for each test
foreach(test_cpp ${TEST_SOURCES})
  # string(REPLACE ".cpp" "" test_name ${test_cpp})
  get_filename_component(test_name ${test_cpp} NAME)

  add_executable("${test_name}.test" EXCLUDE_FROM_ALL ${test_cpp})
  target_link_libraries(${test} Grappa)
  set_property(TARGET "${test_name}.test" PROPERTY FOLDER "Tests")

  # TODO: use "ctest" to make whole regression test suite
  add_custom_target("check-${test_name}"
    OUTPUT ""
    COMMAND ${ENV_VARIABLES} ${MPIRUN} "${test_name}.test" --log_level=${TEST_LOG_LEVEL} --report_level=${TEST_REPORT_LEVEL} --run_test="${test_name}.test"
    DEPENDS Grappa "${test_name}.test"
  )
  set_property(TARGET "check-${test_name}" PROPERTY FOLDER "Tests")
endforeach()