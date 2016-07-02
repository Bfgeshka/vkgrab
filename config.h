/* Size for usual string and large text */
#define bufs 256
#define bufl 10240

/*
 * How to get access token:
 * https://new.vk.com/dev/auth_mobile
 * Token is not needed for proper work yet, it's for future features.
 */
#define TOKEN "&access_token=blahblahblah"

/* 1L - verbose curl connection, 0L - silent */
#define CRL_VERBOSITY 0L

/* Limitations forr one-time photos request. Current is 1000 */
#define LIMIT_A 1000
