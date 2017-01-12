#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

/* Point of interest */
#define APPLICATION_ID 5521111
/* Add your constant access token after '=' */
#define CONST_TOKEN "&access_token="

/* Maximum num. of returned IDs per time in groups.getMembers. Default is 1000 */
#define GROUP_MEMBERS_COUNT 1000

/* Maximum num. of returned IDs per time in users.getSubscriptions. Default is 20, maximum is 200 */
#define USER_SUBSCRIPTIONS_COUNT 200

#define USLEEP_INT 300000

#include <jansson.h>
#include <curl/curl.h>


#define API_VERSION "5.62"
#define BUF_STRING 512
#define PERMISSIONS "offline"
#define REQ_HEAD "https://api.vk.com/method"
#define TOKEN_HEAD "&access_token="


struct group
{
	long long sub_count;
	char name_scrn[BUF_STRING];
	long long * ids;

} group;

struct crl_st
{
	char * payload;
	size_t size;
};


CURLcode     crl_fetch    ( CURL *, const char *, struct crl_st * );
char *       api_request  ( const char *, CURL * );
const char * js_get_str   ( json_t *, char * );
int          group_id     ( int, char **, CURL * );
int          group_memb   ( CURL *, long long * );
int          user_subs    ( long long, CURL *, char * );
long long    js_get_int   ( json_t *, char * );
size_t       crl_callback ( void *, size_t, size_t, void * );
void         check_token  ( void );
void         help_print   ( void );
void         json_error   ( json_t * );
void         api_request_pause ( void );

#endif
