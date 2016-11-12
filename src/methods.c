#include "curl_req.c"
#include <jansson.h>

struct data_account
{
	long long id;
	char screenname[bufs/2];
	char usr_fname[bufs/2];
	char usr_lname[bufs/2];
	char grp_name[bufs/2];
	char grp_type[bufs/4];

	/* 0 means ok: */
	short grp_ok;
	short usr_ok;
};
struct data_account acc;

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
struct control_datatypes types;

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

short
user( char * name, CURL * curl )
{
	strncpy( acc.screenname, name, bufs/2 );
	acc.usr_ok = 0;

	char * url = NULL;
	url = malloc( bufs );
	sprintf( url, "https://api.vk.com/method/users.get?user_ids=%s&v=%s", name, api_ver );
	char * r = vk_get_request(url, curl);
	free(url);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON users.get parsing error.\n%d:%s\n", json_err.line, json_err.text );
		acc.usr_ok = -1;
		return acc.usr_ok;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such user (%s).\n", acc.screenname );
		acc.usr_ok = -2;
		return acc.usr_ok;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );

	/* filling struct */
	acc.id = js_get_int( el, "id" );
	strncpy( acc.usr_fname, js_get_str( el, "first_name" ), bufs/2 );
	strncpy( acc.usr_lname, js_get_str( el, "last_name" ), bufs/2 );

	return acc.usr_ok;
}

short
group( char * name, CURL * curl )
{
	acc.grp_ok = 0;
	strcpy( acc.screenname, name );

	char * url = NULL;
	url = malloc( bufs );
	sprintf( url, "https://api.vk.com/method/groups.getById?v=%s&group_id=%s", api_ver, name );
	char * r = vk_get_request( url, curl );
	free(url);

	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON groups.getById parsing error.\n%d:%s\n", json_err.line, json_err.text );
		acc.grp_ok = -1;
		return acc.grp_ok;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "No such group (%s).\n", acc.screenname );
		acc.grp_ok = -2;
		return acc.grp_ok;
	}

	/* filling struct */
	json_t * el;
	el = json_array_get( rsp, 0 );
	acc.id = - js_get_int( el, "id" );
	strncpy( acc.grp_name, js_get_str( el, "name" ), bufs/2 );
	strncpy( acc.grp_type, js_get_str( el, "type" ), bufs/4 );

	return acc.grp_ok;
}

void
fix_filename( char * dirty )
{
	size_t name_length = strlen( dirty );
	unsigned i;
	for ( i = 0; i < name_length; ++i )
	{
		if ( ( (dirty[i] & 0xC0) != 0x80 ) && (dirty[i] == '/' || dirty[i] == '\\') )
			dirty[i] = '_';
	}
}

void /* if no post_id, then set it to '-1', FILE * log replace with NULL */
dl_photo( char * dirpath, char * filepath, json_t * photo_el, CURL * curl, FILE * log, long long post_id, long long comm_id )
{
	long long pid;
	json_t * biggest;

	pid = js_get_int( photo_el, "id" );
	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
			fprintf( log, "COMMENT %lld: ATTACH: PHOTO %lld\n", comm_id, pid);
		else
			fprintf( log, "ATTACH: PHOTO FOR %lld: %lld\n", post_id, pid);
	}


	biggest = json_object_get( photo_el, "photo_2560" );
	if ( !biggest )
	{
		biggest = json_object_get( photo_el, "photo_1280" );
		if ( !biggest )
		{
			biggest = json_object_get( photo_el, "photo_807" );
			if ( !biggest )
			{
				biggest = json_object_get( photo_el, "photo_604" );
				if ( !biggest )
				{
					biggest = json_object_get( photo_el, "photo_130" );
					if ( !biggest )
						biggest = json_object_get( photo_el, "photo_75" );
				}
			}
		}
	}

	/* downloading */
	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
			sprintf( filepath, "%s/%lld_%lld:%lld_%lld.jpg", dirpath, acc.id, post_id, comm_id, pid );
		else
			sprintf( filepath, "%s/%lld_%lld_%lld.jpg", dirpath, acc.id, post_id, pid );
	}
	else
		sprintf( filepath, "%s/%lld-%lld.jpg", dirpath, acc.id, pid );

	vk_get_file( json_string_value( biggest ), filepath, curl );
}

void /* if no post_id, then set it to '-1', FILE * log replace with NULL */
dl_document( char * dirpath, char * filepath, json_t * doc_el, CURL * curl, FILE * log, long long post_id, long long comm_id )
{
	long long did;
	did = js_get_int( doc_el, "id" );

	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
		{
			fprintf( log, "COMMENT %lld: ATTACH: DOCUMENT %lld (\"%s\")\n", comm_id, did, js_get_str( doc_el, "title" ));
			sprintf( filepath, "%s/%lld_%lld:%lld_%lld.%s", dirpath, acc.id, post_id, comm_id, did, js_get_str( doc_el, "ext" ) );
		}
		else
		{
			fprintf( log, "ATTACH: DOCUMENT FOR %lld: %lld (\"%s\")\n", post_id, did, js_get_str( doc_el, "title" ));
			sprintf( filepath, "%s/%lld_%lld_%lld.%s", dirpath, acc.id, post_id, did, js_get_str( doc_el, "ext" ) );
		}
	}
	else
		sprintf( filepath, "%s/%lld_%lld.%s", dirpath, acc.id, did, js_get_str( doc_el, "ext" ) );

	vk_get_file( js_get_str( doc_el, "url" ), filepath, curl );
}

void
dl_audiofile( char * dirpath, char * filepath, json_t * aud_el, CURL * curl, FILE * log, long long post_id, long long comm_id )
{
	long long aid;
	aid = js_get_int( aud_el, "id" );
	char * dirty = malloc( bufs/2 );
	char * tr_art = malloc( a_field );
	char * tr_tit = malloc( a_field );

	strncpy( tr_art, js_get_str( aud_el, "artist" ), a_field );
	strncpy( tr_tit, js_get_str( aud_el, "title" ), a_field );

	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
		{
			fprintf( log, "COMMENT %lld: ATTACH: TRACK %lld (\"%s - %s\")\n", comm_id, aid, tr_art, tr_tit );
			sprintf( dirty, "%lld_%lld:%lld_%s - %s_%lld.mp3", acc.id, post_id, comm_id, tr_art, tr_tit, aid );
		}
		else
		{
			fprintf( log, "ATTACH: TRACK FOR %lld: %lld (\"%s - %s\")\n", post_id, aid, tr_art, tr_tit );
			sprintf( dirty, "%lld_%lld_%s - %s_%lld.mp3", acc.id, post_id, tr_art, tr_tit, aid );
		}

		fix_filename( dirty );
		sprintf( filepath, "%s/%s", dirpath, dirty );
	}
	else
	{
		sprintf( dirty, "%lld_%s - %s _%lld.mp3", acc.id, tr_art, tr_tit, aid );
		fix_filename( dirty );
		sprintf( filepath, "%s/%s", dirpath, dirty );
	}

	free( tr_art );
	free( tr_tit );
	free( dirty );

	vk_get_file( js_get_str( aud_el, "url" ), filepath, curl );
}

void
dl_video( char * dirpath, char * filepath, json_t * vid_el, CURL * curl, FILE * log, long long post_id, long long comm_id )
{
	long long vid;
	const char * fileurl;

	vid = js_get_int( vid_el, "id" );

	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
			fprintf( log, "COMMENT %lld: ATTACH: VIDEO %lld, (\"%s\")\n", comm_id, vid, js_get_str( vid_el, "title" ) );
		else
			fprintf( log, "ATTACH: VIDEO FOR %lld: %lld, (\"%s\")\n", post_id, vid, js_get_str( vid_el, "title" ) );
	}
	else
		fprintf( log, "\n%lld:(\"%s\")", vid, js_get_str( vid_el, "title" ) );

	json_t * v_block;
	v_block = json_object_get( vid_el, "files" );

	if ( v_block )
	{
		json_t * v_link;
		v_link = json_object_get( v_block, "external" );
		if ( v_link )
		{
			fileurl = json_string_value( v_link );
			if ( post_id > 0 )
				fprintf( log, "ATTACH: VIDEO EXTERNAL LINK: %s\n", fileurl );
			else
				fprintf( log, " %s", fileurl );
		}
		else
		{
			v_link = json_object_get( v_block, "mp4_1080" );
			if ( !v_link )
			{
				v_link = json_object_get( v_block, "mp4_720" );
				if ( !v_link )
				{
					v_link = json_object_get( v_block, "mp4_480" );
					if ( !v_link )
					{
						v_link = json_object_get( v_block, "mp4_360" );
						if ( !v_link )
						{
							v_link = json_object_get( v_block, "mp4_240" );
							if ( !v_link )
								v_link = json_object_get( v_block, "mp4_144" );
						}
					}
				}
			}

			if ( v_link )
			{
				if ( post_id > 0 )
				{
					if ( comm_id > 0 )
						sprintf( filepath, "%s/%lld:%lld_%lld.mp4", dirpath, post_id, comm_id, vid );
					else
						sprintf( filepath, "%s/%lld_%lld.mp4", dirpath, post_id, vid );
				}
				else
					sprintf( filepath, "%s/%lld.mp4", dirpath, vid );

				vk_get_file( json_string_value( v_link ), filepath, curl );
			}
		}
	}
	else
	{
		v_block = json_object_get( vid_el, "player" );
		fileurl = json_string_value( v_block );
		if ( v_block )
		{
			if ( post_id < 0 )
				fprintf( log, " %s", fileurl );
		}
	}
}

void
parse_attachments( char * dirpath, char * filepath, json_t * input_json, CURL * curl, FILE * logfile, long long post_id, long long comm_id )
{
	size_t att_index;
	json_t * att_elem;
	char data_type[5][6] = { "photo", "link", "doc", "audio", "video" };

	json_array_foreach( input_json, att_index, att_elem )
	{
		const char * att_type = js_get_str( att_elem, "type" );
		json_t * output_json;

		/* If photo: 0 */
		if ( strcmp( att_type, data_type[0] ) == 0 && types.pictr == 1 )
		{
			output_json = json_object_get( att_elem, data_type[0] );
			dl_photo( dirpath, filepath, output_json, curl, logfile, post_id, comm_id );
		}

		/* If link: 1 */
		if ( strcmp( att_type, data_type[1] ) == 0 )
		{
			output_json = json_object_get( att_elem, data_type[1] );
			fprintf( logfile, "ATTACH: LINK_URL: %s\nATTACH: LINK_DSC: %s\n",
			         js_get_str( output_json, "url" ), js_get_str( output_json, "description" ) );
		}

		/* If doc: 2 */
		if ( strcmp( att_type, data_type[2] ) == 0 && types.docmt == 1 )
		{
			output_json = json_object_get( att_elem, data_type[2] );
			dl_document( dirpath, filepath, output_json, curl, logfile, post_id, comm_id );
		}

		/* If audio: 3 */
		if ( strcmp( att_type, data_type[3] ) == 0 && types.audio == 1 )
		{
			output_json = json_object_get( att_elem, data_type[3] );
			dl_audiofile( dirpath, filepath, output_json, curl, logfile, post_id, comm_id );
		}

		/* If video: 4 */
		if ( strcmp( att_type, data_type[4] ) == 0 && types.video == 1 )
		{
			output_json = json_object_get( att_elem, data_type[4] );
			dl_video( dirpath, filepath, output_json, curl, logfile, post_id, comm_id );
		}
	}
}

void
help_print()
{
	puts("Usage: vkgrab [OPTIONS] <USER|GROUP>");
	puts("");
	puts("Options:");
	puts("  -T                   generate link for getting a token");
	puts("  -t TOKEN             give a valid token without header \"&access_token=\"");
	puts("  -u USER              ignore group with same screenname");
	puts("  -g GROUP             ignore user with same screenname");
	puts("  -ya, -yv, -yd, -yp   allows downloading of audio, video, documents or pictures");
	puts("  -na, -nv, -nd, -np   forbids downloading of audio, video, documents or pictures\n");
	puts("Notice: if both USER and GROUP do exist, group id proceeds");
}

void
api_request_pause()
{
	if ( usleep( ( unsigned int ) USLEEP_INT ) != 0 )
		puts( "Sleep error." );
}
