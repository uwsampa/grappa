

class Coroutine:

    def __init__(self, tid, func):
        self.tid = tid
        self.func = func
    
    @classmethod
    def Yield(cls):
        yield False   

class Scheduler:

    def __init__(self):
        self.threads = []
        self.next_tid = 0


    def newThread(self, func):
        t = Coroutine(self.assignTid(), func)
        self.threads.append(t)
        return t


    def assignTid(self):
        tid = self.next_tid
        self.next_tid+=1
        return tid


    def runAll(self):
        if len(self.threads)==0: return

        # get generators and initialize the stacks
        stacks = []
        for t in self.threads:
            s = [t.func(t)]
            stacks.append(s)
    
        gen_type = type(stacks[0][0])

        # add threads to run queue
        runqueue = []
        for t in self.threads:
            runqueue.append(t)

        # run all threads loop
        while(runqueue):
            # get next runnable (round robin)
            t = runqueue.pop(0)

            # run the thread until yield() or terminate
            return_val = None
            coro_stack = stacks[t.tid]
            coro = coro_stack[len(coro_stack)-1]
            coro_frame = coro   #for now assumes tid is always 0 to #threads-1
            while True:
                try:
                    coro_frame = coro_frame.send(return_val) # trampoline
                except StopIteration:
                    coro_stack.pop()

                    # case 1: function return but not top level of coro
                    if coro_stack:
                        coro_frame = coro_stack[len(coro_stack)-1]
                        continue
                    # case 2: function return from top level so thread terminate
                    else:
                        #do not re-enqueue
                        break
                
                # case 3: function call
                if type(coro_frame)==gen_type:   
                    # push new stack frame
                    coro_stack.append(coro_frame)

                    # make sure not passing an arg to the first call
                    return_val = None

                # case 4: coroutine does a real yield()
                else: 
                    return_val = coro_frame
                    runqueue.append(t) # re-enqueue thread
                    break




def main():

    def deep3(me):
        print me.tid,"in frame 3"
        yield Coroutine.Yield()

    def deep2(me):
        print me.tid,"in frame 2"
        yield deep3(me)
        print me.tid,"in frame 2 after call"
        yield 100

    def thread_code(me):
        print me.tid,"in frame 1"
        ret = (yield deep2(me))
        print me.tid,"in frame 1 after and received retval",100
        



    s = Scheduler()
    s.newThread(thread_code)
    s.newThread(thread_code)
    s.runAll()



if __name__ == "__main__":
    main()





