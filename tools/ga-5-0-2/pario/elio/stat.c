#if HAVE_CONFIG_H
#   include "config.h"
#endif

/* $Id: stat.c,v 1.20.10.3 2007-08-30 18:19:44 manoj Exp $ */

#include "eliop.h"
#include "chemio.h"
 
#if defined(CRAY) && defined(__crayx1)
#undef CRAY
#endif

#define DEBUG_ 0
 

/**
 * determines directory path for a given file
 */
int elio_dirname(const char *fname, char *dirname, int len)
{
    size_t flen;
    
    if(len<= (flen =strlen(fname))) 
    ELIO_ERROR(LONGFAIL,flen);
    
#ifdef WIN32
    while(fname[flen] != '/' && fname[flen] != '\\' && flen >0 ) flen--;
#else
    while(fname[flen] != '/' && flen >0 ) flen--;
#endif

    if(flen==0)strcpy(dirname,".");
    else {strncpy(dirname, fname, flen); dirname[flen]=(char)0;}
    
    return(ELIO_OK);
}


#ifdef WIN32
#include <direct.h>
#include <stdlib.h>

/**
 * determine drive name given the file path name
 */ 
char* elio_drivename(const char* fname)
{

         static char path[_MAX_PATH];
         static char drive[_MAX_DRIVE];
         
         if( _fullpath(path,fname,_MAX_PATH) == NULL) return NULL;
         _splitpath(path, drive, NULL, NULL, NULL);
         return(drive);
}


void  get_avail_space(int dev, avail_t *avail, int* bsize)
{
      static char drive[4]="A:\\";
      int sectors, cfree, ctotal;
      drive[0]= dev + 'A';

      GetDiskFreeSpace(drive, &sectors, bsize, &cfree, &ctotal);
      *avail = sectors*(avail_t)cfree;
}

#endif /* WIN32 */
         
         
/**
 * Stat a file (or path) to determine it's filesystem info
 */
int  elio_stat(char *fname, stat_t *statinfo)
{
    struct  stat      ufs_stat;
    int bsize;
    
#if defined(PARAGON)
    struct statpfs  *statpfsbuf;
    struct estatfs   estatbuf;
    int              bufsz;
    char             str_avail[64];
#else
    struct  STATVFS   ufs_statfs;
#endif
    
    PABLO_start(PABLO_elio_stat); 
    
#if defined(PARAGON)
    bufsz = sizeof(struct statpfs) + SDIRS_INIT_SIZE;
    if( (statpfsbuf = (struct statpfs *) malloc(bufsz)) == NULL)
        ELIO_ERROR(ALOCFAIL,1);
    if(statpfs(fname, &estatbuf, statpfsbuf, bufsz) == 0)
    {
        if(estatbuf.f_type == MOUNT_PFS)
        statinfo->fs = ELIO_PFS;
        else if(estatbuf.f_type == MOUNT_UFS || estatbuf.f_type == MOUNT_NFS)
        statinfo->fs = ELIO_UFS;
        else
        ELIO_ERROR(FTYPFAIL, 1);

        /*blocks avail - block=1KB */ 
        etos(estatbuf.f_bavail, str_avail);
        if(strlen(str_avail)==10)
        fprintf(stderr,"elio_stat: possible ext. type conversion problem\n");
        if((bsize=strlen(str_avail))>10)
        ELIO_ERROR(CONVFAIL,(long)bsize);
        statinfo->avail = atoi(str_avail);
    } 
    else
    ELIO_ERROR(STATFAIL,1);
    free(statpfsbuf);

#else
    
    if(stat(fname, &ufs_stat) != 0)
        ELIO_ERROR(STATFAIL, 1);

#   if defined(PIOFS)
/*        fprintf(stderr,"filesystem %d\n",ufs_stat.st_vfstype);*/
        /* according to /etc/vfs, "9" means piofs */
        if(ufs_stat.st_vfstype == 9) statinfo->fs = ELIO_PIOFS;
        else
#   endif

    statinfo->fs = ELIO_UFS;
    
    /* only regular or directory files are OK */
    if(!S_ISREG(ufs_stat.st_mode) && !S_ISDIR(ufs_stat.st_mode))
        ELIO_ERROR(TYPEFAIL, 1);
    
#   if defined(CRAY) || defined(NEC)
    if(statfs(fname, &ufs_statfs, sizeof(ufs_statfs), 0) != 0)
#   elif defined (CATAMOUNT)
        statinfo->avail =2*1024*1024*128; 
        return(ELIO_OK);
#   else
        if(STATVFS(fname, &ufs_statfs) != 0)
#   endif
           ELIO_ERROR(STATFAIL,1);
    
#   if defined(WIN32)

       get_avail_space(ufs_statfs.st_dev, &(statinfo->avail), &bsize);
      
#   else
      /* get number of available blocks */
#     if defined(CRAY) || defined(NEC)
          /* f_bfree == f_bavail -- naming changes */

#        ifdef CRAY
          if(ufs_statfs.f_secnfree != 0) /* check for secondary partition */
             statinfo->avail = (avail_t) ufs_statfs.f_secnfree;
          else
#        endif
             statinfo->avail = (avail_t) ufs_statfs.f_bfree;
#     else
          statinfo->avail = (avail_t) ufs_statfs.f_bavail;
#     endif

#     ifdef NO_F_FRSIZE
         /*       on some older systems it was f_bsize */
         bsize = (int) ufs_statfs.f_bsize; 
#     else
         /* get block size, fail if bszie is still 0 */
         bsize = (int) ufs_statfs.f_frsize;
         if(bsize==0)bsize =(int) ufs_statfs.f_bsize; 
         if(bsize==0) ELIO_ERROR(STATFAIL, 1);

         if(DEBUG_)
           printf("stat: f_frsize=%d f_bsize=%d bsize=%d free blocks=%ld\n",
            (int) ufs_statfs.f_frsize,(int) ufs_statfs.f_bsize, bsize,
            statinfo->avail );
#     endif
#   endif
    
    /* translate number of available blocks into kilobytes */
    switch (bsize) {
    case 512:  statinfo->avail /=2; break;
    case 1024: break;
    case 2048: statinfo->avail *=2; break;
    case 4096: statinfo->avail *=4; break;
    case 8192: statinfo->avail *=8; break;
    case 16384: statinfo->avail *=16; break;
    case 32768: statinfo->avail *=32; break;
    default:   { 
        double avail;
        double factor = ((double)bsize)/1024.0;
        avail = factor * (double)statinfo->avail;
        statinfo->avail = (avail_t) avail;
               }
    }
    
#endif
    PABLO_end(PABLO_elio_stat);
    return(ELIO_OK);
}
