#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <stdlib.h>

#include "functions.h"

char TOKEN[BUF_STRING] = TOKEN_HEAD;

char *
api_request( const char * url, CURL * curl )
{
	/* struct initialiisation */
	CURLcode code;
	struct crl_st wk_crl_st;
	struct crl_st * cf = &wk_crl_st;

	/* fetching an answer */
	code = crl_fetch( curl, url, cf );
	curl_easy_reset( curl );

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

size_t
crl_callback( void * content, size_t wk_size, size_t wk_nmemb, void * upoint )
{
	size_t rsize = wk_nmemb * wk_size;
	struct crl_st * p = (struct crl_st *) upoint;

	/* allocation for new size */
	p->payload = (char *) realloc( p->payload, p->size + rsize + 1 );
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
	curl_easy_setopt( hc, CURLOPT_VERBOSE, 0L );
	code = curl_easy_perform( hc );
	return code;
}

long long
js_get_int( json_t * src, char * key )
{
	json_t * elem = json_object_get( src, key );
	return json_integer_value( elem );
}

const char *
js_get_str( json_t * src, char * key )
{
	json_t * elem = json_object_get( src, key );
	return json_string_value( elem );
}

void
check_token()
{
	if ( strlen( TOKEN ) != strlen( CONST_TOKEN ) )
		sprintf( TOKEN, "%s", CONST_TOKEN );
}

int
group_id( int argc, char ** argv, CURL * curl )
{
	int t;
	long long offset = 0;

	/* No arguments */
	if ( argc == 1 )
	{
		help_print();
		return 0;
	}

	/* 1 argument */
	else if ( argc == 2 )
	{
		if ( argv[1][0] == '-' )
		{
			if ( argv[1][1] == 'h' )
				help_print();

			else if ( argv[1][1] == 'T' )
				fprintf(stdout,
				        "https://oauth.vk.com/authorize?client_id=%d&scope=%s&display=page&response_type=token\n",
				        APPLICATION_ID, PERMISSIONS);

			else
			{
				fputs( "Unknown argument, abort.", stderr );
				return -1;
			}

			return 0;
		}

		else
			sprintf( group.name_scrn, "%s", argv[1] );
	}

	/* Several arguments */
	else
		for ( t = 0; t < argc; ++t )
		{
			if ( argv[t][0] == '-' )
			{
				if ( argv[t][1] == 't' )
				{
					if ( strlen( TOKEN ) == strlen( TOKEN_HEAD ) )
						strcat( TOKEN, argv[t+1] );
					else
					{
						if ( atoi( argv[t+1] ) == 0 )
							sprintf( TOKEN, "%c", '\0' );
						else
							sprintf( TOKEN, "%s%s", TOKEN_HEAD, argv[t+1] );
					}
				}
				if ( argv[t][1] == 'h' )
				{
					help_print();
					return 0;
				}
			}

			else
				sprintf( group.name_scrn, "%s", argv[t] );
		}

	/* Requesting data */
	char url[BUF_STRING];
	json_error_t json_err;
	json_t * json;
	json_t * rsp;

		sprintf( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld", \
		        REQ_HEAD, API_VERSION, group.name_scrn, offset );
		char * r = api_request( url, curl );

		json = json_loads( r, 0, &json_err );
		if ( !json )
		{
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", \
			        json_err.line, json_err.text );
			return -2;
		}

		rsp = json_object_get( json, "response" );
		if (!rsp)
		{
			json_error( json );
			return -3;
		}

		if ( offset == 0 )
		{
			group.sub_count = js_get_int( rsp, "count" );
			printf( "Subscribers count: %lld\n", group.sub_count );
		}

	return 1;
}

int
group_memb( CURL * curl, long long * ids )
{
	long long offset = 0;

	/* Requesting data */
	char url[BUF_STRING];
	json_error_t json_err;
	json_t * json;
	json_t * rsp;
	size_t index;

	do
	{
		sprintf( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld%s", \
		        REQ_HEAD, API_VERSION, group.name_scrn, offset, TOKEN );
		char * r = api_request( url, curl );

		json = json_loads( r, 0, &json_err );
		if ( !json )
		{
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", \
			        json_err.line, json_err.text );
			return -2;
		}

		rsp = json_object_get( json, "response" );
		if (!rsp)
		{
			json_error( json );
			return -3;
		}

		json = json_object_get( rsp, "items" );
		json_array_foreach( json, index, rsp )
		{
			ids[offset + index] = json_integer_value( rsp );
		}

		offset +=GROUP_MEMBERS_COUNT;
	}
	while ( group.sub_count > offset );

	return 1;
}



void
json_error( json_t * json )
{
	json_t * err_block;
	err_block = json_object_get( json, "error" );
	if ( err_block )
		fprintf( stderr, "Requst error %lld: %s\n", \
		        js_get_int( err_block, "error_code" ), js_get_str( err_block, "error_msg" ) );

}

void
help_print()
{
	puts("Usage: blacklist_finder [OPTIONS] <GROUP>");
	puts("");
	puts("Options:");
	puts("  -h             show help");
	puts("  -T             generate link for getting a token");
	puts("  -t TOKEN       give a valid token without header \"&access_token=\". If TOKEN is zero then anonymous access given");
}

int
user_subs( long long id, CURL * curl, char * output_file )
{
	long offset = 0;
	long count;

	/* Requesting data */
	char url[BUF_STRING];
	json_error_t json_err;
	json_t * json;
	json_t * rsp;
	size_t index;
	FILE * logfile = fopen( output_file, "w" );

	sprintf( url, "%s/users.get?v=%s&user_id=%lld&fields=screen_name%s", \
	         REQ_HEAD, API_VERSION, id, TOKEN );
	char * rr = api_request( url, curl );

	json = json_loads( rr, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", \
		         json_err.line, json_err.text );
		return -2;
	}

	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		json_error( json );
		return -3;
	}

	fprintf( logfile, "%s:\"%s %s\"\n\n", js_get_str( rsp, "screen_name" ), js_get_str( rsp, "first_name" ), js_get_str( rsp, "last_name" ) );

	do
	{
		sprintf( url, "%s/users.getSubscriptions?v=%s&user_id=%lld&offset=%ld&extended=1%s", \
		         REQ_HEAD, API_VERSION, id, offset, TOKEN );
		char * r = api_request( url, curl );

		json = json_loads( r, 0, &json_err );
		if ( !json )
		{
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", \
			         json_err.line, json_err.text );
			return -2;
		}

		rsp = json_object_get( json, "response" );
		if (!rsp)
		{
			json_error( json );
			return -3;
		}

		if ( offset == 0 )
		{
			count = (long) js_get_int( rsp, "count" );
			printf( "User is subscribed to %ld pages\n", count );
		}

		json = json_object_get( rsp, "items" );
		json_array_foreach( json, index, rsp )
		{
			fprintf( logfile, "%s:\"%s\"", js_get_str( rsp, "screen_name" ), js_get_str( rsp, "name" ) );
		}

		offset +=USER_SUBSCRIPTIONS_COUNT;
	}
	while ( count > offset );

	return 0;
}
