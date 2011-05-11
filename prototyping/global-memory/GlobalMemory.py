import coros_stack

# commands
READ = 0
WRITE = 1
FETCH_INC = 2
KILLQ = 3

class GlobalMemory:

    def __init__(self, outq, inq):
        self.oq = outq
        self.iq = inq

        self.rid_to_data = {}
        self.addrid_to_rid = {}

        self.next_rid = 0

    def __assignRid(self):
        r = self.next_rid
        self.next_rid+=1
        return r

    def readIssue(self, addr, tid):
        rid = self.__assignRid()
        self.addrid_to_rid[(addr,tid)] = rid

        #TODO saved mapping for repeat reads

        # (addr, cmd, rid)
        request = (addr, READ, rid)

        self.oq.produce(request)

    # must be called from coro with "yield readComplete(...)"
    def readComplete(self, addr, tid, retval):
        expect_id = self.addrid_to_rid.pop((addr,tid))

        
        while True:
            # first check my mailbox
            already_data = self.rid_to_data.get(expect_id)
            if already_data is not None:
                self.rid_to_data.pop(expect_id)
                retval.append(already_data)  # could also just yield the 'data' like a return value
                return

            # response has not been dequeue yet so try finding
            
            # empty the response queue or until get mine
            while self.iq.canConsume():
                (rid, data) = self.iq.consume()
                if expect_id==rid:
                   retval.append(data)  # could also just yield the 'data' like a return value
                   return
                else:
                    self.rid_to_data[rid] = data

            # yield and try to find mine again later
            yield coros_stack.Coroutine.Yield()

    def killConnection(self):
        request = (0, KILLQ, self.__assignRid())
        self.oq.produce(request)



class Delegate:
    InitMemory = {0:1, 1:2, 2:4, 3:8, 4:16, 5:32, 6:64, 7:128}
    NetMemory = {0:0, 1:101, 2:202, 3:303, 4:404, 5:505, 6:606, 7:707}


    def __resolveMemory(self, addr):
        # TODO mapping
        if addr>>63==1: # network
            return (True, addr&0xffffffff)
        else:
            return (False, addr&0xffffffff)

    def __init__(self, outqs, inqs):
        self.oqs = outqs
        self.iqs = inqs

        self.memory = dict(Delegate.InitMemory)
        self.net = dict(Delegate.NetMemory)

    def delegation(self):
        active_qs = []
        for c in range(0, len(self.iqs)):
            active_qs.append(True)

        active_count = [len(active_qs)]

        def killQ(i):
            active_qs[i] = False
            active_count[0]-=1

        while active_count[0] > 0:
            #TODO's
            # currently simple: not optimizing for memory concurrency
            # network
            # consume from a queue in bulk to use buffering performance benefit

            for client_id in range(0, len(self.iqs)):
                if not active_qs[client_id]: continue

                iq,oq = self.iqs[client_id], self.oqs[client_id]

                (addr, cmd, rid) = iq.consume()
                #print "del got:",(addr,cmd,rid)

                (isNet, gaddr) = self.__resolveMemory(addr)

                if cmd==READ:
                    d = None
                    if isNet: #TODO go over net
                        d = self.net[gaddr]
                    else:
                        d = self.memory[gaddr]
                    
                    oq.produce((rid, d))
                    #print "del sent:", (rid,d)
                    
                elif cmd==FETCH_INC:
                    d = None
                    if isNet:
                        #TODO go over net
                        d = self.net[gaddr]
                        self.net[gaddr]+=1
                    else:
                        # no sync required
                        d = self.memory[gaddr]
                        self.memory[gaddr]+=1

                    oq.produce((rid, d))
                elif cmd==KILLQ:
                    killQ(client_id)
                else:
                    raise Exception("%d cmd unrecognized")

            




        
