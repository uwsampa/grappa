!  $Date: 2010/03/10 20:39:37 $
!  $Revision: 1.5 $
!  $RCSfile: ftilt.f90,v $
!  $State: Stab $:


! The tilt subroutine takes a 64 word array of 64-bit words as
! an argument. It
! treats the array as a 64x64 bit matrix and performs a transpose.
! For reasonable performance, the calls to tiltm must be inlined.

SUBROUTINE tiltm( a, b, s, m)
  IMPLICIT NONE
  INTEGER(8), INTENT(INOUT) :: a, b
  INTEGER(8), INTENT(IN) :: s, m
  INTEGER(8) z
!  z = (a .xor. ISHFT(b,-s)) .and. m
  z = iand(ieor(a ,ISHFT(b,-s)), m)
  a = ieor(a,z)
  b = ieor(b,ISHFT(z,s))

END SUBROUTINE tiltm

SUBROUTINE tilt( p )
  IMPLICIT NONE
  INTEGER(8), INTENT(INOUT) :: p(0:63)
  INTEGER(8) q(0:63), a, b, c, d, e, f, g, h, i, j
  INTEGER(8), PARAMETER :: &
!       M0 =     6148914691236517205, &   ! z'5555555555555555'
       !M1 =     3689348814741910323, &   ! z'3333333333333333'
       !M2 =     1085102592571150095, &   ! z'0f0f0f0f0f0f0f0f'
       !M3 =       71777214294589695, &   ! z'00ff00ff00ff00ff'
       !M4 =         281470681808895, &   ! z'0000ffff0000ffff
       !M5 =              4294967295      ! z'00000000ffffffff'

       M0 =     z'5555555555555555', &
       M1 =     z'3333333333333333', &
       M2 =     z'0f0f0f0f0f0f0f0f', &
       M3 =     z'00ff00ff00ff00ff', &
       M4 =     z'0000ffff0000ffff', &
       M5 =     z'00000000ffffffff'


  DO i = 0, 7
     j = i*8
     a = p(0+i);       b = p(8+i);       c = p(16+i);      d = p(24+i)
     CALL tiltm(a,b,8_8,M3);  CALL tiltm(c,d,8_8,M3)
     CALL tiltm(a,c,16_8,M4); CALL tiltm(b,d,16_8,M4)

     e = p(32+i);      f = p(40+i);      g = p(48+i);      h = p(56+i)
     CALL tiltm(e,f,8_8,M3);  CALL tiltm(g,h,8_8,M3)
     CALL tiltm(e,g,16_8,M4); CALL tiltm(f,h,16_8,M4)

     CALL tiltm(a,e,32_8,M5); CALL tiltm(b,f,32_8,M5)
     CALL tiltm(c,g,32_8,M5); CALL tiltm(d,h,32_8,M5)
     q( 0+j) = a;      q( 1+j) = b;      q( 2+j) = c;      q( 3+j) = d
     q( 4+j) = e;      q( 5+j) = f;      q( 6+j) = g;      q( 7+j) = h
  ENDDO

  DO i =  0, 7
    j = i*8
    a = q( 0+i);      b = q( 8+i);      c = q(16+i);      d = q(24+i)
    CALL tiltm(a,b,1_8,M0);  CALL tiltm(c,d,1_8,M0)
    CALL tiltm(a,c,2_8,M1);  CALL tiltm(b,d,2_8,M1)

    e = q(32+i);      f = q(40+i);      g = q(48+i);      h = q(56+i)
    CALL tiltm(e,f,1_8,M0);  CALL tiltm(g,h,1_8,M0)
    CALL tiltm(e,g,2_8,M1);  CALL tiltm(f,h,2_8,M1)

    CALL tiltm(a,e,4_8,M2);  CALL tiltm(b,f,4_8,M2)
    CALL tiltm(c,g,4_8,M2);  CALL tiltm(d,h,4_8,M2)
    p( 0+j) = a;      p( 1+j) = b;      p( 2+j) = c;      p( 3+j) = d
    p( 4+j) = e;      p( 5+j) = f;      p( 6+j) = g;      p( 7+j) = h
  ENDDO

END SUBROUTINE tilt

