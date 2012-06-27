#include "GlobalTaskJoiner.hpp"

GlobalTaskJoiner global_joiner;



void GlobalTaskJoiner::reset() {
  localComplete = false;
  target = 0; // for now, just have everyone report into Node 0
  outstanding = 0;
  nodes_outstanding = SoftXMT_nodes();
  waiter = NULL;
}

void GlobalTaskJoiner::registerTask() {
  if (localComplete) cancel_completed();
  outstanding++;
  VLOG(3) << "registered - outstanding = " << outstanding;
}

void GlobalTaskJoiner::signal() {
  CHECK(outstanding > 0) << "too many calls to signal()";
  
  outstanding--;
  VLOG(3) << "signaled - outstanding = " << outstanding;

  if (outstanding == 0) {
    notify_completed();
  }
}

void GlobalTaskJoiner::wake() {
  CHECK(outstanding == 0);
  localComplete = false;
  if ( waiter != NULL ) {
    Thread * w = waiter;
    waiter = NULL;
    SoftXMT_wake(w);
  }
}

void GlobalTaskJoiner::wait() {
  if (outstanding == 0) {
    VLOG(2) << "no local work, entering cancellable barrier";
    notify_completed();
  }
  
  if ( localComplete || outstanding > 0 ) {  // IF, because conditions should not change once correct
    waiter = CURRENT_THREAD;
    SoftXMT_suspend(); // won't be woken until *all* nodes have no work left
  }
}

void GlobalTaskJoiner::am_remoteSignal(GlobalAddress<GlobalTaskJoiner>* joiner, size_t sz, void* payload, size_t psz) {
  CHECK(joiner->node() == SoftXMT_mynode());
  //joiner->pointer()->signal();
  global_joiner.signal(); // equivalent to above, right now
}

void GlobalTaskJoiner::remoteSignal(GlobalAddress<GlobalTaskJoiner> joiner) {
  if (joiner.node() == SoftXMT_mynode()) {
    //joiner.pointer()->signal();
    global_joiner.signal(); // equivalent to above, right now
  } else {
    VLOG(2) << "remoteSignal -> " << joiner.node();
    SoftXMT_call_on(joiner.node(), &GlobalTaskJoiner::am_remoteSignal, &joiner);
  }
}

void GlobalTaskJoiner::am_wake(bool * ignore, size_t sz, void * p, size_t psz) {
  global_joiner.wake();
}

void GlobalTaskJoiner::am_notify_completed(bool * ignore, size_t sz, void * p, size_t psz) {
  CHECK(SoftXMT_mynode() == 0); // for now, Node 0 is responsible for global state
  global_joiner.nodes_outstanding--;
  VLOG(2) << "(completed) nodes_outstanding: " << global_joiner.nodes_outstanding;
  if (global_joiner.nodes_outstanding == 0) {
    global_joiner.nodes_outstanding = SoftXMT_nodes();
    for (Node n=0; n<SoftXMT_nodes(); n++) {
      bool ignore = false;
      SoftXMT_call_on(n, &GlobalTaskJoiner::am_wake, &ignore);
    }
  }
}

void GlobalTaskJoiner::notify_completed() {
  localComplete = true;
  VLOG(3) << "notify_completed";
  SoftXMT_call_on(target, &GlobalTaskJoiner::am_notify_completed, &localComplete);
}

void GlobalTaskJoiner::cancel_completed() {
  localComplete = false;
  GlobalAddress<GlobalTaskJoiner> global_joiner_addr = make_global(&global_joiner, target);
  GlobalAddress<int64_t> global_outstanding_addr = global_pointer_to_member(global_joiner_addr, &GlobalTaskJoiner::nodes_outstanding);
  int64_t result = SoftXMT_delegate_fetch_and_add_word(global_outstanding_addr, 1);
  VLOG(2) << "(cancelled) nodescomplete: " << result+1;
}

