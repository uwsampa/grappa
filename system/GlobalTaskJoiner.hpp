#include "SoftXMT.hpp"
#include "Tasking.hpp"
#include "Delegate.hpp"
#include "Addressing.hpp"

/// Attempting to do GlobalTaskJoiner that acts like LocalTaskJoiner but
/// does a kind of cancellable barrier when each node has 0 outstanding
/// tasks. It is 'cancelled' if more work is stolen by the tasking layer.

/// This is an attempt to roll it all together in a single class by just
/// adding the desired global barrier messages to the joiner code. This
/// version also keeps the original waiting task suspended and uses AMs
/// exclusively to do synchronization.
struct GlobalTaskJoiner {
  // Master barrier
  Node nodes_in;
  bool barrier_done; // only for our own verification

  // All nodes
  Thread * waiter;
  int64_t outstanding;
  bool global_done;
  bool cancel_in_flight;
  bool enter_called;
 
  // Const
  Node target;
  GlobalAddress<GlobalTaskJoiner> _addr;
  
  //GlobalTaskJoiner(): localComplete(false), target(0), outstanding(0), nodes_outstanding(0), waiter(NULL) {}
  GlobalTaskJoiner(): nodes_in(0), barrier_done(false), waiter(NULL), outstanding(0), global_done(false), cancel_in_flight(false), enter_called(false), target(0 /* Node 0 == Master */) {}
  void reset();
  GlobalAddress<GlobalTaskJoiner> addr() {
    if (_addr.pointer() == NULL) { _addr = make_global(this); }
    return _addr;
  }
  void registerTask();
  void signal();
  void wait();
  static void remoteSignal(GlobalAddress<GlobalTaskJoiner> joiner);
  static void remoteSignalNode(Node joiner_node);
private:
  void wake();
  void send_enter();
  void send_cancel();
  static void am_wake(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_enter(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_remoteSignal(GlobalAddress<GlobalTaskJoiner>* joiner, size_t sz, void* payload, size_t psz);
  static void am_remoteSignalNode(int* dummy_arg, size_t sz, void* payload, size_t psz);
};
extern GlobalTaskJoiner global_joiner;


///
/// Spawning tasks that use the global_joiner
///


#include "AsyncParallelFor.hpp"
template < void (*LoopBody)(int64_t,int64_t) >
void joinerSpawn( int64_t s, int64_t n );

/// task wrapper: signal upon user task completion
template < void (*LoopBody)(int64_t,int64_t) >
void asyncFor_with_globalTaskJoiner(int64_t s, int64_t n, GlobalAddress<GlobalTaskJoiner> joiner) {
  //NOTE: really we just need the joiner Node because of the static global_joiner
  async_parallel_for<LoopBody, &joinerSpawn<LoopBody> > (s, n);
  global_joiner.remoteSignal( joiner );
}

/// spawn wrapper: register new task before spawned
template < void (*LoopBody)(int64_t,int64_t) >
void joinerSpawn( int64_t s, int64_t n ) {
  global_joiner.registerTask();
  SoftXMT_publicTask(&asyncFor_with_globalTaskJoiner<LoopBody>, s, n, make_global( &global_joiner ) );
}



template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>) >
void joinerSpawn_hack( int64_t s, int64_t n, GlobalAddress<Arg> shared_arg );

/// task wrapper: signal upon user task completion
template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>) >
void asyncFor_with_globalTaskJoiner_hack(int64_t s, int64_t n, GlobalAddress<Arg> shared_arg ) {
  async_parallel_for<Arg, LoopBody, &joinerSpawn_hack<Arg, LoopBody> > (s, n, shared_arg);
  global_joiner.remoteSignalNode( shared_arg.node() ); 
}

template < typename Arg,
           void (*LoopBody)(int64_t,int64_t,GlobalAddress<Arg>) >
void joinerSpawn_hack( int64_t s, int64_t n, GlobalAddress<Arg> shared_arg ) {
  global_joiner.registerTask();

  // copy the shared_arg data into a global address that corresponds to this Node
  GlobalAddress<Arg> packed = make_global( reinterpret_cast<Arg*>(shared_arg.pointer()) );
  SoftXMT_publicTask( &asyncFor_with_globalTaskJoiner_hack<Arg,LoopBody>, s, n, packed );
} 
