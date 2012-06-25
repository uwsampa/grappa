#include <TAU.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

DECLARE_bool(record_grappa_events);

#define SAMPLE_RATE (1<<4)

#define GRAPPA_DEFINE_EVENT_GROUP(group) \
  DEFINE_bool( record_##group, true, "Enable tracing of events in group.")

#define GRAPPA_DECLARE_EVENT_GROUP(group) \
  DECLARE_bool( record_##group )

#ifdef GRAPPA_TRACE
#define GRAPPA_EVENT(name, text, frequency, group, val) \
  static uint64_t name##_calls = 0; \
  name##_calls++; \
  if (FLAGS_##record_##group && (FLAGS_record_grappa_events) && (name##_calls % frequency == 0)) { \
    TAU_REGISTER_EVENT(name, text); \
    TAU_EVENT(name, val); \
  } \
  do {} while (0)
#else
#define GRAPPA_EVENT(name, text, frequency, group, val) do {} while (0)
#endif

void SoftXMT_set_profiler_argv0( char * argv0 );

void SoftXMT_start_profiling();
void SoftXMT_stop_profiling();

extern bool take_profiling_sample;

