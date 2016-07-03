/* Size for usual string */
#define bufs 256

/*
 * How to get access token:
 * https://new.vk.com/dev/auth_mobile
 *
 * Example:
 * char TOKEN[bufs] "&access_token=blahblahblah"
 */

#define TOKEN_HEAD "&access_token="
char TOKEN[bufs] = TOKEN_HEAD;

/* 1L - verbose curl connection, 0L - silent */
#define CRL_VERBOSITY 0L

/* Limitations for one-time photos request. Current is 1000 */
#define LIMIT_A 1000

/* Limitations for one-time wall posts request. Current is 100 */
#define LIMIT_W 100
