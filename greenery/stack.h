#ifndef STACK_H
#define STACK_H



struct coro;

typedef void (*coro_func)(struct coro *, void *arg);

// move stacks from <old> to <new> and return <ret> (to new guy)
void* swapstacks(void **olds, void **news, void *ret);

// Given memory going DOWN FROM <stack>, create a basic stack we can swap to
// (using swapstack) that will call <f>. (using <it> as its <me>).
// <me> is a location we can store the current stack.
void makestack(void **me, void **stack, coro_func f, struct coro *it);

#endif
