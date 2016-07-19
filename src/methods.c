#include "api.c"
#include <jansson.h>

struct data_user
{
	long long uid;
//	long long hidden;
	/* 0 means ok */
	short is_ok;
	char fname[bufs];
	char lname[bufs];
	char screenname[bufs];
};

struct data_group
{
	long long gid;
	long long is_closed;
	/* 0 means ok */
	short is_ok;
	char screenname[bufs];
	char name[bufs];
	char type[bufs];
};

struct data_album
{
	long long aid;
	long long size;
	char title[bufs];
};

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

struct data_user
user( char * name, CURL * curl )
{
	struct data_user usr;
	strncpy( usr.screenname, name, bufs );
	usr.is_ok = 0;

	char * url = NULL;
	url = malloc( bufs );
	sprintf(url, "https://api.vk.com/method/users.get?user_ids=%s", name);
	char * r = vk_get_request(url, curl);
	free(url);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON users.get parsing error.\n%d:%s\n", json_err.line, json_err.text );
		usr.is_ok = -1;
		return usr;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such user (%s).\n", usr.screenname );
		usr.is_ok = -2;
		return usr;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );


	/* filling struct */
	usr.uid = js_get_int( el, "uid" );
//	usr.hidden = js_get_int ( el, "hidden" );
	strncpy( usr.fname, js_get_str( el, "first_name" ), bufs );
	strncpy( usr.lname, js_get_str( el, "last_name" ), bufs);

	return usr;
}

struct data_group
group( char * name, CURL * curl )
{
	struct data_group grp;
	grp.is_ok = 0;
	strcpy( grp.screenname, name );

	char * url = NULL;
	url = malloc( bufs );
	strcpy( url, "https://api.vk.com/method/groups.getById?group_id=" );
	strcat( url, name );
	char * r = vk_get_request(url, curl);
	free(url);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON groups.getById parsing error.\n%d:%s\n", json_err.line, json_err.text );
		grp.is_ok = -1;
		return grp;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such group (%s).\n", grp.screenname );
		grp.is_ok = -2;
		return grp;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );


	/* filling struct */
	grp.gid = js_get_int( el, "gid" );
	grp.is_closed = js_get_int( el, "is_closed" );
	strncpy( grp.name, js_get_str( el, "name" ), bufs );
	strncpy( grp.type, js_get_str( el, "type" ), bufs );
	strncpy( grp.screenname, name, bufs );


	return grp;
}

void /* if no p_id, then set it to '-1', FILE * log replace with NULL */
photo( char * dirpath, char * filepath, const char * fileurl, json_t * photo_el, CURL * curl, FILE * log, long long p_id )
{
	long long pid;
	json_t * biggest;

	pid = js_get_int( photo_el, "pid" );
	if ( p_id > 0 )
		fprintf( log, "ATTACH: PHOTO FOR %lld: %lld\n", p_id, pid);
	biggest = json_object_get( photo_el, "src_xxxbig" );
	if ( biggest )
		fileurl = json_string_value( biggest );
	else
	{
		biggest = json_object_get( photo_el, "src_xxbig" );
		if ( biggest )
			fileurl = json_string_value( biggest );
		else
		{
			biggest = json_object_get( photo_el, "src_xbig" );
			if ( biggest )
				fileurl = json_string_value( biggest );
			else
			{
				biggest = json_object_get( photo_el, "src_big" );
				if ( biggest )
					fileurl = json_string_value( biggest );
				else
				{
					biggest = json_object_get( photo_el, "src" );
					if ( biggest )
						fileurl = json_string_value( biggest );
					else
					{
						biggest = json_object_get( photo_el, "src_small" );
						if ( biggest )
							fileurl = json_string_value( biggest );
					}
				}
			}
		}
	}

	/* downloading */
	if ( p_id > 0 )
		sprintf( filepath, "%s/%lld_%lld.jpg", dirpath, p_id, pid );
	else
		sprintf( filepath, "%s/%lld.jpg", dirpath, pid );

	printf( "%s", filepath );
	vk_get_file( fileurl, filepath, curl );
}

void /* if no p_id, then set it to '-1', FILE * log replace with NULL */
document( char * dirpath, char * filepath, const char * fileurl, json_t * doc_el, CURL * curl, FILE * log, long long p_id )
{
	long long did;
	did = js_get_int( doc_el, "did" );

	if ( p_id > 0 )
	{
		fprintf( log, "ATTACH: DOCUMENT FOR %lld: %lld (\"%s\")\n", p_id, did, js_get_str( doc_el, "title" ));
		sprintf( filepath, "%s/%lld_%lld.%s", dirpath, p_id, did, js_get_str( doc_el, "ext" ) );
	}
	else
	{
		sprintf( filepath, "%s/%lld.%s", dirpath, did, js_get_str( doc_el, "ext" ) );
	//	fprintf( log, "Document %lld: \"%s\"\n", did, js_get_str( doc_el, "title" ));
	}

	printf( "%s", filepath );
	fileurl = js_get_str( doc_el, "url" );
	vk_get_file( fileurl, filepath, curl );
}

void
audiofile( char * dirpath, char * filepath, const char * fileurl, json_t * aud_el, CURL * curl, FILE * log, long long p_id )
{
	long long aid;
	aid = js_get_int( aud_el, "aid" );

	if ( p_id > 0 )
	{
		fprintf( log, "ATTACH: TRACK FOR %lld: %lld (\"%s\")\n", p_id, aid, js_get_str( aud_el, "title" ));
		sprintf( filepath, "%s/%lld_%s - %s _%lld.mp3", dirpath, p_id, js_get_str( aud_el, "artist" ), js_get_str( aud_el, "title" ), aid );
	}
	else
	{
		sprintf( filepath, "%s/%s - %s _%lld.mp3", dirpath, js_get_str( aud_el, "artist" ), js_get_str( aud_el, "title" ), aid );
	//	fprintf( log, "Document %lld: \"%s\"\n", did, js_get_str( doc_el, "title" ));
	}

	printf( "%s", filepath );
	fileurl = js_get_str( aud_el, "url" );
	vk_get_file( fileurl, filepath, curl );
}
