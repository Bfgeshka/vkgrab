#include "methods.c"

int
main( int argc, char ** argv )
{
	struct data_user usr;
	usr = user("teuprime");
	if ( usr.is_ok == 0 )
		printf("User: %s %s (%s)\nUser ID: %lld\nIs hidden: %lld\n", usr.fname, usr.lname, usr.screenname, usr.uid, usr.hidden);

	struct data_group grp;
	grp = group("ulululuki");
	if ( grp.is_ok == 0 )
		printf("\nGroup: %s (%s)\nGroup ID: %lld\nType: %s\nIs closed: %lld\n", grp.name, grp.screenname, grp.gid, grp.type, grp.is_closed);

	return 0;
}
