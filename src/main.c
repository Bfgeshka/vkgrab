#include <stdio.h>
#include <stdlib.h>

#include "../config.h"
#include "methods.h"
#include "utils.h"

int
main( int argc, char ** argv )
{
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

	sstring output_dir;
	newstring( &output_dir, 256 );

	if ( make_dir( &output_dir, id ) > 0 )
		return 3;

	/* Getting wall content */
	get_wall(output_dir.c);

	/* Getting albums content */
	photos_count = 0;
	if ( types.pictr == 1 )
	{
		size_t arr_size = get_albums();
		if ( arr_size > 0 )
		{
			get_albums_files( arr_size, output_dir.c );
			free(albums);
		}
	}

	/* Getting documents */
	if ( types.docmt == 1 )
		get_docs(output_dir.c);

	if ( id > 0 )
	{
		/* These are fast */
		get_friends(output_dir.c);
		api_request_pause();
		get_groups(output_dir.c);
		api_request_pause();
	}

	if ( types.video == 1 )
		get_videos(output_dir.c);

	free(output_dir.c);
	destroy_all();
	return 0;
}
