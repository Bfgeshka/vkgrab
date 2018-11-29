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
#include "utils.h"

void
prepare( void )
{
	/* curl handler initialisatiion */
	extern CURL * curl;
	curl = curl_easy_init();
	if ( !curl )
	{
		fprintf( stderr, "Curl initialisation error.\n" );
		exit(EXIT_FAILURE);
	}

	set_token();

	/* Account struct */
	acc.screenname = construct_string(128);
	acc.usr_fname = construct_string(128);
	acc.usr_lname = construct_string(128);
	acc.grp_name = construct_string(128);
	acc.grp_type = construct_string(64);
}

void
set_token ( void )
{
	/* Token */
	newstring( &TOKEN, 256 );
	stringset( &TOKEN, "%s", TOKEN_HEAD );

	sstring * CONSTTOKEN = construct_string(256);
	stringset( CONSTTOKEN, "%s", CONST_TOKEN );

	if ( TOKEN.len != CONSTTOKEN->len )
		stringset( &TOKEN, "%s", CONSTTOKEN->c );

	free_string(CONSTTOKEN);
}

void
destroy_all( void )
{
	free_string(acc.screenname);
	free_string(acc.usr_fname);
	free_string(acc.usr_lname);
	free_string(acc.grp_name);
	free_string(acc.grp_type);

	extern CURL * curl;
	curl_easy_cleanup(curl);
}

int
make_dir( sstring * s, long long id )
{
	/* Naming file metadata */
	sstring * name_descript = construct_string(2048);
	int retvalue = 0;

	if ( acc.usr_ok == 0 )
	{
		stringset( s, "u_%lld", acc.id );
		stringset( name_descript, "%lld: %s: %s %s\n", id, acc.screenname->c, acc.usr_fname->c, acc.usr_lname->c );
	}
	else if ( acc.grp_ok == 0 )
	{
		stringset( s, "c_%lld", acc.id );
		stringset( name_descript, "%lld: %s: %s\n", id, acc.screenname->c, acc.grp_name->c );
	}
	else
	{
		fprintf( stderr, "Screenname is invalid.\n");
		retvalue = 1;
		goto make_dir_close_mark;
	}

	/* Creating dir for current id */
	fix_filename(s->c);
	if ( mkdir( s->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	sstring * name_dsc_path = construct_string(BUFSIZ);
	stringset( name_dsc_path, "%s/%s", s->c, FILNAME_IDNAME );

	FILE * u_name = fopen( name_dsc_path->c, "w" );
	fprintf( u_name, "%s", name_descript->c );
	fclose(u_name);

	free_string(name_dsc_path);

	make_dir_close_mark:
	free_string(name_descript);
	return retvalue;
}

int
readable_date( long long epoch, FILE * log )
{
	sstring * date_invoke = construct_string(512);
	sstring * date_result = construct_string(512);
	int retvalue = 0;

	stringset( date_invoke, "date --date='@%lld'", epoch );

	FILE * piped;
	piped = popen( date_invoke->c, "r" );
	if ( piped == NULL )
	{
		stringset( date_result, "%s", "date get failed" );
		retvalue = -1;
		goto readable_data_ret_mark;
	}

	fgets( date_result->c, date_result->bufsize, piped );
	pclose(piped);

	fprintf( log, "DATE: %s", date_result->c );

	readable_data_ret_mark:
	free_string(date_invoke);
	free_string(date_result);
	return retvalue;
}

long long
js_get_int( json_t * src, char * key )
{
	json_t * elem = json_object_get( src, key );
	return json_integer_value(elem);
}

const char *
js_get_str( json_t * src, char * key )
{
	json_t * elem = json_object_get( src, key );
	return json_string_value(elem);
}

short
user( char * name )
{
	stringset( acc.screenname, "%s", name );
	acc.usr_ok = 0;

	sstring * url = construct_string(2048);
	stringset( url, "%s/users.get?user_ids=%s&v=%s%s", REQ_HEAD, name, API_VER, TOKEN.c );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON users.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

		json_decref(json);
		acc.usr_ok = -1;
		return acc.usr_ok;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "No such user (%s).\n", acc.screenname->c );
		acc.usr_ok = -2;
		return acc.usr_ok;
	}
	json_t * el;
	el = json_array_get( rsp, 0 );

	/* filling struct */
	acc.id = js_get_int( el, "id" );
	stringset( acc.usr_fname, "%s", js_get_str( el, "first_name" ) );
	stringset( acc.usr_lname, "%s", js_get_str( el, "last_name" ) );

	json_decref(json);
	return acc.usr_ok;
}

short
group( char * name )
{
	acc.grp_ok = 0;
	stringset( acc.screenname, "%s", name );

	sstring * url = construct_string(2048);
	stringset( url, "%s/groups.getById?v=%s&group_id=%s%s", REQ_HEAD, API_VER, name, TOKEN.c );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON groups.getById parsing error.\n%d:%s\n", json_err->line, json_err->text );

		json_decref(json);
		acc.grp_ok = -1;
		return acc.grp_ok;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "No such group (%s).\n", acc.screenname->c );
		acc.grp_ok = -2;
		return acc.grp_ok;
	}

	/* filling struct */
	json_t * el;
	el = json_array_get( rsp, 0 );
	acc.id = - js_get_int( el, "id" );
	stringset( acc.grp_name, "%s", js_get_str( el, "name" ) );
	stringset( acc.grp_type, "%s", js_get_str( el, "type" ) );

	json_decref(json);
	return acc.grp_ok;
}

json_t *
make_request ( sstring * url, json_error_t * json_err )
{
	struct crl_st cf;
	vk_get_request( url->c, &cf );

	json_t * json;
	json = json_loads( cf.payload, 0, json_err );
	if ( cf.payload != NULL )
	{
		free(cf.payload);
		return json;
	}
	else
		return NULL;
}

void
fix_filename( char * dirty )
{
	for ( unsigned i = 0; dirty[i] != '\0'; ++i )
		if ( ( ( dirty[i] & 0xC0 ) != 0x80 ) && ( dirty[i] == '/' || dirty[i] == '\\' ) )
			dirty[i] = '_';
}

void /* if no post_id, then set it to '-1', FILE * log replace with NULL */
dl_photo( sstring * dirpath, sstring * filepath, json_t * photo_el, FILE * log, long long post_id, long long comm_id )
{
	long long pid;
	json_t * biggest;

	pid = js_get_int( photo_el, "id" );
	if ( post_id > 0 && log != NULL )
	{
		if ( comm_id > 0 )
			fprintf( log, "COMMENT %lld: ATTACH: PHOTO %lld\n", comm_id, pid );
		else
			fprintf( log, "ATTACH: PHOTO FOR %lld: %lld\n", post_id, pid );
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
			stringset( filepath, "%s/%lld_%lld:%lld_%lld.jpg", dirpath->c, acc.id, post_id, comm_id, pid );
		else
			stringset( filepath, "%s/%lld_%lld_%lld.jpg", dirpath->c, acc.id, post_id, pid );
	}
	else
		stringset( filepath, "%s/%lld-%lld.jpg", dirpath->c, acc.id, pid );

	vk_get_file( json_string_value(biggest), filepath->c );
}

void /* if no post_id, then set it to '-1', FILE * log replace with NULL */
dl_document( sstring * dirpath, sstring * filepath, json_t * doc_el, FILE * log, long long post_id, long long comm_id )
{
	long long did;
	did = js_get_int( doc_el, "id" );

	if ( post_id > 0 )
	{
		if ( comm_id > 0 )
		{
			fprintf( log, "COMMENT %lld: ATTACH: DOCUMENT %lld (\"%s\")\n", comm_id, did, js_get_str( doc_el, "title" ) );
			stringset( filepath, "%s/%lld_%lld:%lld_%lld.%s", dirpath->c, acc.id, post_id, comm_id, did, js_get_str( doc_el, "ext" ) );
		}
		else
		{
			fprintf( log, "ATTACH: DOCUMENT FOR %lld: %lld (\"%s\")\n", post_id, did, js_get_str( doc_el, "title" ) );
			stringset( filepath, "%s/%lld_%lld_%lld.%s", dirpath->c, acc.id, post_id, did, js_get_str( doc_el, "ext" ) );
		}
	}
	else
		stringset( filepath, "%s/%lld_%lld.%s", dirpath->c, acc.id, did, js_get_str( doc_el, "ext" ) );

	vk_get_file( js_get_str( doc_el, "url" ), filepath->c );
}

void
dl_video( sstring * dirpath, sstring * filepath, json_t * vid_el, FILE * log, long long post_id, long long comm_id )
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
						stringset( filepath, "%s/%lld:%lld_%lld.mp4", dirpath->c, post_id, comm_id, vid );
					else
						stringset( filepath, "%s/%lld_%lld.mp4", dirpath->c, post_id, vid );
				}
				else
					stringset( filepath, "%s/%lld.mp4", dirpath->c, vid );

				vk_get_file( json_string_value( v_link ), filepath->c );
			}
		}
	}
	else
	{
		v_block = json_object_get( vid_el, "player" );
		fileurl = json_string_value( v_block );

		if ( v_block )
			if ( post_id < 0 )
				fprintf( log, " %s", fileurl );
	}
}

void
parse_attachments( sstring * dirpath, sstring * filepath, json_t * input_json, FILE * logfile, long long post_id, long long comm_id )
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
			dl_photo( dirpath, filepath, output_json, logfile, post_id, comm_id );
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
			dl_document( dirpath, filepath, output_json, logfile, post_id, comm_id );
		}

		/* If video: 3 */
		if ( strcmp( att_type, data_type[3] ) == 0 && types.video == 1 )
		{
			output_json = json_object_get( att_elem, data_type[3] );
			dl_video( dirpath, filepath, output_json, logfile, post_id, comm_id );
		}
	}
}

void
help_print( void )
{
	puts( "Usage: vkgrab [OPTIONS] <USER|GROUP>" );
	puts( "" );
	puts( "Options:" );
	puts( "  -T                   generate link for getting a token" );
	puts( "  -t TOKEN             give a valid token without header \"&access_token=\". If TOKEN is zero then anonymous access given" );
	puts( "  -u USER              ignore group with same screenname" );
	puts( "  -g GROUP             ignore user with same screenname" );
	puts( "  -yv, -yd, -yp   allows downloading of video, documents or pictures" );
	puts( "  -nv, -nd, -np   forbids downloading of video, documents or pictures\n" );
	puts( "Notice: if both USER and GROUP do exist, group id proceeds" );
}

void
api_request_pause( void )
{
	nanosleep( &(const struct timespec){ 0, NANOSLEEP_INT }, NULL );
}

size_t
get_albums()
{
	sstring * addit_request = construct_string(2048);

	/* Wall album is hidden for groups */
	if ( acc.id < 0 )
		stringset( addit_request, "%s", "&album_ids=-7" );
	else
		addit_request->c[0] = '\0';

	/* getting response */
	sstring * url = construct_string(2048);
	stringset( url, "%s/photos.getAlbums?owner_id=%lld&need_system=1%s&v=%s%s", REQ_HEAD, acc.id, TOKEN.c, API_VER, addit_request->c );
	free_string(addit_request);

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
	{
		if ( json_err )
			fprintf( stderr, "JSON album parsing error.\n%d:%s\n", json_err->line, json_err->text );

		json_decref(json);
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
		albums = malloc( counter * sizeof(struct data_album) );
		printf( "\nAlbums: %lld.\n", counter );

		json_t * el;
		size_t index;
		json_array_foreach( al_items, index, el )
		{
			albums[index].aid = js_get_int( el, "id" );
			albums[index].size = js_get_int( el, "size" );
			albums[index].title = construct_string(1024);
			stringset( albums[index].title, "%s", js_get_str( el, "title" ) );
			photos_count += albums[index].size;
		}
	}
	else
		puts( "No albums found." );

	json_decref(json);
	return counter;
}

long long
get_id( int argc, char ** argv )
{
	acc.usr_ok = 1;
	acc.grp_ok = 1;

	switch( argc )
	{
		case 1:
			goto get_id_print_help;

		case 2:
		{
			if ( argv[1][0] == '-' )
				switch( argv[1][1] )
				{
					case 'h':
						goto get_id_print_help;
					case 'T':
						goto get_id_token_request;
					default:
						goto get_id_invalid_arg;
				}
			else
			{
				user(argv[1]);
				if ( acc.usr_ok != 0 )
					group(argv[1]);
			}

			break;
		}

		default:
		{
			for ( int t = 0; t < argc; ++t )
			{
				if ( argv[t][0] == '-' )
					switch( argv[t][1] )
					{
						case 'u':
						{
							user(argv[t+1]);
							break;
						}

						case 'g':
						{
							group(argv[t+1]);
							break;
						}

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

						case 'n':
						case 'y':
						{
							int value = ( argv[t][1] == 'n' ) ? 0 : 1;
							switch( argv[t][2] )
							{
								case 'p':
								{
									types.pictr = value;
									break;
								}

								case 'd':
								{
									types.docmt = value;
									break;
								}

								case 'v':
								{
									types.video = value;
									break;
								}

								default:
									goto get_id_print_help;
							}

							break;
						}

						case 'h':
							goto get_id_print_help;

						default:
							goto get_id_invalid_arg;
					}
				if ( ( t == argc - 1 ) && ( acc.usr_ok == 1 ) && ( acc.grp_ok == 1 ) )
				{
					user(argv[t]);
					group(argv[t]);
				}
			}

			break;
		}
	}

	/* Info out */
	if ( acc.grp_ok == 0 )
	{
		printf( "Group: %s (%s).\nGroup ID: %lld.\nType: %s.\n\n",
		    acc.grp_name->c, acc.screenname->c, acc.id, acc.grp_type->c );
		return acc.id;
	}
	else if ( acc.usr_ok == 0 )
	{
		printf( "User: %s %s (%s).\nUser ID: %lld.\n\n",
		    acc.usr_fname->c, acc.usr_lname->c, acc.screenname->c, acc.id );
		return acc.id;
	}
	else
		goto get_id_print_help;

	/* Print halp and exit */
	get_id_print_help:
	help_print();
	return 0;

	/* Message about invalid argument and exit */
	get_id_invalid_arg:
	puts("Invalid argument.");
	return 0;

	/* Prink token request link and exit */
	get_id_token_request:
	printf( "https://oauth.vk.com/authorize?client_id=%d&scope=%s&display=page&response_type=token\n", APPLICATION_ID, PERMISSIONS );
	return 0;
}

void
get_albums_files( size_t arr_size, char * idpath )
{
	sstring * url = construct_string(2048);
	sstring * curpath = construct_string(2048);
	sstring * dirchar = construct_string(2048);
	sstring * alchar = construct_string(2048);

	unsigned i;

	for( i = 0; i < arr_size; ++i )
	{
		printf( "Album %u/%zu, id: %lld \"%s\" contains %lld photos.\n",
		        i + 1, arr_size, albums[i].aid, albums[i].title->c, albums[i].size );

		if ( albums[i].size > 0 )
		{
			int offset;
			int times = albums[i].size / LIMIT_A;
			for ( offset = 0; offset <= times; ++offset )
			{
				/* common names for service albums */
				switch( albums[i].aid )
				{
					case  -6:
					{
						stringset( alchar, "%s", DIRNAME_ALB_PROF );
						break;
					}

					case  -7:
					{
						stringset( alchar, "%s", DIRNAME_ALB_WALL );
						break;
					}

					case -15:
					{
						stringset( alchar, "%s", DIRNAME_ALB_SAVD );
						break;
					}

					default:
					{
						stringset( alchar, "alb_%lld_(%u:%zu)_(%lld:p)_(%s)",
						    albums[i].aid, i + 1, arr_size, albums[i].size, albums[i].title->c );
					}
				}

				/* creating album directory */
				fix_filename(alchar->c);
				stringset( dirchar, "%s/%s", idpath, alchar->c );
				if ( mkdir( dirchar->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
					if ( errno != EEXIST )
						fprintf( stderr, "mkdir() error (%d).\n", errno );

				/* creating request */
				stringset( url, "%s/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d%s&v=%s",
				         REQ_HEAD, acc.id, albums[i].aid, offset * LIMIT_A, TOKEN.c, API_VER );

				/* parsing json */
				json_error_t * json_err = NULL;
				json_t * json = make_request( url, json_err );
				if ( !json )
					if ( json_err )
						fprintf( stderr, "JSON photos.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

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
					dl_photo( dirchar, curpath, el, NULL, (long long) index + offset * LIMIT_A + 1, -1 );
				}

				json_decref(json);
			}
		}

		api_request_pause();
	}

	free_string(url);
	free_string(curpath);
	free_string(dirchar);
	free_string(alchar);
}

void
get_comments( sstring * dirpath, sstring * filepath, FILE * logfile, long long post_id  )
{
	sstring * url = construct_string(2048);
	long long offset = 0;
	long long posts_count = 0;

	do
	{
		/* Forming request */
		stringset( url, "%s/wall.getComments?owner_id=%lld&extended=0&post_id=%lld&count=%d&offset=%lld%s&v=%s",
		         REQ_HEAD, acc.id, post_id, LIMIT_C, offset, TOKEN.c, API_VER );

		json_error_t * json_err = NULL;
		json_t * json = make_request( url, json_err );
		if ( !json )
			if ( json_err )
				fprintf( stderr, "JSON wall.getComments parsing error.\n%d:%s\n", json_err->line, json_err->text );

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
			long long epoch = js_get_int( el, "date" );

			fprintf( logfile, "COMMENT %lld: EPOCH: %lld ", c_id, epoch  );
			if ( types.ldate == 1 )
				if ( readable_date( epoch, logfile ) != 0 )
					types.ldate = 0;

			fprintf( logfile, "COMMENT %lld: TEXT: %s\n-~-~-~-~-~-~\n",
			         c_id, js_get_str( el, "text" ) );

			/* Searching for attachments */
			json_t * att_json;
			att_json = json_object_get( el, "attachments" );
			if ( att_json )
				parse_attachments( dirpath, filepath, att_json, logfile, post_id, c_id );
		}

		json_decref(json);
		offset += LIMIT_C;
		api_request_pause();
	}
	while( posts_count - offset > 0 );

	free_string(url);
}

void
get_wall( char * idpath )
{
	/* Char allocation */
	sstring * url = construct_string(2048);
	sstring * attach_path = construct_string(2048);
	sstring * curpath = construct_string(2048);

	sstring * posts_path = construct_string(2048);
	stringset( posts_path, "%s/%s", idpath, FILNAME_POSTS );
	FILE * posts = fopen( posts_path->c, "w" );
	free_string(posts_path);

	stringset( curpath, "%s/%s", idpath, DIRNAME_WALL );
	if ( mkdir( curpath->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* Loop start */
	long long offset = 0;
	long long posts_count = 0;
	do
	{
		stringset( url, "%s/wall.get?owner_id=%lld&extended=0&count=%d&offset=%lld%s&v=%s",
		         REQ_HEAD, acc.id, LIMIT_W, offset, TOKEN.c, API_VER );

		json_error_t * json_err = NULL;
		json_t * json = make_request( url, json_err );
		if ( !json )
			if ( json_err )
				fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

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
			long long epoch = js_get_int( el, "date" );

			fprintf( posts, "ID: %lld\n", p_id );
			if ( types.ldate == 1 )
				if ( readable_date( epoch, posts ) != 0 )
					types.ldate = 0;

			fprintf( posts, "EPOCH: %lld\nTEXT: %s\n", epoch, js_get_str( el, "text" ) );

			/* Searching for attachments */
			json_t * att_json;
			att_json = json_object_get( el, "attachments" );
			if ( att_json )
				parse_attachments( curpath, attach_path, att_json, posts, p_id, -1 );

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
						get_comments( curpath, attach_path, posts, p_id );
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
						parse_attachments( curpath, attach_path, rep_att_json, posts, p_id, -1 );
				}
			}

			fprintf( posts, "%s", LOG_POSTS_DIVIDER );
		}

		offset += LIMIT_W;
		api_request_pause();
		json_decref(json);
	}
	while( posts_count - offset > 0 );

	free_string(url);
	free_string(attach_path);
	free_string(curpath);

	fclose(posts);
}

void
get_docs( char * idpath )
{
	sstring * dirpath = construct_string(2048);
	sstring * doc_path = construct_string(2048);

	/* creating document directory */
	stringset( dirpath, "%s/%s", idpath, DIRNAME_DOCS );
	if ( mkdir( dirpath->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* Sending API request docs.get */
	sstring * url = construct_string(2048);
	stringset( url, "%s/docs.get?owner_id=%lld%s&v=%s", REQ_HEAD, acc.id, TOKEN.c, API_VER );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
		if ( json_err )
			fprintf( stderr, "JSON docs.get parsing error:\n%d:%s\n", json_err->line, json_err->text );

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
	printf("\nDocuments: %lld.\n", js_get_int( rsp, "count" ) );

	/* Loop init */
	size_t index;
	json_t * el;
	json_t * items;
	items = json_object_get( rsp, "items" );
	json_array_foreach( items, index, el )
	{
		if ( index != 0 )
			dl_document( dirpath, doc_path, el, NULL, -1, -1 );
	}

	json_decref(json);
}

void
get_friends( char * idpath )
{
	sstring * outfl = construct_string(2048);
	stringset( outfl, "%s/%s", idpath, FILNAME_FRIENDS );
	FILE * outptr = fopen( outfl->c, "w" );
	free_string(outfl);

	sstring * url = construct_string(2048);
	stringset( url, "%s/friends.get?user_id=%lld&order=domain&fields=domain%s&v=%s", REQ_HEAD, acc.id, TOKEN.c, API_VER );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
		if ( json_err )
			fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

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

	json_decref(json);
	fclose(outptr);
}

void
get_groups( char * idpath )
{
	sstring * outfl = construct_string(2048);
	stringset( outfl, "%s/%s", idpath, FILNAME_GROUPS );
	FILE * outptr = fopen( outfl->c, "w" );
	free_string(outfl);

	sstring * url = construct_string(2048);
	stringset( url, "%s/groups.get?user_id=%lld&extended=1%s&v=%s", REQ_HEAD, acc.id, TOKEN.c, API_VER );

	json_error_t * json_err = NULL;
	json_t * json = make_request( url, json_err );
	free_string(url);
	if ( !json )
		if ( json_err )
			fprintf( stderr, "JSON groups.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

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

	json_decref(json);
	fclose( outptr );
}

void
get_videos( char * idpath )
{
	long long vid_count = 0;
	sstring * url = construct_string(2048);
	sstring * vidpath = construct_string(2048);
	sstring * dirpath = construct_string(2048);

	/* creating document directory */
	stringset( dirpath, "%s/%s", idpath, DIRNAME_VIDEO );
	if ( mkdir( dirpath->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* creating log file with external links */
	sstring * vid_log_path = construct_string(2048);
	stringset( vid_log_path, "%s/%s", idpath, FILNAME_VIDEOS );
	FILE * vid_log = fopen( vid_log_path->c, "w" );
	free_string(vid_log_path);

	/* Loop init */
	int offset = 0;
	int times = 0;
	for ( ; offset <= times; ++offset )
	{
		/* creating request */
		stringset( url, "%s/video.get?owner_id=%lld&offset=%d&count=%d%s&v=%s",
		         REQ_HEAD, acc.id, offset * LIMIT_V, LIMIT_V, TOKEN.c, API_VER );

		json_error_t * json_err = NULL;
		json_t * json = make_request( url, json_err );
		if ( !json )
			if ( json_err )
				fprintf( stderr, "JSON video.get parsing error.\n%d:%s\n", json_err->line, json_err->text );

		/* finding response */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if ( !rsp )
		{
			fprintf( stderr, "Videos JSON error.\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
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
			dl_video( dirpath, vidpath, el, vid_log, -1, -1 );
		}

		json_decref(json);

		times = vid_count / LIMIT_V;
	}

	free_string(dirpath);
	free_string(vidpath);
	free_string(url);

	fclose(vid_log);
}
