#include <ParallelLoop.hpp>
#include "relation_io.hpp" // for aligned_vector only


// Iterate over a symmetric global vector, that is each partition has
// a local vector pointed to by the address symmetric_array
template <GlobalCompletionEvent* GCE=&impl::local_gce, typename T, typename F>
void forall(GlobalAddress<aligned_vector<T>> symmetric_array, F loop_body) {
  auto origin = mycore();
  GCE->enroll(cores());
  on_all_cores([=] {
      auto num_elements = symmetric_array->vector.size();
      forall_here<SyncMode::Async,GCE>( 0, num_elements, [=](int64_t start, int64_t iters) {
        for (int64_t j=start; j<start+iters; j++) {
        auto& el = symmetric_array->vector[j];
        loop_body(el);
        }
        });
      GCE->send_completion(origin);
      GCE->wait();
      });
}




template <GlobalCompletionEvent* GCE=&impl::local_gce, typename T, typename F>
void forall_enum(GlobalAddress<aligned_vector<T>> symmetric_array, F loop_body) {
  auto origin = mycore();
  GCE->enroll(cores());
  on_all_cores([=] {
      auto num_elements = symmetric_array->vector.size();
      forall_here<SyncMode::Async,GCE>( 0, num_elements, [=](int64_t start, int64_t iters) {
        for (int64_t j=start; j<start+iters; j++) {
        auto el = symmetric_array->vector[j];
        loop_body(j, el);
        }
        });
      GCE->send_completion(origin);
      GCE->wait();
      });
}
