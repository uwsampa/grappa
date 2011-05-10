from GlobalMemory import GlobalMemory,Delegate
import coros_stack
import threading
from CoreQueue import CoreQueue




def worker(outq, inq, num_thr):
    gm = GlobalMemory(outq, inq)

    def thread_work(me):
        addr = me.tid # just using tid as a mem addr for test
        gm.readIssue(addr, me.tid)
        yield
        data = []
        yield gm.readComplete(addr, me.tid, data)
        dataval = data[0]
        print "%s-%d got back %d from addr %d"%(threading.currentThread().name,me.tid,dataval,addr)

    sched = coros_stack.Scheduler()
    for t in range(0, num_thr):
        sched.newThread(thread_work)
    sched.runAll()

    gm.killConnection()


num_workers = 2
st_per_ht = 3

print "%d workers"%(num_workers)

# create queues
to_del_qs = []
from_del_qs = []
for i in range(0, num_workers):
    to_del_qs.append(CoreQueue.createQueue())
    from_del_qs.append(CoreQueue.createQueue())


cores = []

# workers
for i in range(0, num_workers):
    cores.append(threading.Thread(target=worker, args=(to_del_qs[i], from_del_qs[i], st_per_ht)))

# delegate
delegate = Delegate(from_del_qs, to_del_qs)
cores.append(threading.Thread(target=delegate.delegation))


# run
for i in range(0, len(cores)):
    print cores[i].name, "starting"
    cores[i].start()
for i in range(0, len(cores)):
    cores[i].join()
