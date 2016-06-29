#include "api.c"
#include <jansson.h>

struct data_user
{
	long long uid;
	long long hidden;
	/* 0 means ok */
	short is_ok;
	char   *  fname;
	char   *  lname;
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

struct data_user user( char * name )
{
	struct data_user usr;
	usr.is_ok = 0;
	char * url;
	url = malloc( bufs );
	strcat( url, "https://api.vk.com/method/users.get?user_ids=\0");
//	sprintf( url, "%s%s", "https://api.vk.com/method/users.get?user_ids=", name );
	strcat( url, name );
//	printf("\n%s\n", url);
	char * r = vk_api(url);


	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON parsing error.\n%d:%s\n", json_err.line, json_err.text );
		usr.is_ok = -1;
		return usr;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Wrong name\n" );
		usr.is_ok = -1;
		return usr;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );


	usr.fname = malloc( bufs );
	usr.lname = malloc( bufs );

	usr.uid = js_get_int( el, "uid" );
	usr.hidden = js_get_int ( el, "hidden" );
	strncpy( usr.fname, js_get_str( el, "first_name" ), bufs );
	strncpy( usr.lname, js_get_str( el, "last_name" ), bufs);

	return usr;
}
