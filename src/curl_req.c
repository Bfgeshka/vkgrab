#include "../config.h"
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#define KIBI 1024
#define DEFAULT_TERM_COL 80

struct crl_st
{
	char * payload;
	size_t size;
};

/* terminal width */
unsigned
get_width( void )
{
	unsigned width = DEFAULT_TERM_COL;
	struct winsize ws;
	if ( ioctl( 1, TIOCGWINSZ, &ws ) == 0 )
		width = ws.ws_col;
	return width;
}

/* Num. of spaces for aligning [right side] because of unicode */
unsigned
utf8_char_offset( const char * input )
{
	size_t bytelen = strlen( input );
	unsigned char_count = 0;

	for ( unsigned i = 0; i < bytelen; ++i )
		if ( (input[i] & 0xC0) != 0x80 )
			char_count++;

	return (bytelen - char_count)/2;
}

size_t
crl_callback( void * content, size_t wk_size, size_t wk_nmemb, void * upoint )
{
	/* curl has strange ways, pretty often they can do it better */
	size_t rsize = wk_nmemb * wk_size;
	struct crl_st * p = ( struct crl_st * ) upoint;

	/* allocation for new size */
	p->payload = ( char * ) realloc( p->payload, p->size + rsize + 1 );
	if ( p->payload == NULL )
	{
		fprintf( stderr, "Reallocation failed in crl_callback()\n" );
		return -1;
	}

	/* making valid string */
	memcpy( &( p->payload[p->size] ), content, rsize );
	p->size += rsize;
	p->payload[p->size] = 0;
	return rsize;
}

CURLcode
crl_fetch( CURL * hc, const char * url, struct crl_st * fetch_str )
{
	CURLcode code;

	fetch_str->size = 0;
	fetch_str->payload = ( char * ) calloc( 1, sizeof( fetch_str->payload ) );
	if ( fetch_str->payload == NULL )
	{
		fprintf( stderr, "Allocation failed in crl_fetch()\n" );
		return CURLE_FAILED_INIT;
	}

	curl_easy_setopt( hc, CURLOPT_URL, url );
	curl_easy_setopt( hc, CURLOPT_WRITEFUNCTION, crl_callback );
	curl_easy_setopt( hc, CURLOPT_WRITEDATA, ( void * ) fetch_str );
	curl_easy_setopt( hc, CURLOPT_USERAGENT, "libcurl-agent/1.0" );
	/*	curl_easy_setopt( hc, CURLOPT_TIMEOUT, 5 );
		curl_easy_setopt( hc, CURLOPT_FOLLOWLOCATION, 1 );
		curl_easy_setopt( hc, CURLOPT_MAXREDIRS, 1 );	*/
	curl_easy_setopt( hc, CURLOPT_VERBOSE, CRL_VERBOSITY );
	code = curl_easy_perform( hc );
	return code;
}

size_t
write_file( void * ptr, size_t size, size_t nmemb, FILE * stream )
{
	size_t written = fwrite( ptr, size, nmemb, stream );
	return written;
}


char *
vk_get_request( const char * url, CURL * hc )
{
	/* struct initialiisation */
	CURLcode code;
	struct crl_st wk_crl_st;
	struct crl_st * cf = &wk_crl_st;

	/* fetching an answer */
	code = crl_fetch( hc, url, cf );
	curl_easy_reset( hc );

	/* checking result */
	if ( code != CURLE_OK || cf->size < 1 )
	{
		fprintf( stderr, "GET error: %s\n", curl_easy_strerror( code ) );
		return "err2";
	}
	if ( !cf->payload )
	{
		fprintf( stderr, "Callback is empty, nothing to do here\n" );
		return "err3";
	}

	return cf->payload;
}

int
progress_func( void * ptr, double todl_total, double dl_ed, double undef_a, double undef_b )
{
	(void) undef_a;
	(void) undef_b;
	(void) ptr;

	double percentage = 0;
	if ( todl_total != 0 )
		percentage = (dl_ed / todl_total) * 100;

	putchar( ' ' );
	putchar( '<' );
	for ( unsigned char i = 0; i < 100; i += 5 )
	{
		if ( percentage > i )
			printf( "▓" );
		else
			putchar( ' ' );
	}
	printf( "> [%03.2f%%] %.1f/%.1f KiB\r\b", percentage, dl_ed/KIBI, todl_total/KIBI );
	fflush( stdout );
	return 0;
}

int
cp_file( const char * to, const char * from )
{
	int fd_to, fd_from;
	char buf[8*bufs];
	ssize_t nread;

	fd_from = open( from, O_RDONLY );
	if ( fd_from < 0 )
		return -1;

	fd_to = open( to, O_WRONLY | O_CREAT | O_EXCL, 0666 );
	if ( fd_to < 0 )
		goto cp_func_out_error;

	while ( nread = read( fd_from, buf, sizeof buf ), nread > 0 )
	{
		char *out_ptr = buf;
		ssize_t nwritten;

		do
		{
			nwritten = write( fd_to, out_ptr, nread );

			if ( nwritten >= 0 )
			{
				nread -= nwritten;
				out_ptr += nwritten;
			}
			else if ( errno != EINTR )
				goto cp_func_out_error;
		}
		while ( nread > 0 );
	}

	if ( nread == 0 )
	{
		if ( close( fd_to ) < 0 )
		{
			fd_to = -1;
			goto cp_func_out_error;
		}
		close( fd_from );

		/* Success! */
		return 0;
	}

cp_func_out_error:
	close( fd_from );
	if ( fd_to >= 0 )
		close( fd_to );
	return -1;
}

size_t
vk_get_file( const char * url, const char * filepath, CURL * curl )
{
	if ( curl )
	{
		unsigned spaces_offset = utf8_char_offset( filepath );
		unsigned term_width = get_width();
		/* skip downloading if file exists */
		errno = 0;
		struct stat fst;
		long long file_size = 0;
		int err;
		FILE * fr = fopen( filepath, "r" );
		err = errno;
		if ( fr != NULL )
		{
			fclose( fr );
			if (stat( filepath, &fst ) != -1 )
				file_size = (long long) fst.st_size;

			if ( file_size > 0 )
			{
				printf( "\r\b%-*s", term_width, filepath );
				while ( spaces_offset > 0 )
				{
					--spaces_offset;
					putchar( ' ' );
				}
				printf( "\b\b\b\b\b\b\b\b\033[00;36m[SKIP]\033[00m\n"  );
				return 0;
			}
		}
		if ( err == ENOENT || file_size == 0 )
		{
			fflush( stdout );
			FILE * fw = fopen( TMP_CURL_FILENAME, "w" );
			curl_easy_setopt( curl, CURLOPT_URL, url );
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_file );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, fw );
			curl_easy_setopt( curl, CURLOPT_VERBOSE, CRL_VERBOSITY );
			curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, 1 );
			curl_easy_setopt( curl, CURLOPT_MAXREDIRS, 2 );
			curl_easy_setopt( curl, CURLOPT_NOPROGRESS, 0 );
			curl_easy_setopt( curl, CURLOPT_PROGRESSFUNCTION, progress_func );
			CURLcode code;
			code = curl_easy_perform( curl );

			if ( code != CURLE_OK )
			{
				fprintf( stderr, "GET error: %s [%s]\n", curl_easy_strerror( code ), url );
				return 1;
			}
			curl_easy_reset( curl );
			fclose( fw );
			cp_file( filepath, TMP_CURL_FILENAME );

			printf( "\r\b%-*s", term_width, filepath );
			while ( spaces_offset > 0 )
			{
				--spaces_offset;
				putchar( ' ' );
			}
			printf( "\b\b\b\b\b\b\b\033[01;32m[OK]\033[00m\n" );

		}
	}

	return 0;
}
