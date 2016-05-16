#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "advstring.h"
#include "platform.h"

#ifndef va_copy
    #ifdef __va_copy
        #define va_copy(dest, src)          __va_copy((dest), (src))
    #else
        #define va_copy(dest, src)          memcpy((&dest), (&src), sizeof(va_list))
    #endif
#endif

advstring::advstring(int size) {
	this->_size = size;
	_buffer = (char *)malloc(size+1);
	clear();
}

advstring::~advstring() {
	free(_buffer);
}

inline void advstring::clear() {
	_buffer[0] = 0;
	_currentPos = 0;
}

const char *advstring::str() {
	return _buffer;
}

int advstring::size() {
	return _currentPos;
}

void advstring::insert(int clear, ...) {
	
	if(clear) {
		this->clear();
	}

	int len;
	char *val;
	char *temp;
	va_list vl;
	va_start(vl,clear);
	val=va_arg(vl,char *);
	while(val != NULL) {
		len = strlen(val);
		if (_currentPos + len + 1 > _size) {
			temp = (char*)malloc(_size*2);
			memcpy(temp,_buffer,_size);
			_size = len * 2;
			free(_buffer);
			_buffer = temp;
		}
		memcpy(&_buffer[_currentPos],val,len);
		_currentPos += len;
		_buffer[_currentPos] = 0;
		val=va_arg(vl,char *);
	}
	
	va_end(vl);
}

void advstring::assign(const char *fmt, va_list ap) {
	
	va_list args;
	va_copy(args, ap);
	
	int len = vsnprintf(NULL, 0, fmt, ap);
    if(len > _size) {
		_size = len * 2;
		_buffer = (char*)realloc(_buffer, _size);
		
	}
	
	vsnprintf(_buffer, _size, fmt, args);
	_currentPos = len;
	_buffer[len] = 0;
}

advstring& advstring::operator<< (char *string) {
	int len = strlen(string);
	if(_currentPos + len + 1 <= _size) {
		memcpy(&_buffer[_currentPos],string,len);
		_currentPos += len;
		_buffer[_currentPos] = 0;
	}
	return *this;
}

advstring& advstring::operator<< (const char *string) {
	*this << (char *)string;
	return *this;
}
