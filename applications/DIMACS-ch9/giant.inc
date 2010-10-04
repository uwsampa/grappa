      integer len, ipoint, loglen, minstep, logstep

      parameter (len = 8*1024*1024, loglen = 3+10+10)
      parameter (minstep = 8*1024, logstep = 3+10)

c len should be a power of 2
c loglen is log_2 of len
c logstep is log_2 of minstep
c minstep is len/(32p), could be smaller if len is very large

      integer*4 ix(len), am(55, 55, logstep:loglen-1)
      common /iran/ ipoint, ix, am
      integer init(55)
      common /initrand/ init
