#ifndef CURL_REQ_H_
#define CURL_REQ_H_
#include "../config.h"
/*
#include <math.h>
#include <stdio.h>
*/

#define KIBI 1024
#define DEFAULT_TERM_COL 80

struct crl_st
{
	char * payload;
	size_t size;
};

unsigned get_width        ( void );
unsigned utf8_char_offset ( const char * );
size_t   crl_callback     ( void *, size_t, size_t, void * );
CURLcode crl_fetch        ( CURL *, const char *, struct crl_st * );
size_t   write_file       ( void *, size_t, size_t, FILE * );
/*char *   vk_get_request   ( const char *, CURL * ); */
void     vk_get_request   ( const char *, CURL *, struct crl_st * );
int      progress_func    ( void *, double, double, double, double );
int      cp_file          ( const char *, const char * );
size_t   vk_get_file      ( const char *, const char *, CURL * );

#endif
