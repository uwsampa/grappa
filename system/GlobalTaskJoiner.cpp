#include "GlobalTaskJoiner.hpp"

GlobalTaskJoiner global_joiner;



void GlobalTaskJoiner::reset() {
  VLOG(2) << "reset";
  // only matters for Master
  nodes_in = 0;
  barrier_done = false;
 
  // all
  global_done = false;
  outstanding = 0;
  waiter = NULL;
  cancel_in_flight = false;
  enter_called = false;

  SoftXMT_barrier_suspending();
  VLOG(2) << "barrier_done = " << barrier_done;
}

void GlobalTaskJoiner::registerTask() {
  outstanding++;
  if (outstanding == 1) { // 0 -> 1
    send_cancel();
  }
  VLOG(3) << "registered - outstanding = " << outstanding;
}

void GlobalTaskJoiner::signal() {
  CHECK(outstanding > 0) << "too many calls to signal(): outstanding == " << outstanding;
  
  outstanding--;
  VLOG(3) << "signaled - outstanding = " << outstanding;

  if (outstanding == 0) { // 1 -> 0
    send_enter();
  }
}

void GlobalTaskJoiner::wake() {
  CHECK(!global_done);
  CHECK(outstanding == 0) << "outstanding = " << outstanding;
  VLOG(2) << "wake!";
  global_done = true;
  
  if ( waiter != NULL ) {
    SoftXMT_wake(waiter);
    waiter = NULL;
  }
}

void GlobalTaskJoiner::wait() {
  if (!global_done) {
    if (!cancel_in_flight && outstanding == 0 && !enter_called) {
      // TODO: doing extra 'cancel_in_flight' check
      VLOG(2) << "no local work, entering cancellable barrier";
      send_enter();
    }
    waiter = CURRENT_THREAD;
    SoftXMT_suspend();
    CHECK(global_done);
    CHECK(!cancel_in_flight);
    CHECK(outstanding == 0);
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

// version of remote signal that takes the Node of the joiner only
void GlobalTaskJoiner::am_remoteSignalNode(int* dummy_arg, size_t sz, void* payload, size_t psz) {
  global_joiner.signal(); 
}

void GlobalTaskJoiner::remoteSignalNode(Node joiner_node) {
  if (joiner_node == SoftXMT_mynode()) {
    global_joiner.signal(); 
  } else {
    VLOG(2) << "remoteSignalNode -> " << joiner_node;
    int dummy_arg = -1;
    SoftXMT_call_on(joiner_node, &GlobalTaskJoiner::am_remoteSignalNode, &dummy_arg);
  }
}

void GlobalTaskJoiner::am_wake(bool * ignore, size_t sz, void * p, size_t psz) {
  global_joiner.wake();
}

void GlobalTaskJoiner::am_enter(bool * ignore, size_t sz, void * p, size_t psz) {
  CHECK(SoftXMT_mynode() == 0); // Node 0 is Master
  //CHECK(!global_joiner.barrier_done);

  global_joiner.nodes_in++;
  VLOG(2) << "(completed) nodes_in: " << global_joiner.nodes_in;
  
  if (global_joiner.nodes_in == SoftXMT_nodes()) {
    global_joiner.barrier_done = true;
    VLOG(2) << "### done! ### nodes_in: " << global_joiner.nodes_in;
    for (Node n=0; n<SoftXMT_nodes(); n++) {
      bool ignore = false;
      SoftXMT_call_on(n, &GlobalTaskJoiner::am_wake, &ignore);
    }
  }
}

void GlobalTaskJoiner::send_enter() {
  enter_called = true;
  if (!cancel_in_flight) {
    VLOG(2) << "notify_completed";
    bool ignore = false;
    SoftXMT_call_on(target, &GlobalTaskJoiner::am_enter, &ignore);
  }
}

void GlobalTaskJoiner::send_cancel() {
  if (!cancel_in_flight && enter_called) {
    cancel_in_flight = true;
    enter_called = false;

    GlobalAddress<GlobalTaskJoiner> global_joiner_addr = make_global(&global_joiner, target);
    GlobalAddress<int64_t> global_nodes_in_addr = global_pointer_to_member(global_joiner_addr, &GlobalTaskJoiner::nodes_in);
    int64_t result = SoftXMT_delegate_fetch_and_add_word(global_nodes_in_addr, -1);
    VLOG(2) << "(cancelled) nodes_in: " << result-1;

    cancel_in_flight = false;
   
    CHECK(outstanding > 0);
    //if (outstanding == 0) {
      //send_enter();
    //}
  }
}

