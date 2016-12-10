/*
 * For generation of token getting link.
 * Create your own application, get its ID and insert it here.
 *
 * For creating your application visit https://vk.com/editapp?act=create.
 * Choose "standalone application" and give it a name you like.
 *
 * After go to "My apps": https://vk.com/apps?act=manage
 * Go to 'manage' on the right side.
 *
 * Choose 'settings', and there you can see your application id.
 * Copy it and replace value of APPLICATION_ID.
 */
#define APPLICATION_ID 5525615

/* 1L - verbose curl connection, 0L - silent */
#define CRL_VERBOSITY 0L

/* Limitation for number of photos per request. Current is 1000 */
#define LIMIT_A 1000

/* Limitation for number of wall posts per request. Current is 100 */
#define LIMIT_W 100

/* Limitation for number of videos per request. Current is 200, default is 100 */
#define LIMIT_V 200

/* Limitation for number of comments per request. Current is 100 */
#define LIMIT_C 100

/* File and directory naming for constant values */
#define DIRNAME_DOCS "alb_docs"
#define DIRNAME_WALL "alb_attachments"
#define DIRNAME_ALB_PROF "alb_profile"
#define DIRNAME_ALB_WALL "alb_wall"
#define DIRNAME_ALB_SAVD "alb_saved"
#define DIRNAME_VIDEO "alb_videos"

#define FILNAME_POSTS "wall.txt"
#define FILNAME_DOCS "documents.txt"
#define FILNAME_FRIENDS "friends.txt"
#define FILNAME_GROUPS "communities.txt"
#define FILNAME_VIDEOS "videos.txt"
#define FILNAME_IDNAME "description.txt"

#define TMP_CURL_FILENAME "/tmp/vkgrab.tmp"

/* Diividing string between posts in FILNAME_POSTS */
#define LOG_POSTS_DIVIDER "-~-~-~\n~-~-~-\n\n"

/* Sometimes program runs too fast and VK returns errors because of too often api requests.
 * This timer slows program down before making a request. It wouldn't be applied to file downloading.
 * Default value is 200000, which means 0.2s.
 *
 * Only 3 requests per second are allowed.
 *
 * I had no other choice, don't be harsh. */
#define USLEEP_INT 200000

/* Defines which file types would be downloaded; 0 means skip, 1 means download.
 * These are default values, can be overriden. Read vkgrab -h for more info. */
#define DOGET_VID 1
#define DOGET_DOC 1
#define DOGET_PIC 1
#define DOGET_COM 1

/* Size for usual string. */
#define bufs 1024

/*
 * Better use vkgrab -T for getting temporary token! Read above.
 *
 * How to get access token:
 * https://vk.com/dev/auth_mobile
 * recommended permissions: audio,video,docs,photos
 *
 * Example:
 * char TOKEN[bufs] "&access_token=blahblahblah"
 *
 * Use 'offline' permission if you want permanent token. Be careful with it.
 */
#define permissions "video,docs,photos"
#define TOKEN_HEAD "&access_token="
char TOKEN[bufs] = TOKEN_HEAD;

/* Currently used api version */
#define api_ver "5.60"
