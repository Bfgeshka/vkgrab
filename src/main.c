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
//	long long photos_count = 0;
	/* getting albums */
	char *url = NULL;
	url = malloc( bufs );
	sprintf(url, "https://api.vk.com/method/photos.getAlbums?owner_id=%lld&need_system=1", id);
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
		printf("Albums: %lu\n", (unsigned long) arr_size);
		albums = malloc( arr_size * sizeof(struct data_album) );
		json_array_foreach( rsp, index, el )
		{
			albums[index].aid = js_get_int(el, "aid");
			albums[index].size = js_get_int(el, "size");
			strncpy( albums[index].title, js_get_str(el, "title" ), bufs);
			printf("Album: %s\n\tid: %lld, #: %lld\n", albums[index].title, albums[index].aid, albums[index].size);
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

	if ( argc == 1 || argc > 3 )
	{
		puts("Wrong arguments.");
		puts("Usage: vk_grabber <USER|GROUP>");
		puts("\tOR");
		puts("\tvk_grabber -u USER");
		puts("\tvk_grabber -g GROUP");
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
		printf("User: %s %s (%s)\nUser ID: %lld\nIs hidden: %lld\n\n", usr.fname, usr.lname, usr.screenname, usr.uid, usr.hidden);
	}
	else if ( grp.is_ok == 0 )
	{
		id = - grp.gid;
		printf("Group: %s (%s)\nGroup ID: %lld\nType: %s\nIs closed: %lld\n\n", grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed);
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
//			printf("times = %d\n", times);
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
				sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0&offset=%d", id, albums[i].aid, offset * LIMIT_A );
				char * r;
				r = vk_get_request( url, curl );
//						printf("%s\n", r);

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
					printf( "%s\n", curpath );
		//			FILE * current = fopen( curpath, "r" );
		//			if ( errno == 0 )
		//			{
		//				fclose( current );
						vk_get_file( fileurl, curpath, curl );
		//			}
		//			fclose( current );
				}
			}
		}
	}

	free( alchar );
	free( curpath );
	free( url );
}

void
get_wall( long long id, char * path )
{

}


int
main( int argc, char ** argv )
{
	CURL * curl;
	curl = curl_easy_init();

	/* Checking id */
	long long id = get_id( argc, argv, curl );
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

	/* Getting albums content */
	size_t arr_size = get_albums( id, curl );
	if ( arr_size > 0 )
	{
		get_albums_files( id, arr_size, user_dir, curl );
		free(albums);
	}

//	get_wall( id, user_dir );



	curl_easy_cleanup(curl);
	return 0;
}
