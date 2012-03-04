#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>

#include "luc_defs.h"

static luc_error_t ReadData(void *inPtr, u_int64_t inDataLen,
        void **outPtr, u_int64_t *outDataLen,
        void **completionArg, LUC_Mem_Avail_Completion *completionFctn,
        luc_endpoint_id_t callerEndpoint);

static luc_error_t WriteData(void *inPtr, u_int64_t inDataLen,
        void **outPtr, u_int64_t *outDataLen,
        void **completionArg, LUC_Mem_Avail_Completion *completionFctn,
        luc_endpoint_id_t callerEndpoint0);

static LucEndpoint *srvEndpoint = NULL;

unsigned write_phase = 0;
unsigned chunk_size;
long unsigned offset;
char filename[256];


int main(int argc, char *argv[])
{
    luc_error_t err;

    srvEndpoint = luc_allocate_endpoint(LUC_SERVER_ONLY);

    err = srvEndpoint->registerRemoteCall(FIO_ENGINE, FIO_READ_DATA, ReadData);
    if (err != LUC_ERR_OK) {
        fprintf(stderr, "Error registering callback function: %d\n", err);
        delete srvEndpoint;
        exit(EXIT_FAILURE);
    }
    err = srvEndpoint->registerRemoteCall(FIO_ENGINE, FIO_WRITE_DATA, WriteData);
    if (err != LUC_ERR_OK) {
        fprintf(stderr, "Error registering callback function: %d\n", err);
        delete srvEndpoint;
        exit(EXIT_FAILURE);
    }
    err = srvEndpoint->startService(1);
    if (err != LUC_ERR_OK) {
        fprintf(stderr, "Error starting service: %d\n", err);
        delete srvEndpoint;
        exit(EXIT_FAILURE);
    }
    
    
    
    long int end_point = srvEndpoint->getMyEndpointId();
    printf("Server started, endpoint id is %ld - file >%s<\n", end_point, END_POINT_FILE);
    fflush(stdout);

    struct passwd* userInfoPtr = getpwuid(getuid());
    if (userInfoPtr != NULL )
        (int)chdir(userInfoPtr->pw_dir);
    else {
        printf("I can't chdir in home!\n");
        exit(1);
    }

    /* publishing end point on tmp file */
    char command[256];
    sprintf(command, "rm -rf %s", END_POINT_FILE);
    system(command);
    FILE * fp = fopen(END_POINT_FILE, "w");
    if (fp == NULL ){
      printf("Error creating end point file!\n");
      exit(1);
    }
    fprintf(fp,"%ld", end_point);
    fclose(fp);
    

    pause();

    srvEndpoint->stopService();
    delete srvEndpoint;

    return 0;
} // main





luc_error_t ReadData(void *inPtr, u_int64_t inDataLen,
		     void **outPtr, u_int64_t *outDataLen,
		     void **completionArg, LUC_Mem_Avail_Completion *completionFctn,
		     luc_endpoint_id_t callerEndpoint)
{
    write_phase = 0;
    sscanf((char *) inPtr,"%s %lu %d", &filename, &offset, &chunk_size); 
    
    char *buf = reinterpret_cast<char *>(malloc(sizeof(char) * chunk_size) );

    printf("R Open file %s\n", (char * ) filename);
    FILE *fp = NULL;
    fp = fopen (( const char * ) filename, "r" );
    if ( fp == NULL ) {
        printf("Error: Can not open file %s\n", (char *) filename );
        return 0;   
    } 
     
    printf("Seeking file for position %ld\n", offset);
    fseek(fp, (long) offset, SEEK_SET);
    printf("Reading %d bytes \n", chunk_size );
    
    int ret = fread(buf, sizeof(char), chunk_size, fp);
    printf("Read %d bytes \n", ret);
    fclose(fp);
    if(ret != chunk_size) {
        printf("Error: read file return %d it was supposed to be %d\n", ret, chunk_size);
        return 0;   
    }
  
    *outDataLen = chunk_size; 
    *outPtr = buf;

    *completionArg = buf;
    *completionFctn = free;
    
    return LUC_ERR_OK;
} // ReadData


luc_error_t WriteData(void *inPtr, u_int64_t inDataLen,
		     void **outPtr, u_int64_t *outDataLen,
		     void **completionArg, LUC_Mem_Avail_Completion *completionFctn,
		     luc_endpoint_id_t callerEndpoint)
{
    if (write_phase == 0 ) {
        sscanf((char *) inPtr,"%s %lu %d", &filename, &offset, &chunk_size); 
        write_phase = 1;
        printf("phase 1\n");
        return LUC_ERR_OK;
    } else {
        printf("W Open file %s\n", (char * ) filename);
        FILE *fp = NULL;
        fp = fopen (( const char * ) filename,"a" );
        if ( fp == NULL ) {
            printf("Error: Can not open file %s\n", (char *) filename );
            write_phase = 0;
            return 0;   
        } 

        printf("Seeking file for position %ld\n", offset);
        fseek(fp, (long) offset, SEEK_SET);
        printf("writing %d bytes \n", chunk_size );
    
        int ret = fwrite(inPtr, sizeof(char), chunk_size, fp);

        fclose(fp);
        if(ret != chunk_size) {
            printf("Error: read file return %d it was supposed to be %d\n", ret, chunk_size);
            write_phase = 0;
            return 0;   
        }
        write_phase = 0;
    }
    return LUC_ERR_OK;
} // WriteData


