#include "methods.c"


struct data_user usr;
struct data_group grp;
struct data_album * albums;

long long
//struct data_album *
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
		return 1;
	}
	/* simplifying json */
	json_t * rsp;
	rsp = json_object_get( json, "response" );
	if (!rsp)
	{
		fprintf( stderr, "Album JSON error.\n" );
		return 2;
	}

	/* getting albums metadata*/
	size_t arr_size = json_array_size(rsp);

	if ( arr_size > 0 )
	{
		json_t * el;
		size_t index;
		printf("Albums: %lu\n", arr_size);
		albums = malloc( arr_size * sizeof(struct data_album) );
		//	struct data_album albums[arr_size];
		json_array_foreach( rsp, index, el )
		{
			albums[index].aid = js_get_int(el, "aid");
			albums[index].size = js_get_int(el, "size");
			strncpy( albums[index].title, js_get_str(el, "title" ), bufs);
			printf("Album: %s\n\tid: %lld\n\tnumber of photos: %lld\n", albums[index].title, albums[index].aid, albums[index].size);
			photos_count += albums[index].size;
		}
	}
	else
		printf("ID hasn't any album\n");

	return photos_count;
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

int
main( int argc, char ** argv )
{
	long long id = get_id( argc, argv );
	long long photos_count = get_albums( id );


	if ( photos_count == 0 )
		printf("\nNo any photos in albums!\n");
	else
	{
		printf("\nPhotos: %lld\n", photos_count);
	}



//	printf ("\n%lu\n", vk_get_file("https://pp.vk.me/c630518/v630518327/1ed3c/j_bxowONolY.jpg", "input/file.jpg"));

	/* apropriate finishing */
	free(albums);
	return 0;
}
