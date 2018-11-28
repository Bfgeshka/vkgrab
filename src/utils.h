#ifndef UTILS_H_
#define UTILS_H_

#include <stdlib.h>

typedef struct string
{
	size_t bufsize;
	size_t len;
	char * c;
} sstring;

void newstring ( sstring *, int );
void calclen ( sstring * );
void stringset ( sstring *, const char *, ... );

#endif
