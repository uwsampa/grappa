#include <pthread.h>
#include <stdarg.h>

#define BP_MAX 1024

class BufferedPrinter {
	private:
		char* buf;
		const int capacity;
		int csize;
		pthread_mutex_t* lock;


	public:
		BufferedPrinter(int bufsize);
		~BufferedPrinter();

		void _bprintf(const char* formatstr, ...);
		void _vbprintf(const char* formatstr, va_list al);

		void flush();


};


// convenience function to provide replacement for printf
void bprintf(const char* formatstr, ...);
void vbprintf(const char* formatstr, va_list al);
void bpflush();
