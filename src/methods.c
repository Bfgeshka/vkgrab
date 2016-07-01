#include "api.c"
#include <jansson.h>

struct data_user
{
	long long uid;
	long long hidden;
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
user( char * name )
{
	struct data_user usr;
//	usr.fname = malloc( bufs );
//	usr.lname = malloc( bufs );
//	usr.screenname = malloc( bufs );
	strncpy( usr.screenname, name, bufs );
	usr.is_ok = 0;

	char * url = NULL;
	url = malloc( bufs );
	sprintf(url, "https://api.vk.com/method/users.get?user_ids=%s", name);
	char * r = vk_api(url);
	free(url);

//	printf("%s\n", r);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON user parsing error.\n%d:%s\n", json_err.line, json_err.text );
		usr.is_ok = -1;
		return usr;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such user (%s).\n\n", usr.screenname );
		usr.is_ok = -2;
		return usr;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );


	/* filling struct */
	usr.uid = js_get_int( el, "uid" );
	usr.hidden = js_get_int ( el, "hidden" );
	strncpy( usr.fname, js_get_str( el, "first_name" ), bufs );
	strncpy( usr.lname, js_get_str( el, "last_name" ), bufs);

	return usr;
}

struct data_group
group( char * name )
{
	struct data_group grp;
	grp.is_ok = 0;
	strcpy( grp.screenname, name );

	char * url = NULL;
	url = malloc( bufs );
	strcpy( url, "https://api.vk.com/method/groups.getById?group_id=" );
	strcat( url, name );
	char * r = vk_api(url);
	free(url);

//	printf("%s\n", r);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON group parsing error.\n%d:%s\n", json_err.line, json_err.text );
		grp.is_ok = -1;
		return grp;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such group (%s).\n\n", grp.screenname );
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
