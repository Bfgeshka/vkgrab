#include <curl/curl.h>
#include <errno.h>
#include <jansson.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "methods.h"
#include "curl_req.h"

char TOKEN[BUF_S] = TOKEN_HEAD;

void
check_token()
{
	if ( strlen( TOKEN ) != strlen( CONST_TOKEN ) )
		sprintf( TOKEN, "%s", CONST_TOKEN );
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

short
user( char * name, CURL * curl )
{
	strncpy( acc.screenname, name, BUF_S/2 );
	acc.usr_ok = 0;

	char * url = NULL;
	url = malloc( BUF_S );
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
	strncpy( acc.usr_fname, js_get_str( el, "first_name" ), BUF_S/2 );
	strncpy( acc.usr_lname, js_get_str( el, "last_name" ), BUF_S/2 );

	return acc.usr_ok;
}

short
group( char * name, CURL * curl )
{
	acc.grp_ok = 0;
	strcpy( acc.screenname, name );

	char * url = NULL;
	url = malloc( BUF_S );
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
	strncpy( acc.grp_name, js_get_str( el, "name" ), BUF_S/2 );
	strncpy( acc.grp_type, js_get_str( el, "type" ), BUF_S/4 );

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
	char data_type[5][6] = { "photo", "link", "doc", "video" };

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

		/* If video: 3 */
		if ( strcmp( att_type, data_type[3] ) == 0 && types.video == 1 )
		{
			output_json = json_object_get( att_elem, data_type[3] );
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
	puts("  -t TOKEN             give a valid token without header \"&access_token=\". If TOKEN is zero then anonymous access given");
	puts("  -u USER              ignore group with same screenname");
	puts("  -g GROUP             ignore user with same screenname");
	puts("  -yv, -yd, -yp   allows downloading of video, documents or pictures");
	puts("  -nv, -nd, -np   forbids downloading of video, documents or pictures\n");
	puts("Notice: if both USER and GROUP do exist, group id proceeds");
}

void
api_request_pause()
{
	if ( usleep( ( unsigned int ) USLEEP_INT ) != 0 )
		puts( "Sleep error." );
}


size_t
get_albums( CURL * curl )
{
	char * url = malloc( BUF_S );
	char * addit_request = malloc( BUF_S );

	/* Wall album is hidden for groups */
	if ( acc.id < 0 )
		sprintf( addit_request, "&album_ids=-7" );
	else
		addit_request[0] = '\0';

	/* getting response */
	sprintf( url, "https://api.vk.com/method/photos.getAlbums?owner_id=%lld&need_system=1%s&v=%s%s",
	         acc.id, TOKEN, api_ver, addit_request );
	char * r = vk_get_request( url, curl );
	free( url );
	free( addit_request );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON album parsing error.\n%d:%s\n", json_err.line, json_err.text );
		return 0;
	}

	/* finding response */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "Album JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
		return 0;
	}

	/* Getting number of albums */
	long long counter = js_get_int( rsp, "count" );

	/* getting albums metadata */
	if ( counter > 0 )
	{
		json_t * al_items;
		al_items = json_object_get( rsp, "items" );
		albums = malloc( counter * sizeof( struct data_album ) );
		printf( "\nAlbums: %lld.\n", counter );

		json_t * el;
		size_t index;
		json_array_foreach( al_items, index, el )
		{
			albums[index].aid = js_get_int( el, "id" );
			albums[index].size = js_get_int( el, "size" );
			strncpy( albums[index].title, js_get_str( el, "title" ), BUF_S );
			photos_count += albums[index].size;
		}
	}
	else
		puts( "No albums found." );

	return counter;
}

long long
get_id( int argc, char ** argv, CURL * curl )
{
	acc.usr_ok = 1;
	acc.grp_ok = 1;

	int t;

	if ( argc == 1 )
	{
		help_print();
		return 0;
	}

	else if ( argc == 2 )
	{
		if ( argv[1][0] == '-' )
		{
			if ( argv[1][1] == 'h' )
				help_print();

			else if ( argv[1][1] == 'T' )
				fprintf(stdout,
				        "https://oauth.vk.com/authorize?client_id=%d&scope=%s&display=page&response_type=token\n",
				        APPLICATION_ID, permissions);

			return 0;
		}
		else
		{
			user( argv[1], curl );
			group( argv[1], curl );
		}
	}
	else
		for ( t = 0; t < argc; ++t )
		{
			if ( argv[t][0] == '-' )
			{
				if ( argv[t][1] == 'u' )
					user( argv[t+1], curl );
				if ( argv[t][1] == 'g' )
					group( argv[t+1], curl );
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
				if ( argv[t][1] == 'n' )
				{
					if ( argv[t][2] == 'p' )
						types.pictr = 0;
					else if ( argv[t][2] == 'd' )
						types.docmt = 0;
					/*
					else if ( argv[t][2] == 'a' )
						types.audio = 0;
					*/
					else if ( argv[t][2] == 'v' )
						types.video = 0;
					else
					{
						help_print();
						return 0;
					}
				}
				if ( argv[t][1] == 'y' )
				{
					if ( argv[t][2] == 'p' )
						types.pictr = 1;
					else if ( argv[t][2] == 'd' )
						types.docmt = 1;
					/*
					else if ( argv[t][2] == 'a' )
						types.audio = 1;
					*/
					else if ( argv[t][2] == 'v' )
						types.video = 1;
					else
					{
						help_print();
						return 0;
					}
				}
				if ( argv[t][1] == 'h' )
				{
					help_print();
					return 0;
				}
			}
			if ( ( t == argc - 1 ) && ( acc.usr_ok == 1 ) && ( acc.grp_ok == 1 ) )
			{
				user( argv[t], curl );
				group( argv[t], curl );
			}
		}

	/* Info out */
	if ( acc.grp_ok == 0 )
	{
		printf( "Group: %s (%s).\nGroup ID: %lld.\nType: %s.\n\n",
		        acc.grp_name, acc.screenname, acc.id, acc.grp_type);
		return acc.id;
	}
	else if ( acc.usr_ok == 0 )
	{
		printf( "User: %s %s (%s).\nUser ID: %lld.\n\n",
		        acc.usr_fname, acc.usr_lname, acc.screenname, acc.id );
		return acc.id;
	}

	else
	{
		help_print();
		return 1;
	}
	/* failure */
	return 0;
}

void
get_albums_files( size_t arr_size, char * idpath, CURL * curl )
{
	char * url = malloc( BUF_S );
	char * curpath = malloc( BUF_S );
	char * alchar = malloc( BUF_S );
	char * dirchar = malloc( BUF_S );
	unsigned i;

	for( i = 0; i < arr_size; ++i )
	{
		printf( "Album %u/%zu, id: %lld \"%s\" contains %lld photos.\n",
		        i + 1, arr_size, albums[i].aid, albums[i].title, albums[i].size );

		if ( albums[i].size > 0 )
		{
			int offset;
			int times = albums[i].size / LIMIT_A;
			for ( offset = 0; offset <= times; ++offset )
			{
				/* common names for service albums */
				if ( albums[i].aid == -6 )
					sprintf( alchar, DIRNAME_ALB_PROF );
				else if ( albums[i].aid == -7 )
					sprintf( alchar, DIRNAME_ALB_WALL );
				else if ( albums[i].aid == -15 )
					sprintf( alchar, DIRNAME_ALB_SAVD );
				else
					sprintf( alchar, "%lld[%s]", albums[i].aid, albums[i].title );

				/* creating request */
				sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d%s&v=%s",
				         acc.id, albums[i].aid, offset * LIMIT_A, TOKEN, api_ver );
				char * r = vk_get_request( url, curl );

				/* creating album directory */
				fix_filename( alchar );
				sprintf( dirchar, "%s/%s", idpath, alchar );
				if ( mkdir( dirchar, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
					if ( errno != EEXIST )
						fprintf( stderr, "mkdir() error (%d).\n", errno );

				/* parsing json */
				json_t * json;
				json_error_t json_err;
				json = json_loads( r, 0, &json_err );
				if ( !json )
					fprintf( stderr, "JSON photos.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

				/* finding response */
				json_t * rsp;
				rsp = json_object_get( json, "response" );
				if ( !rsp )
					fprintf( stderr, "Album error.\n" );

				/* iterations in array */
				size_t index;
				json_t * el;
				json_t * items;
				items = json_object_get( rsp, "items" );
				json_array_foreach( items, index, el )
				{
					dl_photo( dirchar, curpath, el, curl, NULL, -1, -1 );
				}
			}
		}

		api_request_pause();
	}

	free( alchar );
	free( curpath );
	free( url );
	free( dirchar );
}

void
get_comments( char * dirpath, char * filepath, CURL * curl, FILE * logfile, long long post_id  )
{
	char * url = malloc( BUF_S );

	/* Loop start */
	long long offset = 0;
	long long posts_count = 0;
	do
	{
		/* Forming request */
		sprintf( url, "https://api.vk.com/method/wall.getComments?owner_id=%lld&extended=0&post_id=%lld&count=%d&offset=%lld%s&v=%s",
		         acc.id, post_id, LIMIT_C, offset, TOKEN, api_ver );
		char * r = vk_get_request( url, curl );

		/* Parsing json */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( !json )
			fprintf( stderr, "JSON wall.getComments parsing error.\n%d:%s\n", json_err.line, json_err.text );

		/* Finding response */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if ( !rsp )
		{
			fprintf( stderr, "Comment error: " );
			rsp = json_object_get( json, "error" );
			const char * error_message = js_get_str( rsp, "error_msg" );
			fprintf( stderr, "%s\n", error_message );
			/* Do not repeat trying if comments are restricted */
			if ( strcmp( error_message, "Access to post comments denied" ) == 0 )
				types.comts = 0;
		}

		/* Getting comments count */
		if ( offset == 0 )
			posts_count = js_get_int( rsp, "count" );

		/* Iterations in array */
		size_t index;
		json_t * el;
		json_t * items;
		items = json_object_get( rsp, "items" );
		json_array_foreach( items, index, el )
		{
			long long c_id = js_get_int( el, "id" );
			long long c_date = js_get_int( el, "date" );
			fprintf( logfile, "COMMENT %lld: EPOCH: %lld\nCOMMENT %lld: TEXT:\n%s\n",
			        c_id, c_date, c_id, js_get_str( el, "text" ) );

			/* Searching for attachments */
			json_t * att_json;
			att_json = json_object_get( el, "attachments" );
			if ( att_json )
				parse_attachments( dirpath, filepath, att_json, curl, logfile, post_id, c_id );
		}

		offset += LIMIT_C;
		api_request_pause();
	}
	while( posts_count - offset > 0 );

	free( url );
}

void
get_wall( char * idpath, CURL * curl )
{
	/* Char allocation */
	char * url = malloc( BUF_S );
	char * curpath = malloc( BUF_S );
	char * posts_path = malloc( BUF_S );
	char * attach_path = malloc( BUF_S );

	sprintf( curpath, "%s/%s", idpath, DIRNAME_WALL );
	sprintf( posts_path, "%s/%s", idpath, FILNAME_POSTS );
	FILE * posts = fopen( posts_path, "w" );

	if ( mkdir( curpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* Loop start */
	long long offset = 0;
	long long posts_count = 0;
	do
	{
		sprintf( url, "https://api.vk.com/method/wall.get?owner_id=%lld&extended=0&count=%d&offset=%lld%s&v=%s",
		         acc.id, LIMIT_W, offset, TOKEN, api_ver );
		char * r = vk_get_request( url, curl );

		/* Parsing json */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( !json )
			fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

		/* Finding response */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if ( !rsp )
		{
			fprintf( stderr, "Wall error.\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
		}

		/* Getting posts count */
		if ( offset == 0 )
		{
			posts_count = js_get_int( rsp, "count" );
			printf( "Wall posts: %lld.\n", posts_count );
		}

		/* Iterations in array */
		size_t index;
		json_t * el;
		json_t * items;
		items = json_object_get( rsp, "items" );
		json_array_foreach( items, index, el )
		{
			long long p_id = js_get_int( el, "id" );
			long long p_date = js_get_int( el, "date" );
			fprintf( posts, "ID: %lld\nEPOCH: %lld\nTEXT: %s\n", p_id, p_date, js_get_str( el, "text" ) );


			/* Searching for attachments */
			json_t * att_json;
			att_json = json_object_get( el, "attachments" );
			if ( att_json )
				parse_attachments( curpath, attach_path, att_json, curl, posts, p_id, -1 );

			/* Searching for comments */
			json_t * comments;
			comments = json_object_get( el, "comments" );
			if ( comments )
			{
				long long comm_count = js_get_int( comments, "count" );
				if ( comm_count > 0 )
				{
					fprintf( posts, "COMMENTS: %lld\n", comm_count );
					if (types.comts == 1)
						get_comments( curpath, attach_path, curl, posts, p_id );
				}
			}

			/* Checking if repost */
			json_t * repost_json;
			repost_json = json_object_get( el, "copy_history" );
			if ( repost_json )
			{
				json_t * rep_elem;
				rep_elem = json_array_get( repost_json, 0 );
				if ( rep_elem )
				{
					fprintf( posts, "REPOST FROM: %lld\nTEXT: %s\n",
					        js_get_int( rep_elem, "from_id" ), js_get_str( rep_elem, "text" ) );
					json_t * rep_att_json;
					rep_att_json = json_object_get( rep_elem, "attachments" );
					if ( rep_att_json )
						parse_attachments( curpath, attach_path, rep_att_json, curl, posts, p_id, -1 );
				}
			}

			/* Finishing */
			fprintf( posts, LOG_POSTS_DIVIDER );
		}

		offset += LIMIT_W;
		api_request_pause();
	}
	while( posts_count - offset > 0 );

	free( url );
	free( curpath );
	free( posts_path );
	free( attach_path );
	fclose( posts );
}

void
get_docs( char * idpath, CURL * curl )
{
	/* char allocation */
	char * url = malloc( BUF_S );
	char * dirpath = malloc( BUF_S );
	char * doc_path = malloc( BUF_S );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_DOCS );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* Sending API request docs.get */
	sprintf( url, "https://api.vk.com/method/docs.get?owner_id=%lld%s&v=%s", acc.id, TOKEN, api_ver );
	char * r = vk_get_request( url, curl );
	free( url );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON docs.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* finding response */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "Documents JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
	}

	/* Show documents count */
	printf("\nDocuments: %lld.\n", js_get_int( rsp, "count" ));

	/* Loop init */
	size_t index;
	json_t * el;
	json_t * items;
	items = json_object_get( rsp, "items" );
	json_array_foreach( items, index, el )
	{
		if ( index != 0 )
			dl_document( dirpath, doc_path, el, curl, NULL, -1, -1 );
	}

	free( dirpath );
	free( doc_path );
}

void
get_friends( char * idpath, CURL * curl )
{
	char * url = malloc( BUF_S );
	char * outfl = malloc( BUF_S );

	sprintf( outfl, "%s/%s", idpath, FILNAME_FRIENDS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/friends.get?user_id=%lld&order=domain&fields=domain%s&v=%s",
	         acc.id, TOKEN, api_ver );
	char * r = vk_get_request( url, curl );
	free( url );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* finding response */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "Friends JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
	}

	printf( "\nFriends: %lld.\n", js_get_int(rsp, "count") );

	/* iterations in array */
	size_t index;
	json_t * items;
	items = json_object_get( rsp, "items" );
	json_t * el;
	json_array_foreach( items, index, el )
	{
		fprintf( outptr, "%s\n", js_get_str( el, "domain" ) );
	}

	free( outfl );
	fclose( outptr );
}

void
get_groups( char * idpath, CURL * curl )
{
	char * url = malloc( BUF_S );
	char * outfl = malloc( BUF_S );

	sprintf( outfl, "%s/%s", idpath, FILNAME_GROUPS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/groups.get?user_id=%lld&extended=1%s&v=%s",
	         acc.id, TOKEN, api_ver );
	char * r = vk_get_request( url, curl );
	free( url );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON groups.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* finding response */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "Groups JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
	}

	printf( "Comminities: %lld.\n", js_get_int( rsp, "count" ) );

	/* iterations in array */
	size_t index;
	json_t * el;
	json_t * items;
	items = json_object_get( rsp, "items" );
	json_array_foreach( items, index, el )
	{
		if ( index != 0 )
			fprintf( outptr, "%s\n", js_get_str( el, "screen_name" ) );
	}

	free( outfl );
	fclose( outptr );
}

void
get_videos( char * idpath, CURL * curl )
{
	char * url = malloc( BUF_S );
	char * dirpath = malloc( BUF_S );
	char * vidpath = malloc( BUF_S );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_VIDEO );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* creating log file with external links */
	char * vid_log_path;
	vid_log_path = malloc( BUF_S );
	sprintf( vid_log_path, "%s/%s", idpath, FILNAME_VIDEOS );
	FILE * vid_log = fopen( vid_log_path, "w" );

	long long vid_count = 0;

	/* Loop init */
	int offset = 0;
	int times = 0;
	for ( ; offset <= times; ++offset )
	{
		/* creating request */
		sprintf( url, "https://api.vk.com/method/video.get?owner_id=%lld&offset=%d&count=%d%s&v=%s",
		         acc.id, offset * LIMIT_V, LIMIT_V, TOKEN, api_ver );
		char * r;
		r = vk_get_request( url, curl );

		/* JSON init */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( !json )
			fprintf( stderr, "JSON video.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

		/* finding response */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if ( !rsp )
		{
			fprintf( stderr, "Videos JSON error.\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
		}

		if ( offset == 0 )
		{
			vid_count = js_get_int( rsp, "count" );
			printf( "\nVideos: %lld.\n", vid_count );
		}

		/* iterations in array */
		size_t index;
		json_t * el;
		json_t * items;
		items = json_object_get( rsp, "items" );
		json_array_foreach( items, index, el )
		{
			dl_video( dirpath, vidpath, el, curl, vid_log, -1, -1 );
		}

		times = vid_count / LIMIT_V;
	}

	free( dirpath );
	free( vid_log_path );
	free( vidpath );
	free( url );
}
