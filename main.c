#include <stdarg.h>
#include "methods.c"

int
main()
{
	struct data_user usr;
	usr = user("teuprime");
	if ( usr.is_ok == 0 )
		printf("User: %s %s\nUser ID: %lld\nIs hidden: %lld\n", usr.fname, usr.lname, usr.uid, usr.hidden);

	return 0;
}
