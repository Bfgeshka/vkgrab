#ifndef UTILS_H_
#define UTILS_H_

struct string
{
	size_t bufsize;
	size_t len;
	char * c;
};

void
calclen ( struct string * s )
{
	for ( size_t i = 0; s->c[i] != '\0'; ++i )
		s->len = i;
}

struct string
newstring ( int size )
{
	struct string s;
	s.bufsize = size;
	s.len = 0;
	s.c = malloc(s.bufsize);

	return s;
}

void
stringset ( struct string * s, const char * src )
{
	snprintf( s->c, s->bufsize, "%s", src );
	calclen(s);
}

#endif
