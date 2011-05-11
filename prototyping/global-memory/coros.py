

class Coroutine:

    def __init__(self, tid, func):
        self.tid = tid
        self.func = func

    

class Scheduler:

    def __init__(self):
        self.threads = []
        self.next_tid = 0


    def newThread(self, func):
        t = Coroutine(self, self.assignTid(), func)
        self.threads.append(t)

        return t


    def assignTid(self):
        tid = self.next_tid
        self.next_tid+=1
        return tid


    def runAll(self):
        # get generators
        gens = []
        for t in self.threads:
           gens.append(t.func(t))

        # add threads to run queue
        runqueue = []
        for t in self.threads:
            runqueue.append(t)


        while(runqueue):
            # get next runnable (round robin)
            t = runqueue.pop(0)

            # run the threaD
            try:
                gens[t.tid].next()    #for now assumes tid is always 0 to #threads-1
                runqueue.append(t) # re-enqueue thread
            except StopIteration:
                # do not re-enqueue thread
                None




def main():
    def thread_code(me):
        for i in range(0,3):
            print me.tid,"is",i
            yield 1


    s = Scheduler()
    s.newThread(thread_code)
    s.newThread(thread_code)
    s.runAll()



if __name__ == "main":
    main()






