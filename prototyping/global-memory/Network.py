import numpy
import threading
import coros_stack
from mpi4py import MPI

# for now using same tag
REQTAG = 11
RESPTAG = 13
KILLTAG = 4

class Network:

    def __init__(self, comm, otherhosts):
        self.comm = comm
        self.LCrecvreqs = []

        self.RCrecvreqs = []
        for h in otherhosts:
            N = numpy.array([0,0,0], 'l')  #address,cmd,rid
            r = self.comm.Irecv([N, MPI.LONG], source=h, tag=REQTAG)
            self.RCrecvreqs.append((r,N,h)) #(Request,buf,host)




    # send nonblocking request for commands only receiving data from global mem
    def LCsendRead(self, desti, datatuple):   #TODO have delegate form mrid from core+rid
        N = numpy.array(datatuple, 'l')
        print "%d id:%x sent request to %d"%(self.comm.Get_rank(), datatuple[2], desti)
        r = self.comm.Isend([N, MPI.LONG], dest=desti, tag=REQTAG)

        # set up the corresponding recv
        Nr = numpy.array([0,0], 'l') 
        rr = self.comm.Irecv([Nr, MPI.LONG], source=desti, tag=RESPTAG)
        self.LCrecvreqs.append((rr,Nr))


    # poll for a returned response
    def LCgetDone(self): #Testsome could be used, but seems like impl only returns one item anyway?
         (index, someDone) = MPI.Request.Testany(map(lambda x: x[0], self.LCrecvreqs))
         if someDone and (index>=0):  # not sure about this Testany implementation: returns True with a negative index
             (r,N) = self.LCrecvreqs.pop(index)
             print "%d got response with payload:%s"%(self.comm.Get_rank(), str(N))
             return tuple(N)
         else:
             return None 



    def RCsendResponse(self, desti, datatuple):
        N = numpy.array(datatuple, 'l')
        print "%d sent response to %d with payload %s"%(self.comm.Get_rank(), desti, str(N))
        self.comm.Isend([N, MPI.LONG], dest=desti, tag=RESPTAG)


    def RCgetReq(self):
        (index, someDone) = MPI.Request.Testany(map(lambda x: x[0], self.RCrecvreqs))
        if someDone:
            (r, N, h) = self.RCrecvreqs[index]
            reqtuple = (tuple(N), h)
            print "%d id:%x got request from %d"%(self.comm.Get_rank(), reqtuple[0][1], h)

            # start new recv request to replace finished one
            Nnew = numpy.array([0,0,0], 'l')
            rnew = self.comm.Irecv([Nnew, MPI.LONG], source=h, tag=REQTAG)
            self.RCrecvreqs[index] = (rnew, Nnew, h)

            return reqtuple
        else:
            return None
            

