#define _XOPEN_SOURCE 600
#include <stddef.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "luc_file_io.h"

int
luc_fread (void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename)
{
    int64_t fd = -1;
    ssize_t sz;
    if (!filename) return -1;
    if ((fd = open (filename, O_RDONLY)) < 1) return -1;
    sz = pread (fd, ptr, size * nmemb, offset);
    close (fd);
    return (int64_t) sz;
}

int
luc_fwrite (void * ptr, size_t size, size_t nmemb, size_t offset, const char * filename)
{
    int64_t fd = -1;
    ssize_t sz;
    if (!filename) return -1;
    if ((fd = open (filename, O_WRONLY|O_CREAT)) < 1) return -1;
    sz = pwrite (fd, ptr, size * nmemb, offset);
    close (fd);
    return (int64_t) sz;
}
