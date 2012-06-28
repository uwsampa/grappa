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
  Thread * waiter;
  Node nodes_outstanding;
  int64_t outstanding;
  bool localComplete;
  Node target;
  GlobalAddress<GlobalTaskJoiner> _addr;
  
  GlobalTaskJoiner(): localComplete(false), target(0), outstanding(0), nodes_outstanding(0), waiter(NULL) {}
  void reset();
  GlobalAddress<GlobalTaskJoiner> addr() {
    if (_addr.pointer() == NULL) { _addr = make_global(this); }
    return _addr;
  }
  void registerTask();
  void signal();
  void wait();
  static void remoteSignal(GlobalAddress<GlobalTaskJoiner> joiner);
private:
  void wake();
  void notify_completed();
  void cancel_completed();
  static void am_wake(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_notify_completed(bool * ignore, size_t sz, void * p, size_t psz);
  static void am_remoteSignal(GlobalAddress<GlobalTaskJoiner>* joiner, size_t sz, void* payload, size_t psz);
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
