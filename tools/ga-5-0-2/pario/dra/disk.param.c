#if HAVE_CONFIG_H
#   include "config.h"
#endif

/** @file
 * store and retrieve metadata/parameters for disk resident array
 *       -- at present time, we use separate file for metadata
 */

#if HAVE_STDIO_H
#   include <stdio.h>
#endif
#if HAVE_UNISTD_H
#   include <unistd.h>
#endif

#include "drap.h"
#include "global.h"

#define MAX_HD_NAME_LEN 100
#define HD_NAME_EXT_LEN 10
#define HDLEN           80 
#define HD_EXT          ".info"

/**
 * check file configuration: shared or independent files are used
 *  we'll verify if every process can access DRA metafile
 *  if yes, then we have shared file, otherwise independent files
 */
int dai_file_config(char* filename)
{
    char param_filename[MAX_HD_NAME_LEN];
    Integer len;
    char  sum='+';
    Integer me=ga_nodeid_();
    Integer nproc = ga_nnodes_();
    Integer status;
    stat_t info;

    if(nproc==1) return 0;

    /* build param file name */
    len = strlen(filename);
    if(len+HD_NAME_EXT_LEN >= MAX_HD_NAME_LEN)
        dai_error("dai_file_config: filename too long:",len);
    strcpy(param_filename,filename);
    strcat(param_filename,HD_EXT);

    /*    printf("checking file: %s\n",param_filename);fflush(stdout);*/

    status = (Integer) elio_stat(param_filename, &info);

    /* processor 0 created the file => it must be able to stat it */
    if(me==0 && status!= ELIO_OK)
        dai_error("dai_file_config: no access from 0",status);

    status = (status==ELIO_OK) ? 1 : 0; /* normalize status */

    /* combine status accross all processors */
    gai_igop(DRA_GOP_TYPE, &status, 1, &sum);

    /* 1     - only 0 can access the file => independent files 
     * nproc - all can access it => shared file
     * otherwise - same processors can access it => something is wrong!!!
     */ 
    if(status == 1) return(1);
    else if(status == nproc) return 0;
#ifdef NO_SMP_NODES
    else dai_error("dai_file_config: confusing file configuration",status); 
#endif
    return 1;
}


/**
 * Retrive metadata for a disk array from the disk
 */
int dai_read_param(char* filename,Integer d_a)
{
    FILE *fd;
    char param_filename[MAX_HD_NAME_LEN];
    Integer len, i, ndim;
    Integer me=ga_nodeid_();
    Integer brd_type=DRA_BRD_TYPE, orig, dra_hndl=d_a+DRA_OFFSET;
    long input;
    int rc=0;
    char dummy[HDLEN];

    ga_sync_();

    if(!me){ /* only process 0 reads metafile */

        /* build param file name */
        len = strlen(filename);
        if(len+HD_NAME_EXT_LEN >= MAX_HD_NAME_LEN)
            dai_error("dai_read_param: filename too long:",len);
        strcpy(param_filename,filename);
        strcat(param_filename,HD_EXT);

        if((fd=fopen(param_filename,"r"))){

            if(!fscanf(fd,"%ld", &input))  dai_error("dai_read_param:ndim",0);
            DRA[dra_hndl].ndim = (Integer) input;
            ndim = (Integer) input;
            for (i=0; i<ndim; i++) {
                if(!fscanf(fd,"%ld", &input))  dai_error("dai_read_param:dims",i);
                DRA[dra_hndl].dims[i] = (Integer) input;
            }

            if(!fscanf(fd,"%ld",&input))   dai_error("dai_read_param:type",0);
            DRA[dra_hndl].type = (int) input;
            if(!fscanf(fd,"%ld",&input))   dai_error("dai_read_param:layout",0);
            DRA[dra_hndl].layout = (Integer) input;

            for (i=0; i<ndim; i++) {
                if(!fscanf(fd,"%ld",&input))   dai_error("dai_read_param:chunk",i);
                DRA[dra_hndl].chunk[i] = (Integer) input;
            }

            if(!fscanf(fd,"%ld",&input))   dai_error("dai_read_param:numfiles",0);
            DRA[dra_hndl].numfiles = (Integer) input;

            if(!fscanf(fd,"%ld",&input))   dai_error("dai_read_param:ioprocs",0);
            DRA[dra_hndl].ioprocs = (Integer) input;

            fgets(dummy,HDLEN,fd); /*advance to next line*/
            if(!fgets(DRA[dra_hndl].name,DRA_MAX_NAME,fd))dai_error("dai_read_param:name",0);

            if(fclose(fd))dai_error("dai_read_param: fclose failed",0);
        }else rc = -1;
    }


    orig = 0; len=sizeof(int);
    ga_brdcst_(&brd_type, &rc, &len, &orig);
    if(rc) return(rc);

    /* process 0 broadcasts data to everybody else */
    len = sizeof(disk_array_t);
    ga_brdcst_(&brd_type, DRA + dra_hndl, &len, &orig);

    return(rc);
}


/**
 * Store parameters of a disk array on the disk
 */
void dai_write_param(char* filename, Integer d_a)
{
    Integer len;
    FILE *fd;
    char param_filename[MAX_HD_NAME_LEN];
    Integer me=ga_nodeid_(), dra_hndl=d_a+DRA_OFFSET;
    Integer i, ndim = DRA[dra_hndl].ndim;

    ga_sync_();

    if(!me){ /* only process 0 writes param file */

        /* build param file name */
        len = strlen(filename);
        if(len + HD_NAME_EXT_LEN >= MAX_HD_NAME_LEN)
            dai_error("dai_write_param: filename too long:",len);
        strcpy(param_filename,filename);
        strcat(param_filename,HD_EXT);

        if(! (fd = fopen(param_filename,"w")) ) {
            char message[MAX_HD_NAME_LEN*2];
            strcpy(message,"dai_write_param:open failed :: ");
            strcpy(message,param_filename);
            dai_error(message,0);
        }

        if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].ndim)) 
            dai_error("dai_write_param:ndim",0);
        for (i=0; i<ndim; i++) {
            if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].dims[i])) 
                dai_error("dai_write_param:dims",i);
        }
        if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].type)) 
            dai_error("dai_write_param:type",0);
        if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].layout))
            dai_error("dai_write_param:layout",0);
        for (i=0; i<ndim; i++) {
            if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].chunk[i]))
                dai_error("dai_write_param:chunk",i);
        }
        if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].numfiles)) 
            dai_error("dai_write_param:numfiles",0);
        if(!fprintf(fd,"%ld ",(long)DRA[dra_hndl].ioprocs)) 
            dai_error("dai_write_param:ioprocs",0);

        if(!fprintf(fd,"\n%s\n",DRA[dra_hndl].name))
            dai_error("dai_write_param:name",0);

        if(fclose(fd))dai_error("dai_write_param: fclose failed",0);
    }

    ga_sync_();
}


/**
 * Delete info file
 */
void dai_delete_param(char* filename,Integer d_a)
{
    char param_filename[MAX_HD_NAME_LEN];
    int len;
    Integer me=ga_nodeid_();

    ga_sync_();

    if(!me){ /* only process 0 reads param file */

        /* build param file name */
        len = strlen(filename);
        if(len+HD_NAME_EXT_LEN >= MAX_HD_NAME_LEN)
            dai_error("dai_read_param: filename too long:",len);
        strcpy(param_filename,filename);
        strcat(param_filename,HD_EXT);

        if(unlink(param_filename)) dai_error("dai_delete_param failed",d_a);
    }
}
