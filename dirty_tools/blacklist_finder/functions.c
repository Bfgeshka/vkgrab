#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "functions.h"
#include "../../config.h"
#include "../../src/curl_req.h"
#include "../../src/methods.h"

char TKN[BUFSIZ] = TOKEN_HEAD;

int
group_id( int argc, char ** argv )
{
	int t;
	long long offset = 0;

	/* No arguments */
	if ( argc == 1 )
	{
		bf_help_print();
		return 0;
	}

	/* 1 argument */
	else if ( argc == 2 )
	{
		if ( argv[1][0] == '-' )
		{
			if ( argv[1][1] == 'h' )
				bf_help_print();

			else if ( argv[1][1] == 'T' )
				fprintf( stdout,
				         "https://oauth.vk.com/authorize?client_id=%d&scope=%s&display=page&response_type=token\n",
				         APPLICATION_ID, PERMISSIONS );

			else
			{
				fputs( "Unknown argument, abort.", stderr );
				return -1;
			}

			return 0;
		}

		else
			sprintf( grp.name_scrn, "%s", argv[1] );
	}

	/* Several arguments */
	else
		for ( t = 0; t < argc; ++t )
		{
			if ( argv[t][0] == '-' )
			{
				if ( argv[t][1] == 't' )
				{
					if ( strlen( TKN ) == strlen( TOKEN_HEAD ) )
						strcat( TKN, argv[t+1] );
					else
					{
						if ( atoi( argv[t+1] ) == 0 )
							sprintf( TKN, "%c", '\0' );
						else
							sprintf( TKN, "%s%s", TOKEN_HEAD, argv[t+1] );
					}
				}
				if ( argv[t][1] == 'h' )
				{
					bf_help_print();
					return 0;
				}
			}

			else
				sprintf( grp.name_scrn, "%s", argv[t] );
		}

	if ( strlen(TKN) != strlen(CONST_TOKEN) )
		sprintf( TKN, "%s", CONST_TOKEN );

	/* Requesting data */
	char url[BUFSIZ];
	struct crl_st wk_crl_st;
	struct crl_st * cf = &wk_crl_st;
	sprintf( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld%s", \
	         REQ_HEAD, API_VER, grp.name_scrn, offset, TKN );

	vk_get_request( url, grp.curl, cf );
	char * r = malloc( cf->size + 1 );
	sprintf( r, "%s", cf->payload );
	r[ cf->size ] = 0;
	if ( cf->payload != NULL )
		free( cf->payload );

	/* Parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( r != NULL )
		free(r);
	if ( !json )
	{
		fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err.line, json_err.text );
		return -2;
	}

	json_auto_t * rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		json_t * err_block;
		err_block = json_object_get( json, "error" );
		if ( err_block )
			fprintf( stderr, "Requst error %lld: %s\n", \
			         js_get_int( err_block, "error_code" ), js_get_str( err_block, "error_msg" ) );
		return -3;
	}

	if ( offset == 0 )
	{
		grp.sub_count = js_get_int( rsp, "count" );
		printf( "Subscribers count: %lld\n", grp.sub_count );
	}
/*	json_decref(json);*/
	api_request_pause();
	return 1;
}

int
group_memb( long long * ids )
{
	long long offset = 0;

	/* Requesting data */

	json_auto_t * rsp;

	size_t index;

	do
	{
		char url[BUFSIZ];
		struct crl_st wk_crl_st;
		struct crl_st * cf = &wk_crl_st;
		sprintf( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld%s", \
		         REQ_HEAD, API_VER, grp.name_scrn, offset, TKN );


		vk_get_request( url, grp.curl, cf );
		char * r = malloc( cf->size + 1 );
		sprintf( r, "%s", cf->payload );
		r[ cf->size ] = 0;
		if ( cf->payload != NULL )
			free( cf->payload );

		/* Parsing json */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( r != NULL )
			free(r);
		if ( !json )
		{
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err.line, json_err.text );
			return -2;
		}

		rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		json_t * err_block;
		err_block = json_object_get( json, "error" );
		if ( err_block )
			fprintf( stderr, "Requst error %lld: %s\n", \
			         js_get_int( err_block, "error_code" ), js_get_str( err_block, "error_msg" ) );
		return -3;
	}

		json = json_object_get( rsp, "items" );
		json_array_foreach( json, index, rsp )
		{
			ids[offset + index] = json_integer_value( rsp );
		}

/*		json_decref(json);*/
		offset += LIMIT_G;
		api_request_pause();
	}
	while ( grp.sub_count > offset );

	return 1;
}

void
bf_help_print()
{
	puts("Usage: blacklist_finder [OPTIONS] <grp.");
	puts("");
	puts("Options:");
	puts("  -h             show help");
	puts("  -T             generate link for getting a token");
	puts("  -t TKN       give a valid TKN without header \"&access_token=\". If TKN is zero then anonymous access given");
}

int
user_subs( long long id, char * output_file )
{
	struct user_numbers un;
	struct user_numbers * numb = &un;
	un.offset = 0;
		struct crl_st wk_crl_st;
		struct crl_st * cf = &wk_crl_st;
	/* Requesting data */
	char url[BUFSIZ];
	sprintf( url, "%s/users.get?v=%s&user_id=%lld&fields=screen_name%s", \
	         REQ_HEAD, API_VER, id, TKN );

		vk_get_request( url, grp.curl, cf );
		char * r = malloc( cf->size + 1 );
		sprintf( r, "%s", cf->payload );
		r[ cf->size ] = 0;
		if ( cf->payload != NULL )
			free( cf->payload );

		/* Parsing json */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( r != NULL )
			free(r);
		if ( !json )
		{
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err.line, json_err.text );
			return -2;
		}

	json_auto_t * rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		json_t * err_block;
		err_block = json_object_get( json, "error" );
		if ( err_block )
			fprintf( stderr, "Requst error %lld: %s\n", \
			         js_get_int( err_block, "error_code" ), js_get_str( err_block, "error_msg" ) );
		return -3;
	}

	FILE * logfile = fopen( output_file, "w" );
	json_auto_t * json_ar = json_array_get( rsp, 0 );
	fprintf( logfile, "%s:\"%s %s\"\n\n", js_get_str( json_ar, "screen_name" ), js_get_str( json_ar, "first_name" ), js_get_str( json_ar, "last_name" ) );
	do
	{
		if ( cycle_users( id, numb, logfile ) != 0 )
		{
			fclose(logfile);
			return -4;
		}


		un.offset += LIMIT_S;
	}
	while ( un.count > un.offset );

/*	json_decref(json);*/
	api_request_pause();
	fclose( logfile );
	return 0;
}

int
cycle_users ( long long id, struct user_numbers * numb, FILE * logfile )
{
	char sub_name[BUFSIZ];
	char sub_scrname[BUFSIZ];
	char url[BUFSIZ];
	struct crl_st wk_crl_st;
	struct crl_st * cf = &wk_crl_st;

	sprintf( url, "%s/groups.get?v=%s&user_id=%lld&offset=%ld&extended=1%s&count=%d", \
	         REQ_HEAD, API_VER, id, numb->offset, TKN, LIMIT_S );

	vk_get_request( url, grp.curl, cf );

	char * r = malloc( cf->size + 1 );
	sprintf( r, "%s", cf->payload );
	r[ cf->size ] = 0;
	if ( cf->payload != NULL )
		free( cf->payload );

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( r != NULL )
		free(r);
	if ( !json )
	{
		fprintf( stderr, "JSON groups.get parsing error.\n%d:%s\n", json_err.line, json_err.text );
/*		json_decref(json);*/
		return -2;
	}

	json_auto_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		json_t * err_block;
		err_block = json_object_get( json, "error" );
		if ( err_block )
			fprintf( stderr, "Requst error %lld: %s\n", \
			         js_get_int( err_block, "error_code" ), js_get_str( err_block, "error_msg" ) );
		return -3;
	}

	if ( numb->offset == 0 )
	{
		numb->count = (long) js_get_int( rsp, "count" );
		printf( "UID %10lld is subscribed to %ld pages.", id, numb->count );
	}

	size_t index;
	json_auto_t * json_ar = json_object_get( rsp, "items" );
	json_array_foreach( json_ar, index, rsp )
	{
		sprintf( sub_scrname, "%s", js_get_str( rsp, "screen_name" ) );
		sprintf( sub_name, "%s", js_get_str( rsp, "name" ) );
		fprintf( logfile, "%s:::\"%s\"\n", sub_scrname, sub_name );
	}
/*	json_decref(json);*/
	api_request_pause();
	return 0;
}
