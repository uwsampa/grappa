#include <sys/types.h>

//TODO: Write documentation for this function

ssize_t write_threaded( int fd, const void *buffer, 
			size_t element_size, 
			size_t length,  
			int max_threads );
