#ifndef METHODS_H_
#define METHODS_H_

#include <jansson.h>
#include "utils.h"
#include "../config.h"

struct data_account
{
	long long id;
	sstring * screenname;
	sstring * usr_fname;
	sstring * usr_lname;
	sstring * grp_name;
	sstring * grp_type;

	/* 0 means ok: */
	short grp_ok;
	short usr_ok;
} acc;

struct data_album
{
	long long aid;
	long long size;
	sstring * title;
};

struct control_datatypes
{
	short docmt;
	short pictr;
	short video;
	short comts;
	short ldate;
} types;

long long photos_count;
struct data_album * albums;

json_t * make_request ( sstring *, json_error_t * );
const char * js_get_str        ( json_t *, char * );
int          readable_date     ( long long, FILE * );
int          make_dir          ( sstring *, long long );
long long    get_id            ( int, char ** );
long long    js_get_int        ( json_t *, char * );
short        group             ( char * );
short        user              ( char * );
size_t       get_albums        ( void );
void         api_request_pause ( void );
void         prepare           ( void );
void         dl_document       ( sstring *, sstring *, json_t *, FILE *, long long, long long );
void         dl_photo          ( sstring *, sstring *, json_t *, FILE *, long long, long long );
void         dl_video          ( sstring *, sstring *, json_t *, FILE *, long long, long long );
void         fix_filename      ( char * );
void         destroy_all       ( void );
void         get_albums_files  ( size_t, char * );
void         get_comments      ( sstring *, sstring *, FILE *, long long );
void         get_docs          ( char * );
void         get_friends       ( char * );
void         get_groups        ( char * );
void         get_videos        ( char * );
void         get_wall          ( char * );
void         help_print        ( void );
void         parse_attachments ( sstring *, sstring *, json_t *, FILE *, long long, long long );


#endif
