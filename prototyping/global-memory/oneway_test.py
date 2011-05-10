from CoreQueue import CoreQueue
import threading
import coros_stack


#One way queue


def workers(outg, inc):

    def myfunc(i):
        outg.produce(i)
        coros_stack.Coroutine.Yield()

    def thread_code(me):
        for x in range(me.tid*10,(me.tid+1)*10):
            yield myfunc(x)

    sched = coros_stack.Scheduler()
    sched.newThread(thread_code)
    sched.newThread(thread_code)
    sched.runAll()


def delegates(outg, inc):
    for x in range(0,20):
        print inc.consume()
        


wtod = CoreQueue.createQueue()
dtow = CoreQueue.createQueue()


p = threading.Thread(target=workers, args=(wtod,dtow))
c = threading.Thread(target=delegates, args=(dtow,wtod))


#run
p.start()
c.start()
p.join()
c.join()
