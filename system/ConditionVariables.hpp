#ifndef __CONDITION_VARIABLES_HPP__
#define __CONDITION_VARIABLES_HPP__

struct Thread;

/**
 * Signaler
 *
 * Object for one-time, one-way signal synchronization.
 * First call to signal() will cause past or future calls
 * to wait() to return.
 * 
 * Essentially behaves as a one-time-use, one-element
 * producer-consumer queue.
 */
class Signaler {
  private:
    Thread * waiter;
    bool done;

  public:
    /// Create new Signaler for one-time use
    Signaler();

    /// Returns when signal has been called.
    void wait();

    /// Allows callers of wait() to return
    /// in the future.
    void signal();
};


/**
 * ConditionVariable
 *
 * Object for sleeping until a condition might
 * be true.
 * This object does not enforce that the condition
 * remain true until the sleeping thread runs. Therefore, for
 * correct behavior, wait() should be used inside
 * a while loop that checks the condition.
 */
class ConditionVariable {
  private:
    Thread * waiter;

  public:
    ConditionVariable();
    void wait();
    void notify();
};

#endif // __CONDITION_VARIABLES_HPP__
