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
		return -1;
	}

	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Album JSON error.\n" );
		rsp = json_object_get( json, "error" );
		fprintf( stderr, "%s\n", js_get_str(rsp, "error_msg") );
		return -2;
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

	if ( argc == 1 || argc > 3 || (argv[1][0] == '-' && argv[1][1] == 'h') )
	{
		puts("Wrong arguments.");
		puts("Usage: vkgrab <USER|GROUP>");
		puts("\tOR");
		puts("\tvkgrab -u USER");
		puts("\tvkgrab -g GROUP");
		return 0;
	}

	/* getting id */
	else if (argv[1][0] == '-')
	{
		if (argv[1][1] == 'u')
			usr = user(argv[2], curl);
		else if (argv[1][1] == 'g')
			grp = group(argv[2], curl);
	}
	else if (argc == 2)
	{
		usr = user(argv[1], curl);
		grp = group(argv[1], curl);
	}

	/* Info out */
	if ( usr.is_ok == 0 )
	{
		id = usr.uid;
		printf( "User: %s %s (%s)\nUser ID: %lld\nIs hidden: %lld\n\n", usr.fname, usr.lname, usr.screenname, usr.uid, usr.hidden );
	}
	else if ( grp.is_ok == 0 )
	{
		id = - grp.gid;
		printf( "Group: %s (%s)\nGroup ID: %lld\nType: %s\nIs closed: %lld\n\n", grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed );
	}

	return id;
}

void
get_albums_files( long long id, size_t arr_size, char * path, CURL * curl)
{
	char * url;
	char * curpath;
	char * alchar;
	url = malloc( bufs );
	curpath = malloc( bufs );
	alchar = malloc( bufs );
	long long pid;
	int i;
	const char * fileurl;

	for( i = 0; i < arr_size; ++i )
	{
		if ( albums[i].size > 0 )
		{
			int offset;
			int times = albums[i].size / LIMIT_A;
			for ( offset = 0; offset <= times; ++offset)
			{
				/* common names for service albums */
				if ( albums[i].aid == -6 )
					sprintf( alchar, "profile" );
				else if ( albums[i].aid == -7 )
					sprintf( alchar, "wall" );
				else if ( albums[i].aid == -15 )
					sprintf( alchar, "saved" );
				else
					sprintf( alchar, "%lld[%s]", albums[i].aid, albums[i].title );

				/* creating request */
				sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d%s", id, albums[i].aid, offset * LIMIT_A, TOKEN );
				char * r;
				r = vk_get_request( url, curl );

				/* creating album directory */
				sprintf( curpath, "%s/%s", path, alchar );
				if ( mkdir( curpath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
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
				json_t * biggest;
				json_array_foreach( rsp, index, el )
				{
					pid = js_get_int(el, "pid");
					biggest = json_object_get( el, "src_xxxbig" );
					if ( biggest )
						fileurl = json_string_value( biggest );
					else
					{
						biggest = json_object_get( el, "src_xxbig" );
						if ( biggest )
							fileurl = json_string_value( biggest );
						else
						{
							biggest = json_object_get( el, "src_xbig" );
							if ( biggest )
								fileurl = json_string_value( biggest );
							else
							{
								biggest = json_object_get( el, "src_big" );
								if ( biggest )
									fileurl = json_string_value( biggest );
								else
									continue;
							}
						}
					}

					/* downloading */
					sprintf( curpath, "%s/%s/%lld.jpg", path, alchar, pid );
					printf( "%s", curpath );
					vk_get_file( fileurl, curpath, curl );
				}
			}
		}
	}

	free( alchar );
	free( curpath );
	free( url );
}

void
get_wall( long long id, char * path, CURL * curl )
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

	sprintf( curpath, "%s/%s", path, "wall_attachments" );
	sprintf( posts_path, "%s/%s", path, "posts.txt" );
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
		json_t * biggest;
		json_t * photo_el;

		long long pid;
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
						photo_el = json_object_get( att_el, "photo" );
						if ( photo_el )
						{
							pid = js_get_int( photo_el, "pid" );
							fprintf( posts, "Photo for %lld: %lld\n", p_id, pid);
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
											continue;
									}
								}
							}

							/* downloading */
							sprintf( attach_path, "%s/%lld_%lld.jpg", curpath, p_id, pid );
							printf( "%s", attach_path );
							vk_get_file( fileurl, attach_path, curl );
						}
						else
						{
							json_t * link_el;
							link_el = json_object_get( att_el, "link" );
							if ( link_el )
							{
								fprintf( posts, "LINK_URL: %s\nLINK_DSC: %s\n", js_get_str( link_el, "url" ), js_get_str( link_el, "description" ) );
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
	fclose( posts );
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

	curl_easy_cleanup(curl);
	return 0;
}
