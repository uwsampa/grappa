#if HAVE_CONFIG_H
#   include "config.fh"
#endif

#define TEST_MULT 1
#if TEST_MULT
#   define LMAX 972
#else
#   define LMAX 943031
#endif
#define MAXVEC 2000000

      program main
      implicit none
#include "mafdecls.fh"
#include "global.fh"
#include "testutil.fh"
c#include "tcgmsg.fh"
      integer heap, stack, fudge, ma_heap
      integer nmax, ndim, nprocs, me
      integer g_a_data, g_a_i, g_a_j, g_a_sbit, g_tmp, isize
      integer g_b, g_c, g_tmp2
      integer i, j, k, one, icnt
      integer lo, hi, chunk, kp1, ld
      GA_ACCESS_INDEX_TYPE i8, idx, id_tmp
      integer g_chk(MAXVEC)
      double precision d_chk(MAXVEC)
      double precision t_beg, t_crt, t_mv, t_gat, t_elem, t_pack
      double precision t_shft, t_scnad
#if TEST_MULT
      double precision ddata(MAXVEC),bb(LMAX),cc(LMAX),gc(LMAX)
      integer ii(LMAX+1),jj(MAXVEC),sbit(MAXVEC)
      double precision rdot
#endif
      logical status
      integer idim, jdim, rmin, rmax
      parameter (heap=2000000, fudge=100, stack=2000000)
      integer i_chk(MAXVEC)
c
c***  Intitialize a message passing library
c
#include "mp3.fh"
c
c***  Initialize GA
c
c     There are 2 choices: ga_initialize or ga_initialize_ltd.
c     In the first case, there is no explicit limit on memory usage.
c     In the second, user can set limit (per processor) in bytes.
c
      call ga_initialize()
      me = ga_nodeid()
      nprocs = ga_nnodes()
      one = 1
c     we can also use GA_set_memory_limit BEFORE first ga_create call
c
      ma_heap = heap + fudge 
c      call GA_set_memory_limit(util_mdtob(ma_heap))
c
      if(ga_nodeid().eq.0)then
        print *
        print *,'GA initialized '
        print *
#if TEST_MULT
        print *
        print *,'Comparing distributed multiply with serial multiply'
        print *
#endif
        call ffflush(6)
      endif
c
c***  Initialize the MA package
c     MA must be initialized before any global array is allocated
c
      status = ma_init(MT_DBL, stack, ma_heap)
      if (.not. status) call ga_error('ma_init failed',-1) 
c
c    create a sparss LMAX x LMAX matrix and two vectors of length
c    LMAX. The matrix is stored in compressed row format.
c    One of the vectors is filled with random data and the other
c    is filled with zeros.
c
      idim = LMAX
      jdim = LMAX
      if (me.eq.0) then
        print *
        print *, 'Nominal dimension of matrix: ',idim
        print *
      endif
      rmin = 5
      rmax = 10
      t_beg = util_timer()
      call create_rand_sprs_mat(idim,jdim,rmin,rmax,
     +                          g_a_data,g_a_i,g_a_j,g_a_sbit,isize)
      t_crt = util_timer() - t_beg
      if (me.eq.0) then
        print *
        print *, 'Number of non-zero elements in compressed matrix: ',
     +           isize
        print *
      endif
#if TEST_MULT
      if (me.eq.0) then
        call nga_get(g_a_data,one,isize,ddata,one)
        call nga_get(g_a_i,one,idim+1,ii,one)
        call nga_get(g_a_j,one,isize,jj,one)
      endif
      call ga_sync
#endif
      g_tmp = ga_create_handle()
      call ga_set_data(g_tmp,one,isize,MT_DBL)
      status = ga_allocate(g_tmp)
c
      g_tmp2 = ga_create_handle()
      call ga_set_data(g_tmp2,one,isize,MT_DBL)
      status = ga_allocate(g_tmp2)
c
      g_b = ga_create_handle()
      call ga_set_data(g_b,one,idim,MT_DBL)
      status = ga_allocate(g_b)
c
c   fill g_b with random values
c
      call nga_distribution(g_b,me,lo,hi)
      call nga_access(g_b,lo,hi,idx,ld)
      ld = hi-lo
      do i8 = 0, ld
        dbl_mb(i8+idx) = drand(0)
      end do
      call nga_release(g_b,lo,hi)
#if TEST_MULT
      call ga_sync
      if (me.eq.0) then
        call nga_get(g_b,one,idim,bb,one)
      endif
      call ga_sync
#endif
c
      g_c = ga_create_handle()
      call ga_set_data(g_c,one,idim,MT_DBL)
      status = ga_allocate(g_c)
      call ga_zero(g_c)
c
c   Find out what section of j-index vector I own and use
c   this information to get a pointer to to the corresponding
c   parts of the j-index vector and g_tmp array.
c
      t_beg = util_timer()
      call nga_distribution(g_a_j,me,lo,hi)
      call nga_access(g_a_j,lo,hi,idx,ld)
      call nga_access(g_tmp,lo,hi,id_tmp,ld)
c
c   scatter elements of g_b into a temporary array that is the
c   same size as g_a_data.
c
      ld = hi-lo+1
      call nga_gather(g_b,dbl_mb(id_tmp),int_mb(idx),ld)
      call ga_sync
      call nga_release(g_a_j,lo,hi)
      call nga_release(g_tmp,lo,hi)
      t_gat = util_timer() - t_beg
c
      t_beg = util_timer()
      call ga_elem_multiply(g_tmp,g_a_data,g_tmp2)
      t_elem = util_timer() - t_beg
c      if (me.eq.0) then
c        do i = 0, 1
c          call nga_distribution(g_a_sbit,i,lo,hi)
c          call nga_get(g_a_sbit,lo,hi,i_chk,one)
c          write(6,'(25i5)') (i_chk(j),j=1,hi-lo+1)
c        end do
c      endif
      t_beg = util_timer()
      call ga_scan_add(g_tmp2,g_tmp,g_a_sbit,one,isize,0)
      t_scnad = util_timer() - t_beg
c      if (me.eq.0) then
c        do i = 0, 1
c          call nga_distribution(g_tmp,i,lo,hi)
c          call nga_get(g_tmp,lo,hi,d_chk,one)
c          write(6,'(10f8.2)') (d_chk(j),j=1,hi-lo+1)
c        end do
c      endif
#if TEST_MULT
      if (me.eq.0) then
        call nga_get(g_a_sbit,one,isize,sbit,one)
        icnt = 0
        if (isize.gt.MAXVEC) call ga_error('MAXVEC exceeded in sbit',
     +                                     isize) 
        do i = 1, isize
          if (sbit(i).eq.1) then
            icnt = icnt + 1
          endif
        end do
      endif
#endif
      t_beg = util_timer()
      call shift_minus(g_a_sbit,isize)
      t_shft = util_timer() - t_beg
!
      t_beg = util_timer()
      call ga_pack(g_tmp,g_c,g_a_sbit,one,isize,icnt)
      t_pack = util_timer() - t_beg
#if TEST_MULT
      if (me.eq.0) then
        call nga_get(g_c,one,idim,gc,one)
        do i = 1, idim
          cc(i) = 0.0d00
          do j = ii(i), ii(i+1)-1
            cc(i) = cc(i) + ddata(j)*bb(jj(j))
          end do
          gc(i) = gc(i) - cc(i)
        end do
        rdot = 0.0d00
        do i = 1, idim
          rdot = rdot + gc(i)**2
        end do
        write(6,*) 'Difference between parallel and serial results: ',
     +             rdot
      endif
#endif
c
      t_mv = t_gat + t_scnad + t_elem + t_shft + t_pack
      call ga_dgop(1,t_crt,1,'+')
      call ga_dgop(2,t_gat,1,'+')
      call ga_dgop(3,t_scnad,1,'+')
      call ga_dgop(4,t_elem,1,'+')
      call ga_dgop(5,t_shft,1,'+')
      call ga_dgop(6,t_pack,1,'+')
      call ga_dgop(7,t_mv,1,'+')
c
      if (me.eq.0) then
        write(6,100) t_crt/dble(nprocs)
        write(6,200) t_gat/dble(nprocs)
        write(6,300) t_scnad/dble(nprocs)
        write(6,400) t_elem/dble(nprocs)
        write(6,500) t_shft/dble(nprocs)
        write(6,600) t_pack/dble(nprocs)
        write(6,700) t_mv/dble(nprocs)
  100   format('Time to create sparse matrix:                  ',f12.4)
  200   format('Time to expand dense vector using gather:      ',f12.4)
  300   format('Time in segmented scan-add:                    ',f12.4)
  400   format('Time for element-wise multiplication:          ',f12.4)
  500   format('Time to shift bit-mask vector:                 ',f12.4)
  600   format('Time to pack result vector:                    ',f12.4)
  700   format('Total time for sparse matrix-vector multiply:  ',f12.4)
      endif
c
      call ga_terminate()
c
c***  Tidy up after message-passing library
c
      call MP_FINALIZE()

c
      stop
      end
c
c   create a random sparse matrix in compressed row form
c
      subroutine create_rand_sprs_mat(idim,jdim,rmin,rmax,
     +                                g_data,g_i,g_j,g_mask,isize)
c
c   idim: i-dimension of array
c   jdim: j-dimension of array
c   rmin, rmax: minimum and maximum number of row elements in array
c   g_data: global array of values
c   g_j: global array containing j indices
c   g_i: global array containing starting location of each row in g_j
c   g_mask: global array of 1s and 0s containing a 1 at the start of
c           every new row in g_data
c   isize: total size of compressed matrix
c
      implicit none
#include "mafdecls.fh"
#include "global.fh"
#include "testutil.fh"
c#include "tcgmsg.fh"
      integer idim, jdim, rmin, rmax, g_data, g_i, g_j, g_tmp
      integer g_sbit, g_mask, isize
      integer nprocs, me, imin, imax, icnt, jmin, jmax, ntot
      double precision rval(MAXVEC), x, dr
      integer jval(MAXVEC), inc(MAXVEC), ones(MAXVEC)
      integer jdx(MAXVEC), i, j, k, itmp, itot, one, lo, hi, ld
      logical status
      integer i_chk(1000000)
      double precision d_chk(1000000)
      GA_ACCESS_INDEX_TYPE idx
c
      me = ga_nodeid()
      nprocs = ga_nnodes()
      x = drand(12345+me)
      dr = dble(rmax-rmin)
      one = 1
c
c  figure out min and max indices of rows that I am responsible for
c
      imin = nint(dble(me)*(dble(idim)/dble(nprocs))) + 1
      imax = nint(dble(me+1)*(dble(idim)/dble(nprocs)))
      if (me.eq.0) then
        write(6,*) 
        write(6,*) 'Minimum number of entries per row: ',
     +                     rmin
        write(6,*) 
        write(6,*) 
        write(6,*) 'Maximum number of entries per row: ',
     +                     rmin+nint(dr)
        write(6,*) 
      endif
      icnt = 0
      do i = imin, imax
        jmax = rmin + nint(dr*drand(0)) 
        do j = 1, jmax
          jdx(j) = int(dble(jdim)*drand(0))+1
          if (jdx(j).gt.jdim) jdx(j) = jdim
        end do
c
c  sort j indices
c
        do j = 1, jmax
          do k = j+1, jmax
            if (jdx(j).gt.jdx(k)) then
              itmp = jdx(j)
              jdx(j) = jdx(k)
              jdx(k) = itmp
            endif
          end do
        end do
c
c  create array elements
c
        inc(i-imin+1) = jmax
        do j = 1, jmax
          icnt = icnt + 1
          rval(icnt) = drand(0)
          jval(icnt) = jdx(j)
        end do
      end do
      itot = icnt
      isize = 0
      do i = 1, imax-imin+1
        isize = isize + inc(i)
      end do
      if (isize.gt.MAXVEC)
     +   call ga_error('ISIZE exceeds MAXVEC in local arrays ',isize)
c
c  create global arrays to hold sparse matrix
c
      call ga_igop(1,itot,1,'+')
      isize = itot
c      write(6,*) me,' (create_rand) isize: ',isize
c
      g_data = ga_create_handle();
      call ga_set_data(g_data,one,itot,MT_DBL)
      status = ga_allocate(g_data)
c
      g_j = ga_create_handle();
      call ga_set_data(g_j,one,itot,MT_INT)
      status = ga_allocate(g_j)
c
      g_i = ga_create_handle();
      call ga_set_data(g_i,one,idim+1,MT_INT)
      status = ga_allocate(g_i)
      status = ga_duplicate(g_i, g_tmp, 'temp array')
c
      g_sbit = ga_create_handle();
      call ga_set_data(g_sbit,one,idim+1,MT_INT)
      status = ga_allocate(g_sbit)
c
      call ga_zero(g_sbit)
      if (me.eq.0) then
        call nga_put(g_sbit,one,one,one,one)
      endif
      call ga_sync
c
c  create offset vector
c
      lo = imin+1
      hi = imax+1
      call ga_zero(g_tmp)
      if (me.eq.0) then
        call nga_put(g_tmp,one,one,one,one)
      endif
      call nga_put(g_tmp,lo,hi,inc,one)
      call ga_sync
c      if (me.eq.0) then
c        do i = 0, nprocs-1
c          call nga_distribution(g_tmp,i,lo,hi)
c          call nga_get(g_tmp,lo,hi,i_chk,one)
c          write(6,'(25i5)') (i_chk(j),j=1,hi-lo+1)
c        end do
c      endif
      itot = idim+1
      call ga_scan_add(g_tmp,g_i,g_sbit,one,itot,0)
      call ga_sync
c      if (me.eq.0) then
c        do i = 0, nprocs-1
c          call nga_distribution(g_i,i,lo,hi)
c          call nga_get(g_i,lo,hi,i_chk,one)
c          write(6,'(25i5)') (i_chk(j),j=1,hi-lo+1)
c        end do
c      endif

      status = ga_destroy(g_tmp)
      status = ga_destroy(g_sbit)
c
c  create mask vector
c
      g_mask = ga_create_handle();
      call ga_set_data(g_mask,one,isize,MT_INT)
      status = ga_allocate(g_mask)
      call ga_zero(g_mask)
!
      call nga_distribution(g_i,me,lo,hi)
      call nga_access(g_i,lo,hi,idx,ld)
      ntot = hi - lo + 1
      if (ntot.gt.MAXVEC)
     +   call ga_error('NTOT exceeds MAXVEC in bit vector ',ntot)
!
!  decrease count by one on last processor because this
!  element doesn't exist
!
      if (me.eq.nprocs-1) then
        ntot = ntot - 1
      endif
      do i = 1, ntot
        ones(i) = 1
      end do
      call nga_scatter(g_mask,ones,int_mb(idx),ntot)
      call ga_sync
      call nga_release(g_i,lo,hi)
c      if (me.eq.0) then
c        do i = 0, 1
c          call nga_distribution(g_mask,i,lo,hi)
c          call nga_get(g_mask,lo,hi,i_chk,one)
c          write(6,'(25i5)') (i_chk(j),j=1,hi-lo+1)
c        end do
c      endif
c
c store values of j in g_j and data values in g_data
c
      call nga_get(g_i,imin,imin,jmin,one)
      call nga_get(g_i,imax+1,imax+1,jmax,one)
      jmax = jmax - 1
      call nga_put(g_j,jmin,jmax,jval,one)
      call nga_put(g_data,jmin,jmax,rval,one)
      call ga_sync
      return
      end
c
c   shift all the elements in a linear global array one element to
c   the left. Wrap elements around end if necessary
c
      subroutine shift_minus(g_a, isize)
      implicit none
#include "mafdecls.fh"
#include "global.fh"
#include "testutil.fh"
c#include "tcgmsg.fh"
      integer g_a, isize
      integer lo, hi, ld, me, itmp
      integer isav
      GA_ACCESS_INDEX_TYPE idx, ii
      me = ga_nodeid()
      call ga_sync
      call nga_distribution(g_a,me,lo,hi)
      call nga_access(g_a,lo,hi,idx,ld)
      ld = hi-lo
      isav = int_mb(idx)
      do ii = 1, ld
        int_mb(idx + ii - 1) = int_mb(idx + ii)
      end do
      if (lo.eq.1) then
        itmp = isize
      else
        itmp = lo-1
      endif
      call nga_release(g_a,lo,hi)
      call ga_sync
      call nga_put(g_a,itmp,itmp,isav,ld)
      call ga_sync
      return
      end
