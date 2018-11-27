#include <curl/curl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "../config.h"
#include "methods.h"

int
main( int argc, char ** argv )
{
	/* curl handler initialisatiion */
	extern CURL * curl;
	curl = curl_easy_init();
	if ( !curl )
	{
		fprintf( stderr, "Curl initialisation error.\n" );
		return 1;
	}

	/* Define downloaded datatypes */
	types.docmt = DOGET_DOC;
	types.pictr = DOGET_PIC;
	types.video = DOGET_VID;
	types.comts = DOGET_COM;
	types.ldate = DOGET_DTE;

	/* Checking id */
	prepare();
	long long id = get_id( argc, argv );
	if ( id == 0 )
		return 2;

	/* Naming file metadata */
	char output_dir[1024];
	char name_descript[2048];
	if ( acc.usr_ok == 0 )
	{
		snprintf( output_dir, 1024, "u_%lld", acc.id );
		snprintf( name_descript, 2048, "%lld: %s: %s %s\n", id, acc.screenname, acc.usr_fname, acc.usr_lname );
	}
	else if ( acc.grp_ok == 0 )
	{
		snprintf( output_dir, 1024, "c_%lld", acc.id );
		snprintf( name_descript, 2048, "%lld: %s: %s\n", id, acc.screenname, acc.grp_name );
	}
	else
	{
		fprintf( stderr, "Screenname is invalid.\n");
		return 3;
	}

	/* Creating dir for current id */
	fix_filename(output_dir);
	if ( mkdir( output_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
			fprintf( stderr, "mkdir() error (%d).\n", errno );

	char name_dsc_path[BUFSIZ];
	snprintf( name_dsc_path, BUFSIZ, "%s/%s", output_dir, FILNAME_IDNAME );

	FILE * u_name = fopen( name_dsc_path, "w" );
	fprintf( u_name, "%s", name_descript );
	fclose(u_name);

	/* Getting wall content */
	get_wall(output_dir);

	/* Getting albums content */
	photos_count = 0;
	if ( types.pictr == 1 )
	{
		size_t arr_size = get_albums();
		if ( arr_size > 0 )
		{
			get_albums_files( arr_size, output_dir );
			free(albums);
		}
	}

	/* Getting documents */
	if ( types.docmt == 1 )
		get_docs(output_dir);

	if ( id > 0 )
	{
		/* These are fast */
		get_friends(output_dir);
		api_request_pause();
		get_groups(output_dir);
		api_request_pause();
	}

	if ( types.video == 1 )
		get_videos(output_dir);

	curl_easy_cleanup(curl);
	return 0;
}
