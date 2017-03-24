#include <curl/curl.h>
#include <errno.h>
#include <sys/stat.h>

#include "./functions.h"
#include "../../src/methods.h"

/*
 * cc ./main.c ./functions.c -O2 -Wall -Wextra -Wpedantic --std=c99 -D_DEFAULT_SOURCE -ljansson -lcurl -o blacklist_finder
 */

int
main( int argc, char ** argv )
{
	grp.curl = curl_easy_init();

/*	check_token();*/

	int grp_check_value = group_id( argc, argv );
	if ( grp_check_value == 0 )
		return 0;
	else if ( grp_check_value < 0 )
		return -1;

	long long * ids;
	ids = malloc( sizeof(long long) * grp.sub_count );

	if ( group_memb( ids ) < 0 )
		return -1;

	char output_dir[BUFSIZ];
	char output_file[BUFSIZ];
	sprintf( output_dir, "c_%s", grp.name_scrn );
	if ( mkdir( output_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
		{
			fprintf( stderr, "mkdir() error (%d).\n", errno );
			return -2;
		}

	long long i = grp.sub_count - 1;
	for ( ; i != 0 ; --i )
	{
		printf( "\n[%7lld/%7lld] ", i + 1, grp.sub_count );
		sprintf( output_file, "%s/u_%lld", output_dir, ids[i] );
		user_subs( ids[i], output_file );

	}

	curl_easy_cleanup( grp.curl );
	return 0;
}
