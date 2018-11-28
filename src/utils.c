#include "utils.h"

#include <stdio.h>

void
calclen ( sstring * s )
{
	for ( size_t i = 0; s->c[i] != '\0'; ++i )
		s->len = i;
}

void
newstring ( sstring * s, int size )
{
	s->bufsize = size;
	s->len = 0;
	s->c = malloc(s->bufsize);
	s->c[0] = '\0';
}

void
stringset ( sstring * s, const char * src )
{
	snprintf( s->c, s->bufsize, "%s", src );
	calclen(s);
}
