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
            r = Irecv([N, MPI.LONG], source=h, tag=REQTAG)
            self.RCrecvreqs.append((r,N,h)) #(Request,buf,host)




    # send nonblocking request for commands only receiving data from global mem
    def LCsendRead(self, desti, datatuple):   #TODO have delegate form mrid from core+rid
        N = numpy.array(datatuple, 'l')
        r = comm.Isend([N, MPI.LONG], dest=desti, tag=REQTAG)

        # set up the corresponding recv
        Nr = numpy.array([0,0], 'l') 
        rr = comm.Irecv([Nr, MPI.LONG], source=desti, tag=RESPTAG)
        self.LCrecvreqs.append((rr,N))


    # poll for a returned response
    def LCgetDone(self): #Testsome could be used, but seems like impl only returns one item anyway?
         (index, someDone) = MPI.Request.Testany(map(lambda x: x[0], self.LCrecvreqs))
         if someDone:
             (r,N) = LCrecvreqs.pop(index)
             resptuple = tuple(N)
         else:
             return None 



    def RCsendResponse(self, desti, datatuple):
        N = numpy.array(datatuple, 'l')
        comm.Isend([N, MPI.LONG], dest=desi, tag=RESPTAG)


    def RCgetReq(self):
        (index, someDone) = MPI.Request.Testany(map(lambda x: x[0], self.RCrecvreqs))
        if someDone:
            (r, N, h) = RCrecvreqs[index]
            reqtuple = tuple(N)

            # start new recv request to replace finished one
            Nnew = numpy.array([0,0,0], 'l')
            rnew = Irecv([Nnew, MPI.LONG], source=h, tag=REQTAG)
            RCrecvreqs[index] = (rnew, Nnew, h)

            return reqtuple
        else:
            return None
            

