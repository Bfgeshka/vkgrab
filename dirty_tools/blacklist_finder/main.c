#include <curl/curl.h>
#include <errno.h>
#include <sys/stat.h>

#include "./functions.h"
#include "../../src/methods.h"
#include "../../src/utils.h"

int
main( int argc, char ** argv )
{
	sstring * output_dir = construct_string(2048);
	sstring * output_file = construct_string(2048);
	long long * ids;
	int retvalue = 0;

	grp.name_scrn = construct_string(2048);
	extern CURL * curl;
	curl = curl_easy_init();

	int grp_check_value = group_id( argc, argv );
	if ( grp_check_value == 0 )
	{
		retvalue = 0;
		goto main_end_mark;
	}
	else if ( grp_check_value < 0 )
	{
		retvalue = -1;
		goto main_end_mark;
	}

	ids = malloc( sizeof(long long) * grp.sub_count );

	if ( group_memb(ids) < 0 )
	{
		retvalue = -2;
		goto main_end_mark;
	}

	stringset( output_dir, "c_%s", grp.name_scrn->c );
	if ( mkdir( output_dir->c, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH ) != 0 )
		if ( errno != EEXIST )
		{
			fprintf( stderr, "mkdir() error (%d).\n", errno );
			retvalue = -3;
			goto main_end_mark;
		}

	long long i = grp.sub_count - 1;
	for ( ; i != 0 ; --i )
	{
		printf( "\n[%7lld/%7lld] ", i + 1, grp.sub_count );
		stringset( output_file, "%s/u_%lld", output_dir->c, ids[i] );
		user_subs( ids[i], output_file );
	}

	main_end_mark:
	free_string(output_dir);
	free_string(output_file);
	curl_easy_cleanup(curl);
	return retvalue;
}
