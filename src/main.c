#include "methods.c"
#include <sys/stat.h>
#include <sys/types.h>


struct data_user usr;
struct data_group grp;
struct data_album * albums;
long long photos_count = 0;

size_t
get_albums( long long id, CURL * curl )
{
	/* getting albums */
	char *url = NULL;
	url = malloc( bufs );
	sprintf( url, "https://api.vk.com/method/photos.getAlbums?owner_id=%lld&need_system=1%s", id, TOKEN );
	char * r;
	r = vk_get_request(url, curl);
	free(url);

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
		printf("\nAlbums: %lu\n", (unsigned long) arr_size);
		albums = malloc( arr_size * sizeof(struct data_album) );
		json_array_foreach( rsp, index, el )
		{
			albums[index].aid = js_get_int(el, "aid");
			albums[index].size = js_get_int(el, "size");
			strncpy( albums[index].title, js_get_str(el, "title" ), bufs);
			printf( "Album: %s (id:%lld, #:%lld)\n", albums[index].title, albums[index].aid, albums[index].size );
			photos_count += albums[index].size;
		}
	}
	else
		printf("ID hasn't any album\n");

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
		puts("Usage:\tvkgrab [OPTIONS] <USER|GROUP>");
		puts("Or:\tvkgrab <USER|GROUP>");
		puts("");
		puts("\t-t TOKEN\tgive a valid token without header \"&access_token=\"");
		puts("\t-u USER\tignoring group with same screenname");
		puts("\t-g GROUP\tignoring user with same screenname");
		puts("If both USER and GROUP do exists, group id would be proceeded");
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
				if ( argv[t][1] == 'g')
					grp = group(argv[t+1], curl);
				if ( argv[t][1] == 't')
				{
					if ( strlen( TOKEN ) == strlen( TOKEN_HEAD ) )
						strcat( TOKEN, argv[t+1] );
					else
						sprintf( TOKEN, "%s%s", TOKEN_HEAD, argv[t+1] );
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
		printf( "Group: %s (%s)\nGroup ID: %lld\nType: %s\nIs closed: %lld\n\n", grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed );
	}

	else if ( usr.is_ok == 0 )
	{
		id = usr.uid;
		printf( "User: %s %s (%s)\nUser ID: %lld\n\n", usr.fname, usr.lname, usr.screenname, usr.uid );
	}
	return id;

}

void
get_albums_files( long long id, size_t arr_size, char * idpath, CURL * curl)
{
	char * url;
	char * curpath;
	char * alchar;
	char * dirchar;
	url = malloc( bufs );
	curpath = malloc( bufs );
	alchar = malloc( bufs );
	dirchar = malloc( bufs );
	const char * fileurl;

	for( int i = 0; i < arr_size; ++i )
	{
		if ( albums[i].size > 0 )
		{
			int offset;
			int times = albums[i].size / LIMIT_A;
			for ( offset = 0; offset <= times; ++offset)
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
				sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d%s", id, albums[i].aid, offset * LIMIT_A, TOKEN );
				char * r;
				r = vk_get_request( url, curl );

				/* creating album directory */
				sprintf( dirchar, "%s/%s", idpath, alchar );
				if ( mkdir( dirchar, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
					if ( errno != EEXIST )
						fprintf(stderr, "mkdir() error (%d).\n", errno);


				/* JSON init */
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
				//			json_t * biggest;
				json_array_foreach( rsp, index, el )
				{
					photo( dirchar, curpath, fileurl, el, curl, NULL, -1 );
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
get_wall( long long id, char * idpath, CURL * curl )
{
	/* char allocation */
	char * url;
	char * curpath;
	char * posts_path;
	char * attach_path;
	const char * fileurl;
	url = malloc( bufs );
	curpath = malloc( bufs );
	posts_path = malloc( bufs );
	attach_path = malloc( bufs );

	sprintf( curpath, "%s/%s", idpath, DIRNAME_WALL );
	sprintf( posts_path, "%s/%s", idpath, FILNAME_POSTS );
	FILE * posts = fopen( posts_path, "w" );

	if ( mkdir( curpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* starting loop */
	int offset = 0;
	long long posts_count = 0;
	do
	{
		sprintf(url, "https://api.vk.com/method/wall.get?owner_id=%lld&extended=0&count=%d&offset=%d%s", id, LIMIT_W, offset, TOKEN);
		char * r;
		r = vk_get_request( url, curl );

		/* JSON init */
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
//		json_t * biggest;
		json_t * attached;

//		long long pid;
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
				json_t * att_el;
				size_t att_index;
				att_json = json_object_get( el, "attachments" );
				if (att_json)
				{
					json_array_foreach( att_json, att_index, att_el );
					{
						attached = json_object_get( att_el, "photo" );
						if ( attached )
							photo( curpath, attach_path, fileurl, attached, curl, posts, p_id );

						else
						{
							attached = json_object_get( att_el, "link" );
							if ( attached )
								fprintf( posts, "LINK_URL: %s\nLINK_DSC: %s\n", js_get_str( attached, "url" ), js_get_str( attached, "description" ) );

							else
							{
								attached = json_object_get( att_el, "doc" );
								if ( attached )
									document( curpath, attach_path, fileurl, attached, curl, posts, p_id );
							}
						}


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
	char * url;
	char * dirpath;
	char * doc_path;
//	char * log_path;
	const char * fileurl;
	url = malloc( bufs );
	dirpath = malloc( bufs );
	doc_path = malloc( bufs );
//	log_path = malloc ( bufs );

	/* creating document directory */
	sprintf( dirpath, "%s/%s", idpath, DIRNAME_DOCS );
	if ( mkdir( dirpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* Sending API request docs.get */
	sprintf( url, "https://api.vk.com/method/docs.get?owner_id=%lld%s", id, TOKEN);
	char * r;
	r = vk_get_request( url, curl );

	/* JSON init */
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


	/* Log file */
//	sprintf( log_path, "%s/%s", idpath, FILNAME_DOCS );
//	FILE * log_doc = fopen( log_path, "w" );
//	fprintf( log_doc, "Documents: %lld\n\n", docs_count);

	/* Loop init */
	size_t index;
	json_t * el;
//	json_t * attached;
	json_array_foreach( rsp, index, el )
	{
		if ( index != 0 )
			document( dirpath, doc_path, fileurl, el, curl, NULL, -1 );
	}

	free( url );
	free( dirpath );
	free( doc_path );
//	free( log_path );
//	fclose( log_doc );
}

void
get_friends( long long id, char * idpath, CURL * curl )
{
	char * url;
	char * outfl;
	url = malloc( bufs );
	outfl = malloc( bufs );
	sprintf( outfl, "%s/%s", idpath, FILNAME_FRIENDS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/friends.get?user_id=%lld&order=domain&fields=domain%s", id, TOKEN );
	char * r;
	r = vk_get_request( url, curl );

	/* JSON init */
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
	char * url;
	char * outfl;
	url = malloc( bufs );
	outfl = malloc( bufs );
	sprintf( outfl, "%s/%s", idpath, FILNAME_GROUPS );
	FILE * outptr = fopen( outfl, "w" );

	sprintf( url, "https://api.vk.com/method/groups.get?user_id=%lld&extended=1%s", id, TOKEN );
	char * r;
	r = vk_get_request( url, curl );

	/* JSON init */
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

	/* Checking id */
	long long id = get_id( argc, argv, curl );
	if ( id == 0 )
		return 2;
	char user_dir[bufs];
	if ( usr.is_ok == 0 )
		sprintf( user_dir, "%s(%lld)_%s_%s", usr.screenname, usr.uid, usr.fname, usr.lname);
	else if (grp.is_ok == 0)
		sprintf( user_dir, "%s(%lld)_%s", grp.screenname, grp.gid, grp.name );
	else
	{
		fprintf( stderr, "Screenname is invalid.\n");
		return 1;
	}

	/* Creating dir forr current id */
	if ( mkdir( user_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* Getting wall content */
	get_wall( id, user_dir, curl );

	/* Getting albums content */
	size_t arr_size = get_albums( id, curl );
	if ( arr_size > 0 )
	{
		get_albums_files( id, arr_size, user_dir, curl );
		free(albums);
	}

	/* Getting user documents */
	get_docs( id, user_dir, curl );

	if ( id > 0 )
	{
		get_friends( id, user_dir, curl );
		get_groups( id, user_dir, curl );
	}


	curl_easy_cleanup(curl);
	return 0;
}
