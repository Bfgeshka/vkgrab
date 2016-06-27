#include <json-c/json.h>
#include <stdarg.h>
#include <stdlib.h>
#include "config.h"
#include "api.c"

int
main()
{
	char * url = "https://api.vk.com/method/users.get?user_ids=teuprime";
	printf("making request to %s\n", url);

	char * r = vk_api(url);

	printf("callback: %s", r);

	return 0;
}



//	json_object * json;
//	enum json_tokener_error jerr = json_tokener_success;
//	json = json_tokener_parse_verbose(cf->payload, &jerr);
