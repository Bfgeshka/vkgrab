#define _DEFAULT_SOURCE
#include "methods.c"


struct data_album * albums;
struct control_datatypes types;
long long photos_count = 0;

size_t
get_albums( CURL * curl )
{
	char * url = malloc( bufs );
	char * addit_request = malloc( bufs );

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
			strncpy( albums[index].title, js_get_str( el, "title" ), bufs );
			printf( "Album: %s (id:%lld, #:%lld).\n",
			        albums[index].title, albums[index].aid, albums[index].size );
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
						sprintf( TOKEN, "%s%s", TOKEN_HEAD, argv[t+1] );
				}
				if ( argv[t][1] == 'n' )
				{
					if ( argv[t][2] == 'p' )
						types.pictr = 0;
					else if ( argv[t][2] == 'd' )
						types.docmt = 0;
					else if ( argv[t][2] == 'a' )
						types.audio = 0;
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
					else if ( argv[t][2] == 'a' )
						types.audio = 1;
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

	/* failure */
	return 0;
}

void
get_albums_files( size_t arr_size, char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * curpath = malloc( bufs );
	char * alchar = malloc( bufs );
	char * dirchar = malloc( bufs );
	unsigned i;

	for( i = 0; i < arr_size; ++i )
	{
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
					photo( dirchar, curpath, el, curl, NULL, -1 );
				}
			}
		}
	}

	free( alchar );
	free( curpath );
	free( url );
	free( dirchar );
}

void
get_wall( char * idpath, CURL * curl )
{
	/* char allocation */
	char * url = malloc( bufs );
	char * curpath = malloc( bufs );
	char * posts_path = malloc( bufs );
	char * attach_path = malloc( bufs );

	sprintf( curpath, "%s/%s", idpath, DIRNAME_WALL );
	sprintf( posts_path, "%s/%s", idpath, FILNAME_POSTS );
	FILE * posts = fopen( posts_path, "w" );

	if ( mkdir( curpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* loop start */
	int offset = 0;
	long long posts_count = 0;
	do
	{
		sprintf( url, "https://api.vk.com/method/wall.get?owner_id=%lld&extended=0&count=%d&offset=%d%s&v=%s",
		         acc.id, LIMIT_W, offset, TOKEN, api_ver );
		char * r = vk_get_request( url, curl );

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
			fprintf( stderr, "Wall error.\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
		}

		/* getting posts count */
		if ( offset == 0 )
		{
			posts_count = js_get_int( rsp, "count" );
			printf( "Posts: %lld.\n", posts_count );
		}

		/* iterations in array */
		size_t index;
		json_t * el;
		json_t * items;
		items = json_object_get( rsp, "items" );
		long long p_date;
		long long p_id;

		json_array_foreach( items, index, el )
		{
			if ( index != 0 || offset != 0 )
			{
				p_id = js_get_int( el, "id" );
				p_date = js_get_int( el, "date" );
				fprintf( posts, "ID: %lld\nEPOCH: %lld\nTEXT: %s\n", p_id, p_date, js_get_str( el, "text" ) );

				json_t * att_json;
				att_json = json_object_get( el, "attachments" );
				if ( att_json )
				{
					/*
		//			size_t arr_size = json_array_size( att_json );
		//			for ( size_t att_i = 0; att_i < arr_size; ++att_i )
		//			{
		//				json_t * att_el = json_array_get( att_json, att_i );
					*/
					size_t att_index;
					json_t * att_el;
					json_array_foreach( att_json, att_index, att_el )
					{

						json_t * attached;
						json_t * tmp_js;
						attached = json_object_get( att_el, "type" );

#define ZZ "photo"
						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 && types.pictr == 1 )
						{
							tmp_js = json_object_get( att_el, ZZ );
							photo( curpath, attach_path, tmp_js, curl, posts, p_id );
						}
#undef ZZ
#define ZZ "link"
						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 )
						{
							tmp_js = json_object_get( att_el, ZZ );
							fprintf( posts, "ATTACH: LINK_URL: %s\nATTACH: LINK_DSC: %s\n",
							         js_get_str( tmp_js, "url" ), js_get_str( tmp_js, "description" ) );
						}
#undef ZZ
#define ZZ "doc"
						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 && types.docmt == 1 )
						{
							tmp_js = json_object_get( att_el, ZZ );
							document( curpath, attach_path, tmp_js, curl, posts, p_id );
						}
#undef ZZ
#define ZZ "audio"
						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 && types.audio == 1 )
						{
							tmp_js = json_object_get( att_el, ZZ );
							audiofile( curpath, attach_path, tmp_js, curl, posts, p_id );
						}
#undef ZZ
#define ZZ "video"
						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 && types.video == 1 )
						{
							tmp_js = json_object_get( att_el, ZZ );
							vid_file( curpath, attach_path, tmp_js, curl, posts, p_id );
						}
#undef ZZ
					}
				}
				fprintf( posts, "------\n\n" );
			}
		}

		offset += LIMIT_W;
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
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * doc_path = malloc( bufs );

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
			document( dirpath, doc_path, el, curl, NULL, -1 );
	}

	free( dirpath );
	free( doc_path );
}

void
get_friends( char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * outfl = malloc( bufs );

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
	char * url = malloc( bufs );
	char * outfl = malloc( bufs );

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
get_music( char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * trackpath = malloc( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_AUDIO );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	sprintf( url, "https://api.vk.com/method/audio.get?owner_id=%lld&need_user=0%s&v=%s",
	         acc.id, TOKEN, api_ver );
	char * r = vk_get_request( url, curl );
	free( url );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON audio.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* finding response */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if ( !rsp )
	{
		fprintf( stderr, "Music response JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str( rsp, "error_msg" ) );
	}

	printf( "\nTracks: %lld.\n", js_get_int( rsp, "count" ) );

	size_t index;
	json_t * el;
	json_t * items;
	items = json_object_get( rsp, "items" );
	json_array_foreach( items, index, el )
	{
		audiofile( dirpath, trackpath, el, curl, NULL, -1 );
	}

	free( dirpath );
	free( trackpath );
}

void
get_videos( char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * vidpath = malloc( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_VIDEO );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	/* creating log file with external links */
	char * vid_log_path;
	vid_log_path = malloc( bufs );
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

		vid_count = js_get_int( rsp, "count" );
		printf( "\nVideos: %lld.\n", vid_count );

		/* iterations in array */
		size_t index;
		json_t * el;
		json_t * items;
		items = json_object_get( rsp, "items" );
		json_array_foreach( items, index, el )
		{
			vid_file( dirpath, vidpath, el, curl, vid_log, -1 );
		}

		times = vid_count / LIMIT_V;
	}

	free( dirpath );
	free( vid_log_path );
	free( vidpath );
	free( url );
}


int
main( int argc, char ** argv )
{
	/* curl handler initialisatiion */
	CURL * curl;
	curl = curl_easy_init();
	if ( !curl )
	{
		fprintf( stderr, "Curl initialisation error.\n" );
		return 1;
	}

	/* Define downloaded datatypes */
	types.audio = DOGET_AUD;
	types.docmt = DOGET_DOC;
	types.pictr = DOGET_PIC;
	types.video = DOGET_VID;

	/* Checking id */
	long long id = get_id( argc, argv, curl );
	if ( id == 0 )
		return 2;

	/* Naming file metadata */
	char output_dir[bufs];
	char name_descript[bufs];
	if ( acc.usr_ok == 0 )
	{
		sprintf( output_dir, "u_%lld", acc.id );
		sprintf( name_descript, "%s: %s %s", acc.screenname, acc.usr_fname, acc.usr_lname );
	}
	else if ( acc.grp_ok == 0 )
	{
		sprintf( output_dir, "c_%lld", acc.id );
		sprintf( name_descript, "%s: %s", acc.screenname, acc.grp_name );
	}
	else
	{
		fprintf( stderr, "Screenname is invalid.\n");
		return 3;
	}

	/* Creating dir for current id */
	fix_filename( output_dir );
	if ( mkdir( output_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );
	char name_dsc_path[bufs];
	sprintf( name_dsc_path, "%s/%s", output_dir, FILNAME_IDNAME );
	FILE * u_name = fopen( name_dsc_path, "w" );
	fprintf( u_name, "%s", name_descript );
	fclose( u_name );

	/* Getting wall content */
	get_wall( output_dir, curl );

	/* Getting albums content */
	if ( types.pictr == 1 )
	{
		size_t arr_size = get_albums( curl );
		if ( arr_size > 0 )
		{
			get_albums_files( arr_size, output_dir, curl );
			free( albums );
		}
	}

	/* Getting documents */
	if ( types.docmt == 1 )
		get_docs( output_dir, curl );

	if ( id > 0 )
	{
		/* These are fast */
		get_friends( output_dir, curl );
		request_pause();
		get_groups( output_dir, curl );
		request_pause();
	}

	if ( types.audio == 1 )
		get_music( output_dir, curl );

	if ( types.video == 1 )
		get_videos( output_dir, curl );

	curl_easy_cleanup( curl );
	return 0;
}
