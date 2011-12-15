#ifndef __LUC_FILE_IO__
#define __LUC_FILE_IO__

#ifdef __cplusplus
extern "C" {
#endif 

int64_t luc_fread(void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename);

int64_t luc_fwrite(void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename);

int64_t luc_dwrite(void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename);

int64_t get_filesize(const char * filename);

#ifdef __cplusplus
}
#endif 

#endif
