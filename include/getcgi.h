/******************************************************
 *
 * GETCGI.H -  Nagios CGI Input Routine Include File
 *
 *
 *****************************************************/

#include "lib/lnag-utils.h"
NAGIOS_BEGIN_DECL

#define ACCEPT_LANGUAGE_Q_DELIMITER	";q="

#define NAGFORMID_COOKIE_NAME        "NagFormId"
#define NAGFORMID_COOKIE_PREFIX      NAGFORMID_COOKIE_NAME "="
#define NAGFORMID_SECURE_COOKIE_NAME "__Host-NagFormId"
#define NAGFORMID_SECURE_COOKIE_PREFIX NAGFORMID_SECURE_COOKIE_NAME "="
#define NAGFORMID_TOKEN_BYTES        32
#define NAGFORMID_TOKEN_HEX_LENGTH   (NAGFORMID_TOKEN_BYTES * 2)

/* information for a single language in the variable HTTP_ACCEPT_LANGUAGE
	sent by the browser */
typedef struct accept_language_struct {
	char *	language;
	char *	locality;
	double	q;
} accept_language;

/* information for all languages in the variable HTTP_ACCEPT_LANGUAGE
	sent by the browser */
typedef struct accept_languages_struct {
	int					count;
	accept_language **	languages;
} accept_languages;

char **getcgivars(void);
void free_cgivars(char **);
void unescape_cgi_input(char *);
void sanitize_cgi_input(char **);
unsigned char hex_to_char(char *);
int generate_nagformid_token(char *, size_t);
int nagformid_request_is_https(void);
void set_nagformid_cookie_header(const char *);

void	process_language( char *);
accept_languages *	parse_accept_languages( char *);
int compare_accept_languages( const void *, const void *);
void	free_accept_languages( accept_languages *);

NAGIOS_END_DECL
