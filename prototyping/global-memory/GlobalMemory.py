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
    LocalBits = 3
    InitMemory = {0:1, 1:2, 2:4, 3:8, 4:16, 5:32, 6:64, 7:128}


    def __isLocal(self, addr):
        # TODO mapping
        return self.__getHost(addr) == self.hostid

    def __getHost(self, addr):
        return (addr>>48)&0xffff

    def __init__(self, outqs, inqs, hostid, otherHosts):
        self.oqs = outqs
        self.iqs = inqs

        self.memory = dict(Delegate.InitMemory)
        self.net = dict(Delegate.NetMemory)

        self.otherHosts = otherHosts
        self.hostid = hostid
        self.totalHosts = len(otherHosts)+1

    @classmethod
    def genID(cls, cid, rid):
        # XXX assumes rid < 32 bits
        return ((cid<<32)&0xffffffff00000000) | (rid&0xffffffff)
    
    @classmethod
    def getCoreAndID(cls, mrid): # get rid
       core = (mrid>>32)&0xffffffff
       rid = mrid&0xffffffff
       return (core, rid)

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
            

            '''handle local requests'''
            for client_id in range(0, len(self.iqs)):
                if not active_qs[client_id]: continue

                iq,oq = self.iqs[client_id], self.oqs[client_id]

                (addr, cmd, rid) = iq.consume()
                #print "del got:",(addr,cmd,rid)

                isLocal = self.__isLocal(addr)

                if cmd==READ:
                    d = None
                    if isLocal:
                        gaddr = addr&((1<<LocalBits)-1)
                        d = self.memory[gaddr]
                        
                        # return to client
                        oq.produce((rid, d))
                    else:
                        mrid = Delegate.genID(client_id, rid)
                        req = (addr,cmd,mrid)
                        self.network.LCsendRead(self.__getHost(addr), req)
                    
                    #print "del sent:", (rid,d)
                    
                elif cmd==FETCH_INC:
                    d = None
                    if isLocal:
                        gaddr = addr&((1<<LocalBits)-1)
                        d = self.memory[gaddr]
                        self.memory[gaddr]+=1

                        # return to client
                        oq.produce((rid, d))
                    else:
                        mrid = Delegate.genID(client_id, rid)
                        req = (addr,cmd,mrid)
                        self.network.LCsendRead(self.__getHost(addr), req)

                elif cmd==KILLQ:
                    killQ(client_id)
                else:
                    raise Exception("%d cmd from local unrecognized"%(cmd))

            '''satisfy remote-originated requests'''
            r_msg = self.network.RCgetReq()
            if r_msg is not None:
                (r_req,r_host) = r_msg
                (addr, cmd, rid) = r_req

                assert self.__isLocal(addr)  # SIM:make sure it belongs here

                if cmd==READ:
                    gaddr = addr&((1<<LocalBits)-1)
                    d = self.memory[gaddr]
                    r_resp = (rid, d)
                    self.network.RCsendResponse(r_host, r_resp)
                elif cmd==FETCH_INC:
                    gaddr = addr&((1<<LocalBits)-1)
                    d = self.memory[gaddr]
                    self.memory[gaddr]+=1
                    r_resp = (rid, d)
                    self.network.RCsendResponse(r_host, r_resp)
                else:
                    raise Exception("%d cmd from host %d unrecognized"%(cmd))

            '''receive satisfied remote requests'''
            while True:
                r_msg = self.network.LCgetDone()
                if r_msg is not None:
                    (mrid, data) = r_msg
                    (core, rid) = Delegate.getCoreAndID(mrid)

                    # return to client
                    if active_qs[core]:
                        oqs[core].produce((rid, data))
                    else:
                        # drop on floor if queue is closed
                        pass
                else:
                    # no more received for now
                    break


         
