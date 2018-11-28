#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>

typedef struct string
{
	size_t bufsize;
	size_t len;
	char * c;
} sstring;

sstring * construct_string ( size_t );
void newstring ( sstring *, size_t );
void calclen ( sstring * );
void stringset ( sstring *, const char *, ... );
void free_string ( sstring * );

#endif
