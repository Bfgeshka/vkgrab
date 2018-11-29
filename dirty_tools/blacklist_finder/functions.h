#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <jansson.h>
#include <curl/curl.h>
#include "../../config.h"
#include "../../src/utils.h"

/* Point of interest */
/*#define APPLICATION_ID 5525618*/
/* Add your constant access token after '=' */
/*#define CONST_TOKEN "&access_token=331b14739333ad8cb26b1c24894e527020f55c0a4ca005912211f219c5231b1dba06b7cbffcffbf20a5bb"*/

/* Maximum num. of returned IDs per time in groups.getMembers. Default is 1000 */
#define LIMIT_G 1000

/* Maximum num. of returned IDs per time in users.getSubscriptions. Default is 20, maximum is 200 */
#define LIMIT_S 200


struct grp
{
	long long sub_count;
	sstring * name_scrn;
	long long * ids;
} grp;

struct user_numbers
{
	long offset;
	long count;
};

int          cycle_users    ( long long, struct user_numbers *, FILE * );
int          group_id       ( int, char ** );
int          group_memb     ( long long * );
int          user_subs      ( long long, sstring * );
void         bf_help_print  ( void );

#endif
