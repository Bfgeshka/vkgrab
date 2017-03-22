#ifndef CURL_REQ_H_
#define CURL_REQ_H_
#include "../config.h"

#define KIBI 1024
#define DEFAULT_TERM_COL 80

struct crl_st
{
	char * payload;
	size_t size;
};

CURLcode crl_fetch        ( CURL *, const char *, struct crl_st * );
int      cp_file          ( const char *, const char * );
int      progress_func    ( void *, double, double, double, double );
size_t   crl_callback     ( void *, size_t, size_t, void * );
size_t   vk_get_file      ( const char *, const char *, CURL * );
size_t   write_file       ( void *, size_t, size_t, FILE * );
unsigned get_width        ( void );
unsigned utf8_char_offset ( const char * );
void     vk_get_request   ( const char *, CURL *, struct crl_st * );

#endif
