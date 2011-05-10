import threading
import Queue

class CoreQueue:
    TYPE = 'simple'


    @classmethod
    def createQueue(cls,size=16):
        if CoreQueue.TYPE=='simple':
            return CoreQueueSimple(size)
        else:
            assert False



class CoreQueueSimple(CoreQueue):
    
    def __init__(self, bufsize):
        self.queue = Queue.Queue(bufsize)

    def produce(self, val):
        self.queue.put(val, block=True)

    def consume(self):
        return self.queue.get(block=True)

    # clearly if there are multiple consumers this does not guarentee no block on consume()
    def canConsume(self):
        return not self.queue.empty()
    



"""
class CoreQueueBuffered(CoreQueue):

    def __init__(self, bufsize):
        self.queue1 = []
        self.queue2 = []
        self.prod1 = True
        self.bufsize = bufsize
        self.lock = thread.Lock()

    def produce(self, val):
        if self.prod1:
            if len(self.queue1) < bufsize:
                self.queue1.append(val)


            if len(self.queue1) == bufsize:
                self.l
"""
