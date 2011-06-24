#include <string.h>
#include <stdarg.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "BufferedPrinter.hpp"

BufferedPrinter* __bp_singleton__ = NULL;

BufferedPrinter::BufferedPrinter(int bufsize)
	: capacity(bufsize)
	, buf((char*)malloc(sizeof(char)*bufsize))
	, csize(0)
	, lock((pthread_mutex_t*)malloc(sizeof(pthread_mutex_t))) {

	*lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
}

BufferedPrinter::~BufferedPrinter() {
	free(buf);
	pthread_mutex_destroy(lock);
}

void BufferedPrinter::_bprintf(const char* formatstr, ...) {

	va_list al;
	va_start(al, formatstr);

	_vbprintf(formatstr, al);

	va_end(al);
}

void BufferedPrinter::_vbprintf(const char* formatstr, va_list al) {
	pthread_mutex_lock(lock);

	char newstr[BP_MAX];
	vsnprintf(newstr, BP_MAX, formatstr, al);

	size_t len = strnlen(newstr, BP_MAX);

	if (csize+len >= capacity) {
		flush();
		sprintf(buf, "%s", newstr);
		csize = len;
	} else {
		strcat(buf, newstr);
		csize+=len;
	}

	pthread_mutex_unlock(lock);
}

void BufferedPrinter::flush() {
	pthread_mutex_lock(lock);

	printf("%s", buf);
	csize = 0;

	pthread_mutex_unlock(lock);
}

void bprintf(const char* formatstr, ...) {
	if (!__bp_singleton__) __bp_singleton__ = new BufferedPrinter(1<<16);

	va_list al;
	va_start(al, formatstr);
	__bp_singleton__->_vbprintf(formatstr, al);
	va_end(al);
}

void vbprintf(const char* formatstr, va_list args) {
	if (!__bp_singleton__) __bp_singleton__ = new BufferedPrinter(1<<16);

	__bp_singleton__->_vbprintf(formatstr, args);
}


void bpflush() {
	if (__bp_singleton__) __bp_singleton__->flush();
}
