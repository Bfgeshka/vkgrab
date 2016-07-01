#include "methods.c"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

struct data_user usr;
struct data_group grp;
struct data_album * albums;
long long photos_count = 0;

size_t
get_albums( long long id )
{
	long long photos_count = 0;
	/* getting albums */
	char *url = NULL;
	url = malloc( bufs );
	sprintf(url, "https://api.vk.com/method/photos.getAlbums?owner_id=%lld&need_system=1", id);
	char * r;
	r = vk_get_request(url);
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
		printf("Albums: %lu\n", arr_size);
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
get_id( int argc, char ** argv )
{

	long long id = 0;

	if ( argc == 1 || argc > 3 )
	{
		puts("Wrong arguments.");
		puts("Usage: vk_grabber <USER|GROUP>");
		puts("\tOR");
		puts("\tvk_grabber -u USER");
		puts("\tvk_grabber -g GROUP");
		return 1;
	}
	/* getting id */
	else if (argv[1][0] == '-')
	{
		if (argv[1][1] == 'u')
			usr = user(argv[2]);
		else if (argv[1][1] == 'g')
			grp = group(argv[2]);
	}
	else if (argc == 2)
	{
		usr = user(argv[1]);
		grp = group(argv[1]);
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
get_albums_files( long long id, size_t arr_size, char * path )
{
	char * url;
	char * curpath;
	const char * fileurl;
	char * alchar;
	url = malloc( bufs );
	curpath = malloc( bufs );
	alchar = malloc( bufs );
	long long pid;
	int i;
	for( i = 0; i < arr_size; ++i )
	{
		if ( albums[i].size > 0 )
		{
			/* common names for service albums */
			if ( albums[i].aid == -6 )
				sprintf( alchar, "profile" );
			else if ( albums[i].aid == -7 )
				sprintf( alchar, "wall" );
			else if ( albums[i].aid == -15 )
				sprintf( alchar, "saved" );
			else
				sprintf( alchar, "%lld", albums[i].aid );

			/* creating request */
			sprintf( url, "https://api.vk.com/method/photos.get?owner_id=%lld&album_id=%lld&photo_sizes=0", id, albums[i].aid );
			char * r;
			r = vk_get_request( url );
			//		printf("%s\n", r);

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
				vk_get_file( fileurl, curpath );
			}
		}
	}

	free( alchar );
	free( curpath );
	free( url );
}

int
main( int argc, char ** argv )
{
	long long id = get_id( argc, argv );

	char user_dir[bufs];
	if ( usr.is_ok == 0 )
		sprintf( user_dir, "%s(%lld_%s_%s)", usr.screenname, usr.uid, usr.fname, usr.lname);
	else if (grp.is_ok == 0)
		sprintf( user_dir, "%s(%lld_%s)", grp.screenname, grp.gid, grp.name );

	if ( mkdir( user_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf(stderr, "mkdir() error (%d).\n", errno);

	/* Getting albums content */
	size_t arr_size = get_albums( id );
	if ( arr_size > 0 )
	{
		get_albums_files( id, arr_size, user_dir );
		free(albums);
	}

	return 0;
}
