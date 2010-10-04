c
c This is a "giant-stepping" version of the classical
c lagged-fibonacci generator described in Knuth.
c Fills a vector V with N 32-bit integers.
c
      subroutine giant(n, v)
      implicit none
      integer n
      integer*4 v(n)
      include "giant.inc"
      integer ii, jj, i, nn

      if (ipoint .eq. 0) then  ! need to initialize
         call rand_buildam()  ! compute adjacency matrices
         call setup()        ! define first 55 values
         call rand_rtail    ! fill in the rest
         ipoint = 10000    ! skip a bit, since initial values may be no good
      endif
      nn = n
      ii = 0
      do while (ii .lt. nn)
         if (ipoint .gt. len) then
            call rand_run
            ipoint = 1
         endif

         jj = min(len + 1 - ipoint, nn - ii)
c$mta assert nodep v
         do i = 1, jj
            v(ii + i) = ix(ipoint + i - 1)
         enddo

         ipoint = ipoint + jj
         ii = ii + jj
      enddo
      end



c
c     this routine generates the next len entries in array ix
c
      subroutine rand_run
      implicit none
      include "giant.inc"
      integer i

c$mta loop serial
      do i = 1, 24
         ix(i) = ix(i + len - 24) + ix(i + len - 55)
      enddo

c$mta loop serial
c$mta assert nodep ix
      do i = 25, 55
         ix(i) = ix(i - 24)       + ix(i + len - 55)
      enddo

      call rand_rtail
      end


c
c     this routine generates the tail end
c     remaining len-56 entries in array ix
c
      subroutine rand_rtail
      implicit none
      include "giant.inc"
      integer i, j, step, s, ii, jj

      step = len / 2
      do i = loglen - 1, logstep, -1
c$mta assert nodep ix
         do j = step, len, 2*step
c$mta assert nodep ix
            do ii = 1, 55
               s = 0
               do jj = 1, 55
                  s = s + ix(j - step + jj)*am(ii, jj, i)
               enddo
               ix(j + ii) = s
            enddo
         enddo
         step = step / 2
      enddo

c$mta assert nodep ix
      do i = 1, len, minstep
c$mta loop serial
c$mta assert nodep ix
         do j = i + 55, i + minstep - 1
            ix(j) = ix(j - 24) + ix(j - 55)
         enddo
      enddo
      end


      subroutine rand_buildam()
      implicit none
      include "giant.inc"
      integer i, j
      integer*4 b(55, 55), c(55, 55)

      do i = 1, 55
         do j = 1, 55
            b(i, j) = 0
         enddo
      enddo

      do i = 1, 54
         b(i, i + 1) = 1
      enddo
      b(55, 1) = 1
      b(55, 32) = 1

      do i = 1, logstep - 1
         call rand_square(b, c)
         call rand_copy(c, b)
      enddo

      call rand_square(b, am(1, 1, logstep))
      do i = logstep + 1, loglen - 1
         call rand_square(am(1, 1, i - 1), am(1, 1, i))
      enddo
      end


      subroutine rand_square(b, c)                 ! b*b => c
      implicit none
      integer*4  b(55, 55), c(55, 55)
      integer s, i, j, k

      do i = 1, 55
         do j = 1, 55
            s = 0
            do k = 1, 55
               s = s + b(i, k)*b(k, j)
            enddo
            c(i, j) = s
         enddo
      enddo
      end


      subroutine rand_copy(c, b)                        ! c => b
      implicit none
      integer*4 c(55, 55), b(55, 55)
      integer i, j
      do i = 1, 55
         do j = 1, 55
            b(i, j) = c(i, j)
         enddo
      enddo
      end



      block data randomdata
      implicit none
      include "giant.inc"
      data init /
     *     -662003454,
     *     1953210302,
     *     2002600785,
     *     1655860747,
     *     433832350,
     *     438263339,
     *     1703199216,
     *     573714703,
     *     275680090,
     *     1583583926,
     *     1965443329,
     *     1636505764,
     *     1011597961,
     *     1315461275,
     *     1069844923,
     *     -984724381,
     *     1498661368,
     *     1968401469,
     *     1300134328,
     *     306246424,
     *     1884751139,
     *     400011959,
     *     1363416242,
     *     1938301951,
     *     336563455,
     *     1215013716,
     *     593537720,
     *     1134594751,
     *     1347315106,
     *     1762719719,
     *     774132919,
     *     1482824219,
     *     1746481261,
     *     1479089144,
     *     1169907872,
     *     485614972,
     *     382361684,
     *     1005464791,
     *     1859353594,
     *     1237390611,
     *     1902249868,
     *     -1944527110,
     *     1008956512,
     *     1953439621,
     *     1640123668,
     *     478464352,
     *     1272929208,
     *     392083579,
     *     1117546963,
     *     1771058762,
     *     1509024645,
     *     1047146551,
     *     994817018,
     *     1489680608,
     *     1506717157 /
      end


      subroutine setup()
      implicit none
      include "giant.inc"
      integer i
      do i = 1, 55
         ix(i) = init(i)
      enddo
      end
