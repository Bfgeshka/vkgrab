#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../config.h"
#include <errno.h>

struct crl_st
{
	char * payload;
	size_t size;
};

size_t
crl_callback( void * content, size_t wk_size, size_t wk_nmemb, void * upoint )
{
	/* curl has strange ways, pretty often they can do it better */
	size_t rsize = wk_nmemb * wk_size;
	struct crl_st * p = (struct crl_st *) upoint;

	/* allocation for new size */
	p->payload = (char *) realloc(p->payload, p->size + rsize + 1);
	if ( p->payload == NULL )
	{
		fprintf( stderr, "Reallocation failed in crl_callback()\n" );
		return -1;
	}

	/* making valid string */
	memcpy(&(p->payload[p->size]), content, rsize);
	p->size += rsize;
	p->payload[p->size] = 0;
	return rsize;
}

CURLcode
crl_fetch( CURL * hc, const char * url, struct crl_st * fetch_str )
{
	CURLcode code;

	fetch_str->size = 0;
	fetch_str->payload = (char *) calloc( 1, sizeof(fetch_str->payload) );
	if (fetch_str->payload == NULL)
	{
		fprintf( stderr, "Allocation failed in crl_fetch()\n" );
		return CURLE_FAILED_INIT;
	}

	curl_easy_setopt(hc, CURLOPT_URL, url);
	curl_easy_setopt(hc, CURLOPT_WRITEFUNCTION, crl_callback);
	curl_easy_setopt(hc, CURLOPT_WRITEDATA, (void *) fetch_str);
	curl_easy_setopt(hc, CURLOPT_USERAGENT, "libcurl-agent/1.0");
//	curl_easy_setopt(hc, CURLOPT_TIMEOUT, 5);
//	curl_easy_setopt(hc, CURLOPT_FOLLOWLOCATION, 1);
//	curl_easy_setopt(hc, CURLOPT_MAXREDIRS, 1);
	curl_easy_setopt(hc, CURLOPT_VERBOSE, CRL_VERBOSITY);
	code = curl_easy_perform(hc);
	return code;
}

size_t
write_file(void * ptr, size_t size, size_t nmemb, FILE * stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
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
		fprintf( stderr, "GET error: %s\n", curl_easy_strerror(code) );
		return "err2";
	}
	if ( !cf->payload )
	{
		fprintf( stderr, "Callback is empty, nothing to do here\n");
		return "err3";
	}

	return cf->payload;
}

size_t
vk_get_file( const char * url, const char * filepath, CURL * curl )
{
	if (curl)
	{
		/* skip downloading if file exists */
		errno = 0;
		int err;
		FILE * fr = fopen( filepath, "r" );
		err = errno;
		if ( fr )
			fclose(fr);
		else if ( err == ENOENT )
		{
			FILE * fw = fopen( filepath, "w");
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fw);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, CRL_VERBOSITY);
			CURLcode code;
			code = curl_easy_perform(curl);

			if ( code != CURLE_OK )
			{
				fprintf( stderr, "GET error: %s\n", curl_easy_strerror(code) );
				return 1;
			}
			curl_easy_reset( curl );
			fclose(fw);
		}
	}

	return 0;
}
