#include <curl/curl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

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
	curl_easy_setopt(hc, CURLOPT_TIMEOUT, 5);
	curl_easy_setopt(hc, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(hc, CURLOPT_MAXREDIRS, 1);
	curl_easy_setopt(hc, CURLOPT_VERBOSE, CRL_VERBOSITY);
	code = curl_easy_perform(hc);
	return code;
}

char *
vk_api( const char * url)
{
	/* handler initialisatiion */
	CURL * hc;
	hc = curl_easy_init();
	if ( hc == NULL )
	{
		fprintf( stderr, "cURL initialisation error\n" );
		return "err";
	}

	curl_easy_setopt(hc, CURLOPT_VERBOSE, 0L);

	/* fetching an answer */
	CURLcode code;
	struct crl_st wk_crl_st;
	struct crl_st * cf = &wk_crl_st;
	code = crl_fetch( hc, url, cf );
	curl_easy_cleanup(hc);

	if (code != CURLE_OK || cf->size < 1)
	{
		fprintf( stderr, "GET error: %s\n", curl_easy_strerror(code) );
		return "err";
	}


	if (!cf->payload)
	{
		fprintf( stderr, "Callback is empty, nothing to do here\n");
		return "err";
	}

	return cf->payload;
}

