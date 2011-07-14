#if HAVE_CONFIG_H
#   include "config.h"
#endif

/*$Id: global.util.c,v 1.48.6.6 2007-05-18 08:19:23 manoj Exp $*/
/*
 * module: global.util.c
 * author: Jarek Nieplocha
 * last modification: Tue Dec 20 09:41:55 PDT 1994
 *
 * DISCLAIMER
 * 
 * This material was prepared as an account of work sponsored by an
 * agency of the United States Government.  Neither the United States
 * Government nor the United States Department of Energy, nor Battelle,
 * nor any of their employees, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
 * ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,
 * COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT,
 * SOFTWARE, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT
 * INFRINGE PRIVATELY OWNED RIGHTS.
 * 
 * 
 * ACKNOWLEDGMENT
 * 
 * This software and its documentation were produced with United States
 * Government support under Contract Number DE-AC06-76RLO-1830 awarded by
 * the United States Department of Energy.  The United States Government
 * retains a paid-up non-exclusive, irrevocable worldwide license to
 * reproduce, prepare derivative works, perform publicly and display
 * publicly by or for the US Government, including the right to
 * distribute to other US Government contractors.
 */
#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_STRING_H
#   include <string.h>
#endif
#if HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "farg.h"
#include "global.h"
#include "globalp.h"
#include <armci.h> 
#define ARMCI 1

#if defined(SUN)
  void fflush();
#endif


/*\ PRINT g_a[ilo:ihi, jlo:jhi]
\*/
void ga_file_print_patch(file, g_a, ilo, ihi, jlo, jhi, pretty)
        FILE *file;
        Integer *g_a, *ilo, *ihi, *jlo, *jhi, *pretty;
/*
  Pretty = 0 ... spew output out with no formatting
  Pretty = 1 ... format output so that it is readable
*/  
{
#define BUFSIZE 6
#define FLEN 80 
Integer i, j,jj, dim1, dim2, type, jmax, ld=1, bufsize ;
Integer a_grp;
int ibuf[BUFSIZE];
DoublePrecision  dbuf[BUFSIZE];
float fbuf[BUFSIZE]; 
long lbuf[BUFSIZE]; 
long long llbuf[BUFSIZE]; 
char *name;

  a_grp = ga_get_pgroup_(g_a);
  ga_pgroup_sync_(&a_grp);
  gai_check_handle(g_a, "ga_print");
  if(ga_pgroup_nodeid_(&a_grp) == 0){

     ga_inquire_internal_(g_a,  &type, &dim1, &dim2);
/*     name[FLEN-1]='\0';*/
     gai_inquire_name(g_a, &name);
     if (*ilo <= 0 || *ihi > dim1 || *jlo <= 0 || *jhi > dim2){
         fprintf(stderr,"%ld %ld %ld %ld dims: [%ld,%ld]\n", 
                 (long)*ilo,(long)*ihi, (long)*jlo,(long)*jhi,
                 (long)dim1, (long)dim2);
         gai_error(" ga_print: indices out of range ", *g_a);
     }

     fprintf(file,"\n global array: %s[%ld:%ld,%ld:%ld],  handle: %d \n",
             name, (long)*ilo, (long)*ihi, (long)*jlo, (long)*jhi, (int)*g_a);

     bufsize = (type==C_DCPL)? BUFSIZE/2 : BUFSIZE;
     bufsize = (type==C_SCPL)? BUFSIZE/2 : BUFSIZE;


     if (!*pretty) {
       for (i=*ilo; i <*ihi+1; i++){
         for (j=*jlo; j <*jhi+1; j+=bufsize){
           jmax = GA_MIN(j+bufsize-1,*jhi);
           switch(type){
              case C_INT:
                   ga_get_(g_a, &i, &i, &j, &jmax, ibuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj++)
                     fprintf(file," %8d",ibuf[jj]);
                   break;
              case C_DBL:
                   ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj++)
                     fprintf(file," %11.5f",dbuf[jj]);
                   break;
              case C_DCPL:
                   ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj+=2)
                     fprintf(file," %11.5f,%11.5f",dbuf[jj], dbuf[jj+1]);
                   break;
              case C_SCPL:
                   ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj+=2)
                     fprintf(file," %11.5f,%11.5f",dbuf[jj], dbuf[jj+1]);
                   break;
              case C_FLOAT:
                   ga_get_(g_a, &i, &i, &j, &jmax, fbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj++)
                     fprintf(file," %11.5f",fbuf[jj]);
                   break;       
              case C_LONG:
                   ga_get_(g_a, &i, &i, &j, &jmax, lbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj++)
                     fprintf(file," %8ld",lbuf[jj]);
                   break;
              case C_LONGLONG:
                   ga_get_(g_a, &i, &i, &j, &jmax, llbuf, &ld);
                   for(jj=0; jj<(jmax-j+1); jj++)
                     fprintf(file," %8lld",llbuf[jj]);
                   break;
              default: gai_error("ga_print: wrong type",0);
           }
         }
         fprintf(file,"\n");
       }
       fflush(file);

     } else {

        for (j=*jlo; j<*jhi+1; j+=bufsize){
        jmax = GA_MIN(j+bufsize-1,*jhi);

           fprintf(file, "\n"); fprintf(file, "\n");

           /* Print out column headers */

           fprintf(file, "      ");
           switch(type){
              case C_INT:
                   for (jj=j; jj<=jmax; jj++) fprintf(file, "%6ld  ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=jmax; jj++) fprintf(file," -------");
                   break;
              case C_LONG:  
                   for (jj=j; jj<=jmax; jj++) fprintf(file, "%6ld  ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=jmax; jj++) fprintf(file," -------");
                   break;
              case C_LONGLONG:  
                   for (jj=j; jj<=jmax; jj++) fprintf(file, "%6ld  ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=jmax; jj++) fprintf(file," -------");
                   break;
              case C_DCPL:
                   for (jj=j; jj<=jmax; jj++) fprintf(file,"%20ld    ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=2*jmax; jj++) fprintf(file," -----------");
                   break;
              case C_SCPL:
                   for (jj=j; jj<=jmax; jj++) fprintf(file,"%20ld    ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=2*jmax; jj++) fprintf(file," -----------");
                   break;
              case C_DBL:
                   for (jj=j; jj<=jmax; jj++) fprintf(file,"%8ld    ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=jmax; jj++) fprintf(file," -----------");         
              case C_FLOAT:
                   for (jj=j; jj<=jmax; jj++) fprintf(file,"%8ld    ", (long)jj);
                   fprintf(file,"\n      ");
                   for (jj=j; jj<=jmax; jj++) fprintf(file," -----------");
           }
           fprintf(file,"\n");

           for(i=*ilo; i <*ihi+1; i++){
              fprintf(file,"%4ld  ",(long)i);

              switch(type){
                 case C_INT:
                      ga_get_(g_a, &i, &i, &j, &jmax, ibuf, &ld);
                      for(jj=0; jj<(jmax-j+1); jj++)
                        fprintf(file," %8d",ibuf[jj]);
                      break;
                 case C_LONG: 
                      ga_get_(g_a, &i, &i, &j, &jmax,lbuf, &ld);
                      for(jj=0; jj<(jmax-j+1); jj++)
                        fprintf(file," %8ld",lbuf[jj]);
                      break;
                 case C_LONGLONG: 
                      ga_get_(g_a, &i, &i, &j, &jmax,llbuf, &ld);
                      for(jj=0; jj<(jmax-j+1); jj++)
                        fprintf(file," %8lld",llbuf[jj]);
                      break;
                 case C_DBL:
                      ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
                      for(jj=0; jj<(jmax-j+1); jj++)
                        fprintf(file," %11.5f",dbuf[jj]);
                      break;
                 case C_FLOAT:
                      ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
                      for(jj=0; jj<(jmax-j+1); jj++)
                        fprintf(file," %11.5f",fbuf[jj]);
                      break;     
                 case C_DCPL:
	              ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
	              for(jj=0; jj<(jmax-j+1); jj+=2)
	                fprintf(file," %11.5f,%11.5f",dbuf[jj], dbuf[jj+1]);
                      break;
                 case C_SCPL:
	              ga_get_(g_a, &i, &i, &j, &jmax, dbuf, &ld);
	              for(jj=0; jj<(jmax-j+1); jj+=2)
	                fprintf(file," %11.5f,%11.5f",dbuf[jj], dbuf[jj+1]);
                      break;
                 default: gai_error("ga_print: wrong type",0);
	     }
	     fprintf(file,"\n");
         }
         fflush(file);
      }
    }
  }
       
  ga_pgroup_sync_(&a_grp);
}

/*\ PRINT g_a[ilo:ihi, jlo:jhi]
\*/
void FATR ga_print_patch_(g_a, ilo, ihi, jlo, jhi, pretty)
        Integer *g_a, *ilo, *ihi, *jlo, *jhi, *pretty;
/*
  Pretty = 0 ... spew output out with no formatting
  Pretty = 1 ... format output so that it is readable
*/  
{
  ga_file_print_patch(stdout, g_a, ilo, ihi, jlo, jhi, pretty);
}


void FATR ga_print_stats_()
{
int i;
     GAstat_arr = (long*)&GAstat;
#ifdef __crayx1
#ifdef NO_GA_STATS
     printf("\tNOTE:GA stats have been disabled on x1 for some GA calls, to enable them comment the line LIB_DEFINES += -DNO_GA_STATS in global/src/GNUmakefile under the GA directory");
#endif
#endif
     printf("\n                         GA Statistics for process %4d\n",(int)ga_nodeid_());
     printf("                         ------------------------------\n\n");
     printf("       create   destroy   get      put      acc     scatter   gather  read&inc\n");

     printf("calls: ");
     for(i=0;i<8;i++) 
        if(GAstat_arr[i] < 9999) printf("%4ld     ",GAstat_arr[i]);
        else                   printf("%.2e ",(double)GAstat_arr[i]);
     printf("\n");
     if(GAstat.numget==0)GAstat.numget=1;
     if(GAstat.numput==0)GAstat.numput=1;
     if(GAstat.numacc==0)GAstat.numacc=1;
     if(GAstat.numsca==0)GAstat.numsca=1;
     if(GAstat.numgat==0)GAstat.numgat=1;
     printf("number of processes/call %.2e %.2e %.2e %.2e %.2e\n",
                   ((double)GAstat.numget_procs)/(double)GAstat.numget,
                   ((double)GAstat.numput_procs)/(double)GAstat.numput,
                   ((double)GAstat.numacc_procs)/(double)GAstat.numacc,
                   ((double)GAstat.numsca_procs)/(double)GAstat.numsca,
                   ((double)GAstat.numgat_procs)/(double)GAstat.numgat);


     printf("bytes total:             %.2e %.2e %.2e %.2e %.2e %.2e\n",
                   GAbytes.gettot, GAbytes.puttot, GAbytes.acctot,
                   GAbytes.scatot, GAbytes.gattot, GAbytes.rditot);

     printf("bytes remote:            %.2e %.2e %.2e %.2e %.2e %.2e\n",
                   GAbytes.gettot - GAbytes.getloc, 
                   GAbytes.puttot - GAbytes.putloc,
                   GAbytes.acctot - GAbytes.accloc,
                   GAbytes.scatot - GAbytes.scaloc,
                   GAbytes.gattot - GAbytes.gatloc,
                   GAbytes.rditot - GAbytes.rdiloc);

     printf("Max memory consumed for GA by this process: %ld bytes\n",GAstat.maxmem);
     if(GAstat.numser)
        printf("Number of requests serviced: %ld\n",GAstat.numser);
     fflush(stdout);
}

   
 

/*\  ERROR TERMINATION
 *   C-version
\*/
void gai_error(char *string, Integer icode)
{
#ifndef ARMCI
extern void Error();
#endif

#ifdef CRAY_T3D 
#  define FOUT stdout
#else
#  define FOUT stderr
#endif
#define ERR_LEN 400
    int level;
    char error_buffer[ERR_LEN];


    /* print GA names stack */
    sprintf(error_buffer,"%d:", (int)ga_nodeid_());
    for(level = 0; level < GA_stack_size; level++){
       strcat(error_buffer,GA_name_stack[level]);
       strcat(error_buffer,":");
    }
    strcat(error_buffer,string);
    strcat(error_buffer,":");
       
#ifdef ARMCI
    ARMCI_Error(error_buffer,(int)icode);
#else
    ga_clean_resources(); 
    if (ga_nnodes_() > 1) Error(error_buffer, icode);
    else{
      fprintf(FOUT,"%s %ld\n",error_buffer,icode);
      perror("system message:");
      fflush(FOUT);
      exit(1);
    }
#endif
}


/*\  ERROR TERMINATION
 *   Fortran version
\*/
#if F2C_HIDDEN_STRING_LENGTH_AFTER_ARGS
void FATR ga_error_(char *string, Integer *icode, int slen)
#else
void FATR ga_error_(char *string, int slen, Integer *icode)
#endif
{
#define FMSG 256
char buf[FMSG];
      ga_f2cstring(string,slen, buf, FMSG);
      gai_error(buf,*icode);
}


void ga_debug_suspend()
{
#ifdef HAVE_PAUSE
   fprintf(stdout,"ga_debug: process %ld ready for debugging\n",
           (long)getpid());
   fflush(stdout);
   pause();
#endif
}








#ifdef ARMCI

/*********************************************************************
 *        N-dimensional operations                                   *
 *********************************************************************/


/*\ print range of n-dimensional array with two strings before and after
\*/
static void gai_print_range(char *pre,int ndim, 
                            Integer lo[], Integer hi[], char* post)
{
        int i;

        printf("%s[",pre);
        for(i=0;i<ndim;i++){
                printf("%ld:%ld",(long)lo[i],(long)hi[i]);
                if(i==ndim-1)printf("] %s",post);
                else printf(",");
        }
}


static void swap(Integer *a, Integer *b)
{
  Integer tmp;
  tmp = *a;
  *a = *b;
  *b = tmp;
}


/*\ prints array distribution in C or Fortran style
\*/
void gai_print_distribution(int fstyle, Integer g_a)
{
Integer ndim, i, proc, type, nproc=ga_nnodes_();
Integer dims[MAXDIM], lo[MAXDIM], hi[MAXDIM];
char msg[100];
char *name;
int local_sync_begin,local_sync_end;

    local_sync_begin = _ga_sync_begin; local_sync_end = _ga_sync_end;
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    if(local_sync_begin)ga_sync_();

    if(ga_nodeid_() ==0){
      nga_inquire_internal_(&g_a, &type, &ndim, dims);
      gai_inquire_name(&g_a, &name);
      printf("Array Handle=%d Name:'%s' ",(int)g_a, name);
      printf("Data Type:");
      switch(type){
        case C_DBL: printf("double"); break;
        case C_INT: printf("integer"); break;
        case C_DCPL: printf("double complex"); break;
        case C_SCPL: printf("float (single) complex"); break;
        case C_FLOAT: printf("float"); break; 
        case C_LONG: printf("long"); break; 
        case C_LONGLONG: printf("long long"); break; 
        default: gai_error("ga_print_distribution: type not supported",type);
      }
      printf("\nArray Dimensions:");
      if(fstyle){
         for(i=0; i<ndim-1; i++)printf("%ldx",(long)dims[i]);
         printf("%ld\n",(long)dims[ndim-1]);
      }else{
         for(i=ndim-1; i>0; i--)printf("%ldx",(long)dims[i]);
         printf("%ld\n",(long)dims[0]);
      }

      /* print array range for every processor */
      for(proc = 0; proc < nproc; proc++){
          nga_distribution_(&g_a,&proc,lo,hi);
          sprintf(msg,"Process=%d\t owns array section: ",(int)proc);

          /* for C style need to swap and decremenent by 1 both arrays */
          if(!fstyle){
             for(i=0; i<ndim/2; i++){
                 swap(lo+i,lo+ndim-i-1); 
                 swap(hi+i,hi+ndim-i-1); 
             }
             for(i=0; i<ndim; i++)lo[i]--;
             for(i=0; i<ndim; i++)hi[i]--;
          }
          gai_print_range(msg,(int)ndim,lo,hi,"\n");
      }
      fflush(stdout);
    }

    if(local_sync_end)ga_sync_();
}


/*\ print array distribution to stdout (fortran style)
\*/
void FATR ga_print_distribution_(Integer* g_a)
{
   gai_print_distribution(1, *g_a);
}



/*
 * Jialin added nga_print and nga_print_patch on Jun 28, 1999
 */

/*\ PRINT g_a[ilo, jlo]
\*/
void FATR nga_file_print_patch(file, g_a, lo, hi, pretty)
        Integer *g_a, *lo, *hi, *pretty;
        FILE *file;
/*
  Pretty = 0 ... spew output out with no formatting
  Pretty = 1 ... format output so that it is readable
*/  
{
#define BUFSIZE 6
#define FLEN 80 

    Integer i, j;
    Integer type;
    char *name;
    Integer ndim, dims[MAXDIM], ld[MAXDIM];
    Integer bufsize;
    int ibuf[BUFSIZE], ibuf_2d[BUFSIZE*BUFSIZE];
    DoublePrecision dbuf[BUFSIZE], dbuf_2d[BUFSIZE*BUFSIZE];
    float fbuf[BUFSIZE], fbuf_2d[BUFSIZE*BUFSIZE];
    Integer lop[MAXDIM], hip[MAXDIM];
    long lbuf[BUFSIZE], lbuf_2d[BUFSIZE*BUFSIZE];
    long long llbuf[BUFSIZE], llbuf_2d[BUFSIZE*BUFSIZE];
    Integer done, status_2d, status_3d;
    _ga_sync_begin = 1; _ga_sync_end=1; /*remove any previous masking*/
    ga_sync_();
    gai_check_handle(g_a, "nga_print");

    /* only the first process print the array */
    if(ga_nodeid_() == 0) {
        
        nga_inquire_internal_(g_a,  &type, &ndim, dims);
        gai_inquire_name(g_a, &name);
        
        /* check the boundary */
        for(i=0; i<ndim; i++)
            if(lo[i] <= 0 || hi[i] > dims[i]) 
                gai_error("g_a indices out of range ", *g_a);
        
        /* print the general information */
        fprintf(file,"\n global array: %s[", name);
        for(i=0; i<ndim; i++)
            if(i != (ndim-1))
                fprintf(file, "%ld:%ld,", (long)lo[i], (long)hi[i]);
            else
                fprintf(file, "%ld:%ld",  (long)lo[i], (long)hi[i]);
        fprintf(file,"],  handle: %d \n", (int)*g_a);
        
        bufsize = (type==C_DCPL)? BUFSIZE/2 : BUFSIZE;
        bufsize = (type==C_SCPL)? BUFSIZE/2 : BUFSIZE;
        
        for(i=0; i<ndim; i++) ld[i] = bufsize;
        
        if(!*pretty) {
            done = 1;
            for(i=0; i<ndim; i++) {
                lop[i] = lo[i]; hip[i] = lo[i];
            }
            hip[0] = GA_MIN(lop[0]+bufsize-1, hi[0]);
            while(done) {
                switch(type) {
                    case C_INT:      nga_get_(g_a, lop, hip, ibuf, ld); break;
                    case C_DBL:      nga_get_(g_a, lop, hip, dbuf, ld); break;
                    case C_DCPL:     nga_get_(g_a, lop, hip, dbuf, ld); break;
                    case C_FLOAT:    nga_get_(g_a, lop, hip, fbuf, ld); break; 
                    case C_SCPL:     nga_get_(g_a, lop, hip, fbuf, ld); break;
                    case C_LONG:     nga_get_(g_a, lop, hip, lbuf, ld); break; 
                    case C_LONGLONG: nga_get_(g_a, lop, hip, llbuf,ld); break;
                    default: gai_error("ga_print: wrong type",0);
                }
                
                /* print the array */
                for(i=0; i<(hip[0]-lop[0]+1); i++) {
                    fprintf(file,"%s(", name);
                    for(j=0; j<ndim; j++)
                        if((j == 0) && (j == (ndim-1)))
                            fprintf(file, "%ld", (long)lop[j]+i);
                        else if((j != 0) && (j == (ndim-1)))
                            fprintf(file, "%ld", (long)lop[j]);
                        else if((j == 0) && (j != (ndim-1)))
                            fprintf(file, "%ld,", (long)lop[j]+i);
                        else fprintf(file, "%ld,", (long)lop[j]);
                    switch(type) {
                        case C_INT:
                            fprintf(file,") = %d\n", ibuf[i]);break;
                        case C_LONG:
                            fprintf(file,") = %ld\n", lbuf[i]);break;
                        case C_LONGLONG:
                            fprintf(file,") = %lld\n", llbuf[i]);break;
                        case C_DBL:
                            if((double)dbuf[i]<100000.0)
                                fprintf(file,") = %f\n", dbuf[i]);
                            else fprintf(file,") = %e\n", dbuf[i]);
                            break;
                        case C_DCPL:
                            if(((double)dbuf[i*2]<100000.0) &&
                               ((double)dbuf[i*2+1]<100000.0))
                                fprintf(file,") = (%f,%f)\n",
                                        dbuf[i*2],dbuf[i*2+1]);
                            else
                                fprintf(file,") = (%e,%e)\n",
                                        dbuf[i*2],dbuf[i*2+1]);
                            break;
                        case C_SCPL:
                            if(((float)dbuf[i*2]<100000.0) &&
                               ((float)dbuf[i*2+1]<100000.0))
                                fprintf(file,") = (%f,%f)\n",
                                        fbuf[i*2],fbuf[i*2+1]);
                            else
                                fprintf(file,") = (%e,%e)\n",
                                        fbuf[i*2],fbuf[i*2+1]);
                            break;
                        case C_FLOAT: fprintf(file,") = %f\n", fbuf[i]);break; 
                    }
                }
                
                fflush(file);
                
                lop[0] = hip[0]+1; hip[0] = GA_MIN(lop[0]+bufsize-1, hi[0]);
                
                for(i=0; i<ndim; i++) {
                    if(lop[i] > hi[i]) {
                        if(i == (ndim-1)) {
                            done = 0;
                        } else {
                            lop[i] = lo[i];
                            if(i == 0) hip[i] = GA_MIN(lop[i]+bufsize-1, hi[i]);
                            else hip[i] = lo[i];
                            lop[i+1]++; hip[i+1]++;
                        }
                    }
                }
            }
        }
        else {
            /* pretty print */
            done = 1;
            for(i=0; i<ndim; i++) {
                lop[i] = lo[i];
                if((i == 0) || (i == 1))
                    hip[i] = GA_MIN(lop[i]+bufsize-1, hi[i]);
                else 
                    hip[i] = lo[i];
            }
            
            status_2d = 1; status_3d = 1;
            
            while(done) {
                if(status_3d && (ndim > 2)) { /* print the patch info */
                    fprintf(file,"\n -- patch: %s[", name);
                    for(i=0; i<ndim; i++)
                        if(i < 2)
                            if(i != (ndim-1))
                                fprintf(file, "%ld:%ld,", (long)lo[i], (long)hi[i]);
                            else
                                fprintf(file, "%ld:%ld", (long)lo[i], (long)hi[i]);
                        else
                            if(i != (ndim-1))
                                fprintf(file, "%ld,", (long)lop[i]);
                            else
                                fprintf(file, "%ld", (long)lop[i]);
                    fprintf(file,"]\n"); status_3d = 0;
                }
                
                if(status_2d &&(ndim > 1)) {
                    fprintf(file, "\n"); 
                    switch(type) {
                        case C_INT:
                            fprintf(file, "     ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%7ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," --------");
                            break;
                        case C_LONG:
                            fprintf(file, "     ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%7ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," --------");
                            break;
                        case C_LONGLONG:
                            fprintf(file, "     ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%7ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," --------");
                            break;
                        case C_DBL:
                            fprintf(file, "   ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%10ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," -----------");
                            break;
                        case C_DCPL:
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%22ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," -----------------------");
                            break;
                        case C_SCPL:
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%22ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," -----------------------");
                            break;
                        case C_FLOAT:
                            fprintf(file, "     ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file, "%7ld  ", (long)i);
                            fprintf(file,"\n      ");
                            for (i=lop[1]; i<=hip[1]; i++)
                                fprintf(file," --------");
                            break;
                       default:
                         gai_error("ga_print: wrong type", 0);
                    }
                    
                    fprintf(file,"\n");
                    status_2d = 0;
                }
                
                switch(type) {
                    case C_INT: nga_get_(g_a, lop, hip, ibuf_2d, ld); break;
                    case C_LONG: nga_get_(g_a, lop, hip,lbuf_2d, ld); break;
                    case C_LONGLONG: nga_get_(g_a, lop, hip,llbuf_2d,ld);break;
                    case C_DBL: nga_get_(g_a, lop, hip, dbuf_2d, ld); break;
                    case C_DCPL: nga_get_(g_a, lop, hip, dbuf_2d, ld);break;
                    case C_FLOAT: nga_get_(g_a, lop, hip, fbuf_2d, ld);break;
                    case C_SCPL: nga_get_(g_a, lop, hip, fbuf_2d, ld);break;  
                   default: gai_error("ga_print: wrong type",0);
                }
                
                for(i=0; i<(hip[0]-lop[0]+1); i++) {
                    fprintf(file,"%4ld  ", (long)(lop[0]+i));
                    switch(type) {
                        case C_INT:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    fprintf(file," %8d", ibuf_2d[j*bufsize+i]);
                            else fprintf(file," %8d", ibuf_2d[i]);
                            break;
                        case C_LONG:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    fprintf(file," %8ld",lbuf_2d[j*bufsize+i]);
                            else fprintf(file," %8ld",lbuf_2d[i]);
                            break;
                        case C_LONGLONG:
                            if(ndim > 1)
                               for(j=0; j<(hip[1]-lop[1]+1); j++)
                                  fprintf(file," %8lld",llbuf_2d[j*bufsize+i]);
                            else fprintf(file," %8lld",llbuf_2d[i]);
                            break;
                        case C_DBL:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    if((double)dbuf_2d[j*bufsize+i]<100000.0)
                                        fprintf(file," %11.5f",
                                                dbuf_2d[j*bufsize+i]);
                                    else
                                        fprintf(file," %.5e",
                                                dbuf_2d[j*bufsize+i]);
                            else
                                if((double)dbuf_2d[i]<100000.0)
                                    fprintf(file," %11.5f",dbuf_2d[i]);
                                else
                                    fprintf(file," %.5e",dbuf_2d[i]);
                            break;
                        case C_FLOAT:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    fprintf(file," %11.5f", fbuf_2d[j*bufsize+i]);
                            else fprintf(file," %11.5f", fbuf_2d[i]);
                            break;           
                        case C_DCPL:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    if(((double)dbuf_2d[(j*bufsize+i)*2]<100000.0)&&((double)dbuf_2d[(j*bufsize+i)*2+1]<100000.0))
                                        fprintf(file," %11.5f,%11.5f",
                                                dbuf_2d[(j*bufsize+i)*2],
                                                dbuf_2d[(j*bufsize+i)*2+1]);
                                    else
                                        fprintf(file," %.5e,%.5e",
                                                dbuf_2d[(j*bufsize+i)*2],
                                                dbuf_2d[(j*bufsize+i)*2+1]);
                            else
                                if(((double)dbuf_2d[i*2]<100000.0) &&
                                   ((double)dbuf_2d[i*2+1]<100000.0))
                                    fprintf(file," %11.5f,%11.5f",
                                            dbuf_2d[i*2], dbuf_2d[i*2+1]);
                                else
                                    fprintf(file," %.5e,%.5e",
                                            dbuf_2d[i*2], dbuf_2d[i*2+1]);
			    break;
                        case C_SCPL:
                            if(ndim > 1)
                                for(j=0; j<(hip[1]-lop[1]+1); j++)
                                    if(((float)fbuf_2d[(j*bufsize+i)*2]<100000.0)&&((float)fbuf_2d[(j*bufsize+i)*2+1]<100000.0))
                                        fprintf(file," %11.5f,%11.5f",
                                                fbuf_2d[(j*bufsize+i)*2],
                                                fbuf_2d[(j*bufsize+i)*2+1]);
                                    else
                                        fprintf(file," %.5e,%.5e",
                                                fbuf_2d[(j*bufsize+i)*2],
                                                fbuf_2d[(j*bufsize+i)*2+1]);
                            else
                                if(((float)fbuf_2d[i*2]<100000.0) &&
                                   ((float)fbuf_2d[i*2+1]<100000.0))
                                    fprintf(file," %11.5f,%11.5f",
                                            fbuf_2d[i*2], fbuf_2d[i*2+1]);
                                else
                                    fprintf(file," %.5e,%.5e",
                                            fbuf_2d[i*2], fbuf_2d[i*2+1]);
                            break;
                       default:
                          gai_error("ga_print: wrong data type", 0);
                    }
                    
                    fprintf(file,"\n");
                }
                
                lop[0] = hip[0]+1; hip[0] = GA_MIN(lop[0]+bufsize-1, hi[0]);
                
                for(i=0; i<ndim; i++) {
                    if(lop[i] > hi[i]) {
                        if(i == (ndim-1)) {
                            done = 0;
                        } else {
                            lop[i] = lo[i];
                            
                            if((i == 0) || (i == 1)) {
                                hip[i] = GA_MIN(lop[i]+bufsize-1, hi[i]);
                            } else {
                                hip[i] = lo[i];
                            }
                            
                            if(i == 0) {
                                lop[i+1] = hip[i+1]+1;
                                hip[i+1] = GA_MIN(lop[i+1]+bufsize-1, hi[i+1]);
                            } else {
                                lop[i+1]++; hip[i+1]++;
                            }
                            
                            if(i == 0) status_2d = 1;
                            if(i == 1) status_3d = 1;
                        }
                    }
                }
            }
        }
        fflush(file);
    }
    
    ga_sync_();
}

void FATR nga_print_patch_(Integer *g_a, Integer *lo, Integer *hi, Integer *pretty)
{
  nga_file_print_patch(stdout, g_a, lo, hi, pretty);

}

void FATR ga_summarize_(Integer *verbose)
{
#define DEV stdout
    
    Integer i, j, g_a;
    Integer printed, arr_no;
    Integer type, active;
    char *name;
    Integer ndim, dims[MAXDIM];
    Integer lop[MAXDIM], hip[MAXDIM];
    Integer nproc = ga_nnodes_();
    
    fprintf(DEV, " Summary of allocated global arrays\n");
    fprintf(DEV, "-----------------------------------\n");

    printed = 0;
    arr_no = 0;
    
    for(g_a=-1000; g_a<-900; g_a++) {
        active = ga_verify_handle_(&g_a);

        if(active == 1) {
            printed = 1;
            nga_inquire_internal_(&g_a, &type, &ndim, dims);
            gai_inquire_name(&g_a, &name);
            
            switch(type) {
                case C_INT:
                    fprintf(DEV, "  array %d => integer ", (int)arr_no);
                    break;
                case C_DBL:
                    fprintf(DEV, "  array %d => double precision ",(int)arr_no);
                    break;
                case C_DCPL:
                    fprintf(DEV, "  array %d => double complex ", (int)arr_no);
                    break;
                case C_SCPL:
                    fprintf(DEV, "  array %d => float (single) complex ", (int)arr_no);
                    break;
                case C_FLOAT:
                    fprintf(DEV, "  array %d => float ",(int)arr_no);
                    break;       
                case C_LONG:
                    fprintf(DEV, "  array %d => long ",(int)arr_no);
                    break;   
                case C_LONGLONG:
                    fprintf(DEV, "  array %d => long long",(int)arr_no);
                    break;   
                default: gai_error("ga_print: wrong type",0);
            }
            arr_no++;

            fprintf(DEV,"%s(", name);
            for(i=0; i<ndim; i++)
                if(i != (ndim-1)) fprintf(DEV, "%ld,", (long)dims[i]);
                else fprintf(DEV, "%ld", (long)dims[i]);
            fprintf(DEV,"),  handle: %d \n",(int) g_a);

            if(*verbose) {
                for(i=0; i<nproc; i++){
                    nga_distribution_(&g_a, &i, lop, hip);
                    
                    fprintf(DEV,"    (");
                    for(j=0; j<ndim; j++)
                        if(j != (ndim-1))
                            fprintf(DEV, "%ld:%ld,",(long)lop[j], (long)hip[j]);
                        else
                            fprintf(DEV, "%ld:%ld", (long)lop[j], (long)hip[j]);
                    fprintf(DEV,") -> %d \n",(int) i);
                }
            }
        }
    }

    if(!printed) fprintf(DEV, "  No active global arrays\n");

    fprintf(DEV, "\n\n");
    fflush(DEV);
}

#endif

void ga_print_file(FILE *file, Integer *g_a)
{
#ifdef ARMCI    
    Integer i;
    Integer type, ndim, dims[MAXDIM];
    Integer lo[MAXDIM];
    Integer pretty = 1;

    nga_inquire_internal_(g_a, &type, &ndim, dims);

    for(i=0; i<ndim; i++) lo[i] = 1;

    nga_file_print_patch(file, g_a, lo, dims, &pretty);

#else
    Integer type, dim1, dim2;
    Integer ilo=1, jlo=1;
    Integer pretty = 1;
    
    ga_inquire_internal_(g_a, &type, &dim1, &dim2);
    
    ga_file_print_patch(file, g_a, &ilo, &dim1, &jlo, &dim2, &pretty);
#endif    
}
  
void FATR ga_print_(Integer *g_a)
{
    ga_print_file(stdout, g_a);
}


/*\ return ClusterNode id of the specified process
\*/
Integer FATR ga_cluster_proc_nodeid_(Integer *proc)
{
    int id = armci_domain_id(ARMCI_DOMAIN_SMP, (int)*proc);
    return (Integer) id;
}

/*\ return ClusterNode id of the calling process
\*/
Integer FATR ga_cluster_nodeid_()
{
    int id = armci_domain_my_id(ARMCI_DOMAIN_SMP);
    return (Integer) id;
}

/*\ number of nodes in a cluster
\*/
Integer FATR ga_cluster_nnodes_()
{
    return (Integer) armci_domain_count(ARMCI_DOMAIN_SMP);
}

/*\ number of processes in the job on the specified node
\*/
Integer FATR ga_cluster_nprocs_(Integer *node)
{
    int id;
    id = (int)*node;
    return (Integer) armci_domain_nprocs(ARMCI_DOMAIN_SMP, id);
}


/*\ global id of calling process on the node
\*/
Integer FATR ga_cluster_procid_(Integer *node, Integer *loc_proc_id)
{
        int nodeid, procid;
        nodeid = (int)*node;
        procid = (int)*loc_proc_id;
        return (Integer) armci_domain_glob_proc_id(ARMCI_DOMAIN_SMP, nodeid,
                                                   procid);
}

#ifdef MPI
#  include <mpi.h>
#else
#  include "tcgmsg.h"
#endif
/*\ wrapper for wallclock timer. Returns an alapsed time on calling process
\*/
DoublePrecision FATR ga_wtime_() 
{
    double wtime=0.0;
#ifdef MPI
    wtime = MPI_Wtime();
#else
    wtime = tcg_time();
#endif
    return (DoublePrecision)wtime; 
}
