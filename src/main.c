#include "methods.c"


struct data_user usr;
struct data_group grp;
struct data_album * albums;
struct control_datatypes types;
long long photos_count = 0;

size_t
get_albums( long long id, CURL * curl )
{
	/* getting albums */
//	char *url = NULL;
//	url = malloc( bufs );
	char * url = malloc( bufs );

	sprintf( url, "https://api.vk.com/method/photos.getAlbums?owner_id=%lld&need_system=1%s", id, TOKEN );
	char * r;
	r = vk_get_request(url, curl);
	free(url);

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
	{
		fprintf( stderr, "JSON album parsing error.\n%d:%s\n", json_err.line, json_err.text );
		return 0;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Album JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
		return 0;
	}

	/* getting albums metadata*/
	size_t arr_size = json_array_size(rsp);

	if ( arr_size > 0 )
	{
		json_t * el;
		size_t index;
		printf("\nAlbums: %lu.\n", (unsigned long) arr_size);
		albums = malloc( arr_size * sizeof(struct data_album) );
		json_array_foreach( rsp, index, el )
		{
			albums[index].aid = js_get_int(el, "aid");
			albums[index].size = js_get_int(el, "size");
			strncpy( albums[index].title, js_get_str(el, "title" ), bufs);
			printf( "Album: %s (id:%lld, #:%lld).\n",
			        albums[index].title, albums[index].aid, albums[index].size );
			photos_count += albums[index].size;
		}
	}
	else
		printf("No albums found.\n");

	return arr_size;
}

long long
get_id( int argc, char ** argv, CURL * curl )
{
	long long id = 0;
	usr.is_ok = 1;
	grp.is_ok = 1;

	if ( argc == 1 || (argv[1][0] == '-' && argv[1][1] == 'h' ) )
	{
		help_print();
		return 0;
	}
	else if (argc == 2)
	{
		usr = user(argv[1], curl);
		grp = group(argv[1], curl);
	}
	else
		for ( int t = 0; t < argc; ++t )
		{
			if ( argv[t][0] == '-' )
			{
				if ( argv[t][1] == 'u' )
					usr = user(argv[t+1], curl);
				if ( argv[t][1] == 'g' )
					grp = group(argv[t+1], curl);
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
			if ( (t == argc - 1) && (usr.is_ok == 1) && (grp.is_ok == 1) )
			{
				usr = user(argv[t], curl);
				grp = group(argv[t], curl);
			}
		}

	/* Info out */
	if ( grp.is_ok == 0 )
	{
		id = - grp.gid;
		printf( "Group: %s (%s).\nGroup ID: %lld.\nType: %s.\nIs closed: %lld.\n\n",
		        grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed );
	}
	else if ( usr.is_ok == 0 )
	{
		id = usr.uid;
		printf( "User: %s %s (%s).\nUser ID: %lld.\n\n",
		        usr.fname, usr.lname, usr.screenname, usr.uid );
	}

	return id;
}

void
get_albums_files( long long id, size_t arr_size, char * idpath, CURL * curl )
{
//	char * url;
//	char * curpath;
//	char * alchar;
//	char * dirchar;
//	url = malloc( bufs );
//	curpath = malloc( bufs );
//	alchar = malloc( bufs );
//	dirchar = malloc( bufs );
//	const char * fileurl;
	char * url = malloc( bufs );
	char * curpath = malloc( bufs );
	char * alchar = malloc( bufs );
	char * dirchar = malloc( bufs );
//	char * fileurl = malloc( bufs );

	for( int i = 0; i < arr_size; ++i )
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
				sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d%s",
				         id, albums[i].aid, offset * LIMIT_A, TOKEN );
				char * r;
				r = vk_get_request( url, curl );

				/* creating album directory */
				fix_filename( alchar );
				sprintf( dirchar, "%s/%s", idpath, alchar );
				if ( mkdir( dirchar, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
					if ( errno != EEXIST )
						fprintf(stderr, "mkdir() error (%d).\n", errno);

				/* parsing json */
				json_t * json;
				json_error_t json_err;
				json = json_loads( r, 0, &json_err );
				if ( !json )
					fprintf( stderr, "JSON photos.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

				/* simplifying json */
				json_t * rsp;
				rsp = json_object_get( json, "response" );
				if (!rsp)
					fprintf( stderr, "Album error\n" );

				/* iterations in array */
				size_t index;
				json_t * el;
				json_array_foreach( rsp, index, el )
				{
//					photo( dirchar, curpath, fileurl, el, curl, NULL, -1 );
					photo( dirchar, curpath, el, curl, NULL, -1 );
				}
			}
		}
	}

	free( alchar );
	free( curpath );
	free( url );
	free( dirchar );
//	free( fileurl );
}

void
get_wall( long long id, char * idpath, CURL * curl )
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
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* loop start */
	int offset = 0;
	long long posts_count = 0;
	do
	{
		sprintf(url, "https://api.vk.com/method/wall.get?owner_id=%lld&extended=0&count=%d&offset=%d%s",
		        id, LIMIT_W, offset, TOKEN);
		char * r;
		r = vk_get_request( url, curl );

		/* parsing json */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( !json )
			fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

		/* simplifying json */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if (!rsp)
		{
			fprintf( stderr, "Wall error\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
		}

		/* getting posts count */
		if ( offset == 0 )
		{
			json_t * temp_json;
			temp_json = json_array_get( rsp, 0 );
			posts_count = json_integer_value( temp_json );
			printf("Posts: %lld\n", posts_count);
		}

		/* iterations in array */
		size_t index;
		json_t * el;
		long long p_date;
		long long p_id;

		json_array_foreach( rsp, index, el )
		{
			if ( index != 0 || offset != 0 )
			{
				p_id = js_get_int( el, "id" );
				p_date = js_get_int( el, "date" );
				fprintf( posts, "ID: %lld\nEPOCH: %lld\nTEXT: %s\n", p_id, p_date, js_get_str(el, "text") );

				json_t * att_json;
				att_json = json_object_get( el, "attachments" );
				if (att_json)
				{
					size_t arr_size = json_array_size(att_json);
					for ( size_t att_i = 0; att_i < arr_size; ++att_i )
					{
						json_t * att_el = json_array_get( att_json, att_i );

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

//#define ZZ "video"
//						if ( strncmp( json_string_value(attached), ZZ, 3 ) == 0 && types.video == 1 )
//						{
//							tmp_js = json_object_get( att_el, ZZ );
//							vid_file( curpath, attach_path, fileurl, tmp_js, curl, posts, p_id );
//						}
//#undef ZZ
					}
				}
				fprintf(posts, "------\n\n");
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
get_docs( long long id, char * idpath, CURL * curl )
{
	/* char allocation */
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * doc_path = malloc( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_DOCS );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* Sending API request docs.get */
	sprintf( url, "https://api.vk.com/method/docs.get?owner_id=%lld%s", id, TOKEN);
	char * r;
	r = vk_get_request( url, curl );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON docs.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Documents JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
	}

	/* Show documents count */
	json_t * temp_json;
	temp_json = json_array_get( rsp, 0 );
	long long docs_count = json_integer_value( temp_json );
	printf("\nDocuments: %lld\n", docs_count);

	/* Loop init */
	size_t index;
	json_t * el;
	json_array_foreach( rsp, index, el )
	{
		if ( index != 0 )
			document( dirpath, doc_path, el, curl, NULL, -1 );
	}

	free( url );
	free( dirpath );
	free( doc_path );
}

void
get_friends( long long id, char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * outfl = malloc( bufs );

	sprintf( outfl, "%s/%s", idpath, FILNAME_FRIENDS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/friends.get?user_id=%lld&order=domain&fields=domain%s",
	         id, TOKEN );
	char * r;
	r = vk_get_request( url, curl );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON wall.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Friends JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
	}

	/* iterations in array */
	size_t index;
	json_t * el;
	json_array_foreach( rsp, index, el )
	{
		fprintf( outptr, "%s\n", js_get_str(el, "domain") );
	}

	printf("\nFriends list (of %lu) saved!\n", (unsigned long) index);

	free( url );
	free( outfl );
	fclose( outptr );
}

void
get_groups( long long id, char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * outfl = malloc( bufs );

	sprintf( outfl, "%s/%s", idpath, FILNAME_GROUPS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/groups.get?user_id=%lld&extended=1%s", id, TOKEN );
	char * r;
	r = vk_get_request( url, curl );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON groups.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Groups JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
	}

	/* iterations in array */
	size_t index;
	json_t * el;
	json_array_foreach( rsp, index, el )
	{
		if ( index != 0 )
			fprintf( outptr, "%s\n", js_get_str(el, "screen_name") );
	}

	printf("Communities list (of %lu) saved!\n", (unsigned long) index - 1);

	free( url );
	free( outfl );
	fclose( outptr );
}

void
get_music( long long id, char * idpath, CURL * curl )
{
//	char * url;
//	char * dirpath;
//	char * trackpath;
//	const char * fileurl;
//	url = malloc( bufs );
//	dirpath = malloc( bufs );
//	trackpath = malloc( bufs );
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * trackpath = malloc( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_AUDIO );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	sprintf( url, "https://api.vk.com/method/audio.get?owner_id=%lld&need_user=0%s", id, TOKEN );
	char * r;
	r = vk_get_request( url, curl );

	/* parsing json */
	json_t * json;
	json_error_t json_err;
	json = json_loads( r, 0, &json_err );
	if ( !json )
		fprintf( stderr, "JSON audio.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Music response JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
	}

	size_t index;
	json_t * el;
	json_array_foreach( rsp, index, el )
	{
		if ( index == 0 )
			printf( "\nTracks: %lld\n", json_integer_value(el) );
		else
			audiofile( dirpath, trackpath, el, curl, NULL, -1 );
//			audiofile( dirpath, trackpath, fileurl, el, curl, NULL, -1 );
	}

	free( dirpath );
	free( url );
	free( trackpath );
}

void
get_videos( long long id, char * idpath, CURL * curl )
{
	char * url = malloc( bufs );
	char * dirpath = malloc( bufs );
	char * vidpath = malloc( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_VIDEO );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* creating log file with external links */
	char * vid_log_path;
	vid_log_path = malloc( bufs );
	sprintf( vid_log_path, "%s/%s", idpath, FILNAME_VIDEOS );
	FILE * vid_log = fopen( vid_log_path, "w" );

	/* finding out videos count */
//	sprintf( url, "https://api.vk.com/method/video.get?owner_id=%lld&count=0&offset=0%s", id, TOKEN );
//	sleep(1); /* too many requests per second, must be reduced */
//	char * r_pre;
//	r_pre = vk_get_request( url, curl );

	/* parsing json */
//	json_t * json;
//	json_error_t json_err;
//	json = json_loads( r_pre, 0, &json_err );
//	if ( !json )
//		fprintf( stderr, "JSON scout video.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

	/* simplifying json */
//	json_t * rsp;
//	rsp = json_object_get( json, "response" );
//	if (!rsp)
//	{
//		fprintf( stderr, "Videos scout JSON error.\n" );
//		rsp = json_object_get( json, "error" );
//		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
//	}

	long long vid_count = 0;


	/* Loop init */
	int offset = 0;
	int times = 0;
	for ( ; offset <= times; ++offset )
	{
		/* creating request */
		sprintf( url, "https://api.vk.com/method/video.get?owner_id=%lld&offset=%d&count=%d%s",
		         id, offset * LIMIT_V, LIMIT_V, TOKEN );
		char * r;
		r = vk_get_request( url, curl );

		/* JSON init */
		json_t * json;
		json_error_t json_err;
		json = json_loads( r, 0, &json_err );
		if ( !json )
			fprintf( stderr, "JSON video.get parsing error.\n%d:%s\n", json_err.line, json_err.text );

		/* simplifying json */
		json_t * rsp;
		rsp = json_object_get( json, "response" );
		if (!rsp)
		{
			fprintf( stderr, "Videos JSON error.\n" );
			rsp = json_object_get( json, "error" );
			fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
		}

		/* iterations in array */
		size_t index;
		json_t * el;
		json_array_foreach( rsp, index, el )
		{
			if ( index == 0 && offset == 0 )
			{
				vid_count = json_integer_value( json_array_get( rsp, 0 ) );
				printf("\nVideos: %lld\n", vid_count);
			}
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
		fprintf( stderr, "curl initialisation error\n" );
		return 3;
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

	char user_dir[bufs];
	char name_descript[bufs];
	if ( usr.is_ok == 0 )
	{
		sprintf( user_dir, "%s(%lld)", usr.screenname, usr.uid );
		sprintf( name_descript, "%s_%s",  usr.fname, usr.lname );
	}
	else if (grp.is_ok == 0)
	{
		sprintf( user_dir, "%s(%lld)", grp.screenname, grp.gid );
		sprintf( name_descript, "%s",  grp.name );
	}
	else
	{
		fprintf( stderr, "Screenname is invalid.\n");
		return 1;
	}

	/* Creating dir for current id */
	fix_filename( user_dir );
	if ( mkdir( user_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);
	char name_dsc_path[bufs];
	sprintf( name_dsc_path, "%s/%s", user_dir, FILNAME_IDNAME );
	FILE * u_name = fopen( name_dsc_path, "w" );
	fprintf( u_name, name_descript );
	fclose( u_name );

	/* Getting wall content */
	get_wall( id, user_dir, curl );

	/* Getting albums content */
	if ( types.pictr == 1 )
	{
		size_t arr_size = get_albums( id, curl );
		if ( arr_size > 0 )
		{
			get_albums_files( id, arr_size, user_dir, curl );
			free(albums);
		}
	}

	/* Getting user documents */
	if ( types.docmt == 1 )
		get_docs( id, user_dir, curl );

	if ( usleep( (unsigned int) USLEEP_INT ) != 0 ) puts("sleep error");

	if ( id > 0 )
	{
		get_friends( id, user_dir, curl );
		if ( usleep( (unsigned int) USLEEP_INT ) != 0 ) puts("sleep error");
		get_groups( id, user_dir, curl );
	}

	if ( usleep( (unsigned int) USLEEP_INT ) != 0 ) puts("sleep error");

	if ( types.audio == 1 )
		get_music( id, user_dir, curl );

	if ( types.video == 1 )
		get_videos( id, user_dir, curl );

	curl_easy_cleanup(curl);
	return 0;
}
