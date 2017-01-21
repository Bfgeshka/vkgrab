#include <curl/curl.h>
#include <errno.h>
#include <sys/stat.h>

#include "./functions.h"

/*
 * cc ./main.c ./functions.c -O2 -Wall -Wextra -Wpedantic --std=c99 -D_DEFAULT_SOURCE -ljansson -lcurl -o blacklist_finder
 */

int
main( int argc, char ** argv )
{
	CURL * curl;
	curl_global_init(CURL_GLOBAL_SSL);
	curl = curl_easy_init();
	if ( !curl )
	{
		fprintf( stderr, "Curl initialisation error.\n" );
		return -1;
	}


	check_token();


	int grp_check_value = group_id( argc, argv, curl );
	if ( grp_check_value == 0 )
		return 0;
	else if ( grp_check_value < 0 )
		return -1;


	long long * ids;
	ids = malloc( sizeof(long long) * group.sub_count );

	if ( group_memb( curl, ids ) < 0 )
		return -1;


	char output_dir[BUF_STRING];
	char output_file[BUF_STRING];
	sprintf( output_dir, "c_%s", group.name_scrn );
	if ( mkdir( output_dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
		{
			fprintf( stderr, "mkdir() error (%d).\n", errno );
			return -2;
		}

	long long i;
	for ( i = 0; i < group.sub_count; ++i )
	{
		printf( "\n[%7lld/%7lld] ", i + 1, group.sub_count );
		sprintf( output_file, "%s/u_%lld", output_dir, ids[i] );
		user_subs( ids[i], curl, output_file );

	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return 0;
}
