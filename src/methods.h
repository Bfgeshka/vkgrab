#ifndef METHODS_H_
#define METHODS_H_

#include <curl/curl.h>
#include <jansson.h>
#include "../config.h"

#define NAME_BUF 512

struct data_account
{
	long long id;
	char screenname[NAME_BUF/2];
	char usr_fname[NAME_BUF/2];
	char usr_lname[NAME_BUF/2];
	char grp_name[NAME_BUF/2];
	char grp_type[NAME_BUF/4];

	/* 0 means ok: */
	short grp_ok;
	short usr_ok;
} acc;
//struct data_account acc;

struct data_album
{
	long long aid;
	long long size;
	char title[NAME_BUF];
};

struct control_datatypes
{
	short docmt;
	short pictr;
	short video;
	short comts;
} types;

long long photos_count;
struct data_album * albums;

const char * js_get_str        ( json_t *, char * );
long long    js_get_int        ( json_t *, char * );
short        group             ( char *, CURL * );
short        user              ( char *, CURL * );
void         api_request_pause ( void );
void         dl_document       ( char *, char *, json_t *, CURL *, FILE *, long long, long long );
void         dl_photo          ( char *, char *, json_t *, CURL *, FILE *, long long, long long );
void         dl_video          ( char *, char *, json_t *, CURL *, FILE *, long long, long long );
void         fix_filename      ( char * );
void         help_print        ( void );
void         parse_attachments ( char *, char *, json_t *, CURL *, FILE *, long long, long long );

size_t       get_albums        ( CURL * );
long long    get_id            ( int, char **, CURL * );
void         get_albums_files  ( size_t, char *, CURL * );
void         get_comments      ( char *, char *, CURL *, FILE *, long long );
void         get_wall          ( char *, CURL * );
void         get_docs          ( char *, CURL * );
void         get_friends       ( char *, CURL * );
void         get_groups        ( char *, CURL * );
void         get_videos        ( char *, CURL * );
void         check_token       ( void );

#endif
