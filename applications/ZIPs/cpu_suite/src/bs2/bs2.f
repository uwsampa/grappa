c  CVS Info
c  $Date: 2010/01/07 17:39:22 $
c  $Revision: 1.5 $
c  $RCSfile: bs2.f,v $
c  $State: Stab $:
c
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c------------------------------------------------------------------c
cc x**2 mod 2^n-1
c------------------------------------------------------------------c
      subroutine b_s_x2(x,y,nbits)
      implicit integer(a-y)
c     nwords is tunable, MUST equal NWORDS in the C driver
      parameter(nwords=4)
      dimension
     +      x(0:nwords-1, 0:nbits-1)
     +     ,y(0:nwords-1, 0:nbits-1)
      dimension
     +     c(0:nwords-1)

C--CVS variable declaration
      TYPE CVS
      sequence
      character( 160 )     string
      integer              stringend
      END TYPE CVS
C--CVS initilaize variables
      TYPE( CVS ),save :: CVS_INFO =
     $  CVS("BMARKGRP $Date: 2010/01/07 17:39:22 $ " //
     $      "Revision:  $" //
     $      "$RCSfile: bs2.f,v $ $Name:  $", 0)


ccc initialize the sum to: x, if its low bit is 1, 0 otherwise.
ccc  (we could initialize sum to 0, then do k = 0,nbits-1 below)
      do j = 0,nbits-1
         do i = 0,nwords-1
!            y(i,j) = x(i,j) .and. x(i,0)
	     y(i,j) = iand(x(i,j),x(i,0))
         enddo
      enddo

ccc initialize the carry to 0
      do i = 0,nwords-1
         c(i) = 0
      enddo

ccc add cls(x,k) + carry to y , but use 0 if x<1,k> = 0
      do k = 1,nbits-1

ccc need n-k when doing cls
         nmk = nbits - k

ccc   loop on bits. low bit is bit0

         jj = nbits-k  ! will always be j+nbits-k mod nbits

         do j = 0,nbits-1

ccc get index for x <<< k :  (j + n-k) mod n

c            jj = (j + nmk)
c            if(jj.ge.nbits)jj=jj-nbits
            do i = 0,nwords-1

ccc we add x <<< k   only if the k-th bit of x is not 0
               tempy = y(i,j)
!               temp1 = tempy .xor. ( x(i,jj) .and. x(i,k) )
!               temp2 = tempy .xor. c(i)
               temp1 = ieor(tempy ,iand( x(i,jj),x(i,k) ))
               temp2 = ieor(tempy,c(i))


ccc new sum bit is y .xor. x .xor. c
!               y(i,j) = temp1 .xor. c(i)

		y(i,j) = ieor(temp1,c(i))

ccc new carry bit is y .xor. [ (y.xor.x) .and. (y.xor.c) ]
!               c(i) = tempy .xor. (temp1 .and. temp2)
		c(i) = ieor(tempy,(iand(temp1,temp2)))

            enddo               ! i

            jj = jj+1
            if(jj.eq.nbits) jj = 0

         enddo                  ! j
ccccccc
ccc add in final carry
ccccccc
         do j = 0,nbits-1

            do i = 0,nwords-1
               tempy = y(i,j)

ccc new sum bit is y .xor. c
!               y(i,j) = tempy .xor. c(i)
               y(i,j) = ieor(tempy,c(i))

ccc new carry bit is y .and.c
!               c(i) = tempy .and. c(i)
		c(i) = iand(tempy,c(i))

            enddo               ! i

         enddo                  ! j

      enddo                     ! k

      end
cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
