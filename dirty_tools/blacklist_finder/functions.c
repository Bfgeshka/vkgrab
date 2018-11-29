#include <curl/curl.h>
#include <jansson.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "functions.h"
#include "../../config.h"
#include "../../src/curl_req.h"
#include "../../src/methods.h"
#include "../../src/utils.h"

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
			switch( argv[1][1] )
			{
				case 'h':
					bf_help_print();
					return 0;

				case 'T':
					fprintf( stdout,
					    "https://oauth.vk.com/authorize?client_id=%d&scope=%s&display=page&response_type=token\n",
					    APPLICATION_ID, PERMISSIONS );
					return 0;

				default:
					fputs( "Unknown argument, abort.", stderr );
					return -1;
			}
		else
			stringset( grp.name_scrn, "%s", argv[1] );
	}

	/* Several arguments */
	else
		for ( t = 0; t < argc; ++t )
		{
			if ( argv[t][0] == '-' )
			{
				switch ( argv[t][1] )
				{
					case 't':
					{
						if ( argv[t+1] != NULL )
						{
							if ( atoi(argv[t+1]) != 0 )
								stringset( &TOKEN, "%s%s", TOKEN_HEAD, argv[t+1] );
							else
								stringset( &TOKEN, "%c", '\0' );
						}
						else
						{
							fputs( "Bad argument, abort.", stderr );
							return -1;
						}

						break;
					}

					case 'h':
						bf_help_print();
						return -1;

					default:
						fputs( "Bad argument, abort.", stderr );
						return -1;
				}
			}
			else
				stringset( grp.name_scrn, "%s", argv[t] );
		}

	/* Requesting data */
	sstring * url = construct_string(2048);
	stringset( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld%s", \
	    REQ_HEAD, API_VER, grp.name_scrn->c, offset, TOKEN.c );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err->line, json_err->text );

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

	api_request_pause();
	return 1;
}

int
group_memb( long long * ids )
{
	long long offset = 0;
	size_t index;

	/* Requesting data */
	do
	{
		sstring * url = construct_string(2048);
		stringset( url, "%s/groups.getMembers?v=%s&group_id=%s&offset=%lld%s", \
		    REQ_HEAD, API_VER, grp.name_scrn->c, offset, TOKEN.c );

		json_error_t * json_err = NULL;
		json_t * json = make_request( url, json_err );
		free_string(url);
		if ( !json )
		{
			if ( json_err )
				fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err->line, json_err->text );

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

		json = json_object_get( rsp, "items" );
		json_array_foreach( json, index, rsp )
		{
			ids[offset + index] = json_integer_value( rsp );
		}

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
	puts("  -t TOKEN       give a valid TOKEN without header \"&access_token=\". If TOKEN is zero then anonymous access given");
}

int
user_subs( long long id, sstring * output_file )
{

	/* Requesting data */
	sstring * url = construct_string(2048);
	stringset( url, "%s/users.get?v=%s&user_id=%lld&fields=screen_name,last_seen%s", \
	    REQ_HEAD, API_VER, id, TOKEN.c );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON groups.getMembers parsing error.\n%d:%s\n", json_err->line, json_err->text );

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

	FILE * logfile = fopen( output_file->c, "w" );
	json_auto_t * json_ar = json_array_get( rsp, 0 );
	fprintf( logfile, "%s:\"%s %s\"\n", js_get_str( json_ar, "screen_name" ), js_get_str( json_ar, "first_name" ), js_get_str( json_ar, "last_name" ) );

	json_auto_t * lseen = json_object_get( json_ar, "last_seen" );
	fprintf( logfile, ":ls:%lld\n\n", js_get_int( lseen, "time" ) );

	struct user_numbers numb;
	numb.offset = 0;

	do
	{
		if ( cycle_users( id, &numb, logfile ) != 0 )
		{
			fclose(logfile);
			return -4;
		}

		numb.offset += LIMIT_S;
	}
	while ( numb.count > numb.offset );

	api_request_pause();
	fclose(logfile);
	return 0;
}

int
cycle_users( long long id, struct user_numbers * numb, FILE * logfile )
{

	int retvalue = 0;

	sstring * url = construct_string(2048);
	stringset( url, "%s/groups.get?v=%s&user_id=%lld&offset=%ld&extended=1%s&count=%d", \
	    REQ_HEAD, API_VER, id, numb->offset, TOKEN.c, LIMIT_S );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON groups.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

		retvalue = -2;
		goto cycle_users_end_mark;
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

		retvalue = -3;
		goto cycle_users_end_mark;
	}

	if ( numb->offset == 0 )
	{
		numb->count = (long) js_get_int( rsp, "count" );
		printf( "UID %10lld is subscribed to %ld pages.", id, numb->count );
	}

	sstring * sub_name = construct_string(2048);
	sstring * sub_scrname = construct_string(2048);

	size_t index;
	json_auto_t * json_ar = json_object_get( rsp, "items" );
	json_array_foreach( json_ar, index, rsp )
	{
		stringset( sub_scrname, "%s", js_get_str( rsp, "screen_name" ) );
		stringset( sub_name, "%s", js_get_str( rsp, "name" ) );
		fprintf( logfile, "%s:::\"%s\"\n", sub_scrname->c, sub_name->c );
	}

	free_string(sub_name);
	free_string(sub_scrname);

	cycle_users_end_mark:
	api_request_pause();
	return retvalue;
}
