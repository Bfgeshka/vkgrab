#include "utils.h"

#include <stdio.h>
#include <stdarg.h>

void
calclen ( sstring * s )
{
	for ( size_t i = 0; s->c[i] != '\0'; ++i )
		s->len = i;
}

void
newstring ( sstring * s, size_t size )
{
	s->bufsize = size;
	s->len = 0;
	s->c = malloc(s->bufsize);
	s->c[0] = '\0';
}

sstring *
construct_string ( size_t size )
{
	sstring * s = malloc(sizeof(sstring));
	s->bufsize = size;
	s->len = 0;
	s->c = malloc(s->bufsize);
	s->c[0] = '\0';

	return s;
}

void
free_string ( sstring * s )
{
	free(s->c);
	free(s);
}

void
stringset ( sstring * s, const char * fmt, ... )
{
	va_list args;
	va_start( args, fmt );
	vsnprintf( s->c, s->bufsize, fmt, args );
	va_end(args);

	calclen(s);
}
