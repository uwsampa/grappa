C--- 8 June 1999 Chris S.  modified to read a file with the first argument
C--- being a generator type
C--- added 'integer gentype' 
C--- added 'Reading in a generator type' and   'read *, gentype'
#define PARAM 0
#define TIMING_TRIAL_SIZE 1000000

#ifdef POINTER_SIZE
#if POINTER_SIZE == 8
#define SPRNG_POINTER integer*8
#else
#define SPRNG_POINTER integer*4
#endif
#else
#define SPRNG_POINTER integer*4
#endif

        program test_generator

	implicit none
	integer gentype
        external finit_rng, fget_rn_int, fget_rn_flt, fget_rn_dbl
        external fcpu_t

        SPRNG_POINTER finit_rng
        real*8 fget_rn_dbl, fcpu_t, tempd
        real*4 fget_rn_flt, tempf
        integer fget_rn_int, tempi

        integer i
        SPRNG_POINTER gen
        real*8 temp1, temp2, temp3, temp4
        real*8 temp_mult
C--- Reading in a generator type
        read *, gentype

        temp_mult =  TIMING_TRIAL_SIZE/1.0e6
        gen = finit_rng(gentype,0,1,0,PARAM)
	
        temp1 = fcpu_t()

        do 100 i = 1,TIMING_TRIAL_SIZE
           tempi = fget_rn_int(gen)
 100    continue
  
        temp2 = fcpu_t()
  
        do 200 i = 1,TIMING_TRIAL_SIZE
           tempf = fget_rn_flt(gen)
 200    continue

        temp3 = fcpu_t()
  

        do 300 i = 1,TIMING_TRIAL_SIZE
           tempd = fget_rn_dbl(gen)
 300    continue

        temp4 = fcpu_t()

        if( temp2-temp1 .lt. 1.0e-15 ) then
           print *, 'Timing information not available or nor accurate'
           stop
        end if

        if( temp3-temp2 .lt. 1.0e-15 ) then
           print *, 'Timing information not available or nor accurate'
           stop
        end if

         if(temp4-temp3 .lt. 1.0e-15 ) then
           print *, 'Timing information not available or not accurate'
           stop
        end if

        temp1 = temp_mult/(temp2-temp1)
        temp2 = temp_mult/(temp3-temp2)
        temp3 = temp_mult/(temp4-temp3)

        print *, 'User + System time Information'
        print *, '(Note: MRS = Million Random Numbers Per Second)'
        write(6,500) temp1
        write(6,501) temp2
        write(6,502) temp3

 500    format('      Integer generator: ',  f7.3, ' MRS')
 501    format('      real*4  generator: ',  f7.3, ' MRS')
 502    format('      real*8 generator:  ',  f7.3, ' MRS')

        end

