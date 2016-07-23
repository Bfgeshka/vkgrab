#include "api.c"
#include <jansson.h>

struct data_user
{
	long long uid;
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

struct control_datatypes
{
	short audio;
	short docmt;
	short pictr;
	short video;
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

void
fix_filename( char * dirty )
{
	for ( int i = 0; i < strlen( dirty ); ++i )
	{
		if ( dirty[i] == '/' || dirty[i] == ':' || dirty[i] == '\\' )
		{
			dirty[i] = '_';
		}
	}
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

	printf( "%s ", filepath );
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
		sprintf( filepath, "%s/%lld.%s", dirpath, did, js_get_str( doc_el, "ext" ) );


	printf( "%s ", filepath );
	fileurl = js_get_str( doc_el, "url" );
	vk_get_file( fileurl, filepath, curl );
}

void
audiofile( char * dirpath, char * filepath, const char * fileurl, json_t * aud_el, CURL * curl, FILE * log, long long p_id )
{
	long long aid;
	char * dirty = malloc( bufs/2 );
	aid = js_get_int( aud_el, "aid" );

	char * tr_art = malloc( a_field );
	char * tr_tit = malloc( a_field );
	strncpy( tr_art, js_get_str( aud_el, "artist" ), a_field );
	strncpy( tr_tit, js_get_str( aud_el, "title" ), a_field );

	if ( p_id > 0 )
	{
		fprintf( log, "ATTACH: TRACK FOR %lld: %lld (\"%s - %s\")\n", p_id, aid, tr_art, tr_tit );
		sprintf( dirty, "%lld_%s - %s_%lld.mp3", p_id, tr_art, tr_tit, aid );
		fix_filename( dirty );
		sprintf( filepath, "%s/%s", dirpath, dirty );
	}
	else
	{
		sprintf( dirty, "%s - %s _%lld.mp3", tr_art, tr_tit, aid );
		fix_filename( dirty );
		sprintf( filepath, "%s/%s", dirpath, dirty );
	}

	printf( "%s ", filepath );
	fileurl = js_get_str( aud_el, "url" );
	free( dirty );
	vk_get_file( fileurl, filepath, curl );
}

void
vid_file( char * dirpath, char * filepath, const char * fileurl, json_t * vid_el, CURL * curl, FILE * log, long long p_id )
{
	long long vid;
	vid = js_get_int( vid_el, "vid" );

	if ( p_id > 0 )
		fprintf( log, "ATTACH: VIDEO FOR %lld: %lld, (\"%s\")\n", p_id, vid, js_get_str( vid_el, "title" ) );
	else
		fprintf( log, "\n%lld:(\"%s\")", vid, js_get_str( vid_el, "title" ) );



	json_t * v_block;
	v_block = json_object_get( vid_el, "files" );

	json_t * v_link;
	v_link = json_object_get( v_block, "external" );
	if ( v_link )
	{
		fileurl = json_string_value( v_link );
		if ( p_id > 0 )
			fprintf( log, "ATTACH: VIDEO EXTERNAL LINK: %s\n", fileurl );
		else
			fprintf( log, " %s", fileurl );
	}
	else
	{
		v_link = json_object_get( v_block, "mp4_1080" );
		if ( v_link )
			fileurl = json_string_value( v_link );
		else
		{
			v_link = json_object_get( v_block, "mp4_720" );
			if ( v_link )
				fileurl = json_string_value( v_link );
			else
			{
				v_link = json_object_get( v_block, "mp4_480" );
				if ( v_link )
					fileurl = json_string_value( v_link );
				else
				{
					v_link = json_object_get( v_block, "mp4_360" );
					if ( v_link )
						fileurl = json_string_value( v_link );
					else
					{
						v_link = json_object_get( v_block, "mp4_240" );
						if ( v_link )
							fileurl = json_string_value( v_link );
						else printf(" No valid link!\n");
					}
				}
			}
			if ( p_id > 0 )
				sprintf( filepath, "%s/%lld_%lld.mp4", dirpath, p_id, vid );
			else
				sprintf( filepath, "%s/%lld.mp4", dirpath, vid );
		}
		vk_get_file( fileurl, filepath, curl );
	}

}
