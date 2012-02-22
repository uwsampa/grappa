#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

//#define MAX_CHUNK_SIZE (256 * 1024 * 1024)
#define MAX_CHUNK_SIZE (1024 * 1024)
#include "luc_file_io.h"
#include "luc_defs.h"

/* returns the size of the file in bytes */
int get_filesize(const char * filename) {
    struct stat buf;

    if ( stat(filename, &buf) != 0 ) 
        return -1;
    return  buf.st_size;
}

/* fix absolute and relative path names */
int fix_filename(char * luc_filename, const char * filename) {
    /* if it starts with / it is an absolute path leave it unchanged */
    char tmp_dir[1024];
    size_t len;
    if (filename[0] == '/') {
	strcpy(luc_filename, filename); 
	return 1;
    }

    /* Otherwise, just append the provided string to the current
       working directory.  Relative paths will handle themselves. */

    if (!getcwd (tmp_dir, 1024)) {
	fprintf (stderr, "tmp_dir too short");
	abort ();
    }

    len = strlen (tmp_dir);
    memcpy (luc_filename, tmp_dir, len);
    luc_filename[len] = '/';
    strcpy (&luc_filename[len+1], filename);
    return 1;
}

/* read the end point published by the server */
long int get_end_point() {
    /* reading end point from file */
    long int end_point; 
    FILE * fp;
    char tmp_dir[256];
    getcwd(tmp_dir,256);
    struct passwd* userInfoPtr = getpwuid(getuid());
    if (userInfoPtr != NULL )
        chdir(userInfoPtr->pw_dir);
    else {
        printf("I can't chdir in home!\n");
        exit(1);
    }
 
    fp = fopen(END_POINT_FILE, "r");
    if ( fp == NULL ) {
        printf("Error: luc file I/O server not found >%s<\n", END_POINT_FILE);
        exit(1);
    }
    fscanf(fp,"%ld", &end_point);
    fclose(fp);
    printf("Client read endpoint %ld from file %s\n", end_point, END_POINT_FILE);
    chdir(tmp_dir);
    return end_point;
}





int luc_fread(void *ptr, size_t size, size_t nmemb, size_t offset, const char * filename) {

    unsigned num_chunks, chunk = 0;
    int      file_size_byte, read_bytes;
    luc_error_t result;
    static LucEndpoint *clientEndpoint = NULL;
    
    char luc_filename[1024];
    fix_filename(luc_filename, filename);
    printf("File name: %s\n", luc_filename);
    
    
    file_size_byte = get_filesize(luc_filename);
    if ( file_size_byte == -1 ) return -1;
    read_bytes = nmemb * size;
    
    /* if the offset is larger than the file return 0 */
    if ( offset * size > file_size_byte ) 
       return 0; 

    /* if the number of bytes requested is larger than the file size 
       adjust the read_bytes */
    if ( read_bytes > file_size_byte - offset * size )
        read_bytes = file_size_byte - offset * size;


    /* compute number of chunks to read for this file */
    num_chunks = read_bytes  / MAX_CHUNK_SIZE;
    if ( num_chunks * MAX_CHUNK_SIZE < read_bytes )
        num_chunks++;

    /* allocating client end point */
    if (!clientEndpoint) {
        clientEndpoint = luc_allocate_endpoint(LUC_CLIENT_ONLY);
        result = clientEndpoint->startService();

        if (result != LUC_ERR_OK) {
            fprintf(stderr, "Error starting LUC client: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }

    /* reading end point from file */
    luc_endpoint_id_t serverID = get_end_point();  
    /* request chunks to the server */ 
    for ( chunk = 0 ; chunk < num_chunks; chunk++) {
         
        size_t chunk_size = MAX_CHUNK_SIZE;
        /* the last chunk needs to be adjusted */
        if ( chunk == num_chunks - 1 )
            chunk_size = read_bytes  - (chunk * MAX_CHUNK_SIZE);
            
        printf("Reading chunk %d/%d, size %d\n", chunk, num_chunks, chunk_size);
        
        const unsigned INPUT_PARAMETER_SIZE = 1024;
        char input_parameters[INPUT_PARAMETER_SIZE];
        long unsigned chunk_offset = offset + chunk * MAX_CHUNK_SIZE;
        sprintf(input_parameters,"%s %lu %d", luc_filename, chunk_offset, chunk_size);
        
        char * out_ptr = (char *) ptr + (chunk * MAX_CHUNK_SIZE) * sizeof(char);

        result = clientEndpoint->remoteCallSync(serverID, FIO_ENGINE, FIO_READ_DATA, 
                (void *) input_parameters , sizeof(char) * INPUT_PARAMETER_SIZE,
                out_ptr , &chunk_size);

       
        if (result < LUC_ERR_MAX || result != LUC_ERR_OK) {
            fprintf(stderr, "LUC RPC error: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }
    return read_bytes;
}





int luc_fwrite(void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename) {
    unsigned num_chunks, chunk = 0;
    long     write_bytes;
    luc_error_t result;
    static LucEndpoint *clientEndpoint = NULL;
   
    write_bytes = nmemb * size;

    /* compute number of chunks to write for this file */
    num_chunks = write_bytes  / MAX_CHUNK_SIZE;
    if ( num_chunks * MAX_CHUNK_SIZE < write_bytes )
        num_chunks++;

    /* allocating client end point */
    if (!clientEndpoint) {
        clientEndpoint = luc_allocate_endpoint(LUC_CLIENT_ONLY);
        result = clientEndpoint->startService();

        if (result != LUC_ERR_OK) {
            fprintf(stderr, "Error starting LUC client: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }
    
    luc_endpoint_id_t serverID = get_end_point();  


    char luc_filename[1024];
    fix_filename(luc_filename, filename);
    printf("File name: %s\n", luc_filename);

    /* send chunks to the server */ 
    for ( chunk = 0 ; chunk < num_chunks; chunk++) {
         
        size_t chunk_size = MAX_CHUNK_SIZE;
        /* the last chunk needs to be adjusted */
        if ( chunk == num_chunks - 1 )
            chunk_size = write_bytes  - (chunk * MAX_CHUNK_SIZE);
            
        printf("writing chunk %d/%d, size %d\n", chunk, num_chunks, chunk_size);
        
        const unsigned INPUT_PARAMETER_SIZE = 1024;
        char input_parameters[INPUT_PARAMETER_SIZE];
        long unsigned chunk_offset = offset + chunk * MAX_CHUNK_SIZE;
        sprintf(input_parameters,"%s %lu %d", luc_filename, chunk_offset, chunk_size);
        

        result = clientEndpoint->remoteCallSync(serverID, FIO_ENGINE, FIO_WRITE_DATA, 
                (void *) input_parameters , sizeof(char) * INPUT_PARAMETER_SIZE,
                NULL , NULL);

        char * in_ptr =  (char *) ptr + (chunk * MAX_CHUNK_SIZE) * sizeof(char);
        result = clientEndpoint->remoteCallSync(serverID, FIO_ENGINE, FIO_WRITE_DATA, 
                (void *) in_ptr , sizeof(char) * chunk_size,
                NULL , NULL);
       
        if (result < LUC_ERR_MAX || result != LUC_ERR_OK) {
            fprintf(stderr, "LUC RPC error: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }


    return write_bytes;
}



int luc_dwrite(void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename) {
    unsigned num_chunks, chunk = 0;
    long     write_bytes;
    luc_error_t result;
    static LucEndpoint *clientEndpoint = NULL;
   
    write_bytes = nmemb * size;

    /* compute number of chunks to write for this file */
    num_chunks = write_bytes  / MAX_CHUNK_SIZE;
    if ( num_chunks * MAX_CHUNK_SIZE < write_bytes )
        num_chunks++;

    /* allocating client end point */
    if (!clientEndpoint) {
        clientEndpoint = luc_allocate_endpoint(LUC_CLIENT_ONLY);
        result = clientEndpoint->startService();

        if (result != LUC_ERR_OK) {
            fprintf(stderr, "Error starting LUC client: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }
    
    luc_endpoint_id_t serverID = get_end_point();  


    char luc_filename[1024];
    fix_filename(luc_filename, filename);
    printf("File name: %s\n", luc_filename);

    /* send chunks to the server */ 
    for ( chunk = 0 ; chunk < num_chunks; chunk++) {
         
        size_t chunk_size = MAX_CHUNK_SIZE;
        /* the last chunk needs to be adjusted */
        if ( chunk == num_chunks - 1 )
            chunk_size = write_bytes  - (chunk * MAX_CHUNK_SIZE);
            
        printf("writing chunk %d/%d, size %d\n", chunk, num_chunks, chunk_size);
        
        const unsigned INPUT_PARAMETER_SIZE = 1024;
        char input_parameters[INPUT_PARAMETER_SIZE];
        long unsigned chunk_offset = offset + chunk * MAX_CHUNK_SIZE;
        sprintf(input_parameters,"%s %lu %d", luc_filename, chunk_offset, chunk_size);
        

        result = clientEndpoint->remoteCallSync(serverID, FIO_ENGINE, FIO_WRITE_DOUBLES, 
                (void *) input_parameters , sizeof(char) * INPUT_PARAMETER_SIZE,
                NULL , NULL);

        char * in_ptr =  (char *) ptr + (chunk * MAX_CHUNK_SIZE) * sizeof(char);
        result = clientEndpoint->remoteCallSync(serverID, FIO_ENGINE, FIO_WRITE_DOUBLES, 
                (void *) in_ptr , sizeof(char) * chunk_size,
                NULL , NULL);
       
        if (result < LUC_ERR_MAX || result != LUC_ERR_OK) {
            fprintf(stderr, "LUC RPC error: %d\n", result);
            delete clientEndpoint;
            exit(EXIT_FAILURE);
        }
    }


    return write_bytes;
}

