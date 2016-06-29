#include "methods.c"

int
main( int argc, char ** argv )
{
	struct data_user usr;
	struct data_group grp;

	if ( argc == 1 || argc > 3 )
	{
		puts("Wrong arguments.");
		puts("Usage: vk_grabber <USER|GROUP>");
		puts("\tOR");
		puts("\tvk_grabber -u USER");
		puts("\tvk_grabber -g GROUP");
		return 1;
	}
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

	if ( usr.is_ok == 0 )
		printf("User: %s %s (%s)\nUser ID: %lld\nIs hidden: %lld\n", usr.fname, usr.lname, usr.screenname, usr.uid, usr.hidden);
	if ( grp.is_ok == 0 )
		printf("Group: %s (%s)\nGroup ID: %lld\nType: %s\nIs closed: %lld\n", grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed);


	return 0;
}
