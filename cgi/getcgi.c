/******************************************
 *
 * GETCGI.C -  Nagios CGI Input Routines
 *
 *
 *****************************************/

#include "../include/config.h"
#include "../include/getcgi.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>


#undef PARANOID_CGI_INPUT


int nagformid_request_is_https(void) {
	const char *https = getenv("HTTPS");
	const char *scheme = getenv("REQUEST_SCHEME");

	return (https != NULL && (!strcasecmp(https, "on") || !strcmp(https, "1"))) ||
		(scheme != NULL && !strcasecmp(scheme, "https"));
}


int generate_nagformid_token(char *buffer, size_t buffer_length) {
	static const char hex[] = "0123456789abcdef";
	unsigned char random_bytes[NAGFORMID_TOKEN_BYTES];
	ssize_t bytes_read;
	size_t offset = 0;
	size_t i;
	int fd;

	if(buffer == NULL || buffer_length < NAGFORMID_TOKEN_HEX_LENGTH + 1)
		return -1;

	do {
		fd = open("/dev/urandom", O_RDONLY);
		} while(fd < 0 && errno == EINTR);
	if(fd < 0)
		return -1;

	while(offset < sizeof(random_bytes)) {
		bytes_read = read(fd, random_bytes + offset, sizeof(random_bytes) - offset);
		if(bytes_read < 0 && errno == EINTR)
			continue;
		if(bytes_read <= 0) {
			close(fd);
			return -1;
			}
		offset += (size_t)bytes_read;
		}
	close(fd);

	for(i = 0; i < sizeof(random_bytes); i++) {
		buffer[i * 2] = hex[random_bytes[i] >> 4];
		buffer[i * 2 + 1] = hex[random_bytes[i] & 0x0f];
		}
	buffer[NAGFORMID_TOKEN_HEX_LENGTH] = '\0';
	return 0;
}


static void nagformid_cookie_path(char *buffer, size_t buffer_length) {
	const char *script_name = getenv("SCRIPT_NAME");
	char *last_slash;
	size_t i;

	if(buffer == NULL || buffer_length == 0)
		return;
	strncpy(buffer, "/", buffer_length);
	buffer[buffer_length - 1] = '\0';
	if(script_name == NULL || script_name[0] != '/' || strlen(script_name) >= buffer_length)
		return;

	for(i = 0; script_name[i] != '\0'; i++)
		if((unsigned char)script_name[i] <= 0x20 || (unsigned char)script_name[i] >= 0x7f || script_name[i] == ';')
			return;

	strncpy(buffer, script_name, buffer_length);
	buffer[buffer_length - 1] = '\0';
	last_slash = strrchr(buffer, '/');
	if(last_slash == NULL) {
		strncpy(buffer, "/", buffer_length);
		buffer[buffer_length - 1] = '\0';
		return;
		}
	last_slash[1] = '\0';
}


void set_nagformid_cookie_header(const char *token) {
	char cookie_path[1024];

	if(token == NULL)
		return;
	if(nagformid_request_is_https()) {
		/* __Host- cookies cannot be set by sibling hosts and require Path=/,
		 * Secure, and no Domain attribute. */
		printf("Set-Cookie: " NAGFORMID_SECURE_COOKIE_NAME "=%s; Path=/; HttpOnly; SameSite=Strict; Secure\r\n", token);
		return;
		}

	nagformid_cookie_path(cookie_path, sizeof(cookie_path));
	printf("Set-Cookie: " NAGFORMID_COOKIE_NAME "=%s; Path=%s; HttpOnly; SameSite=Strict\r\n", token, cookie_path);
}


/* Remove potentially harmful characters from CGI input that we don't need or want */
void sanitize_cgi_input(char **cgivars) {
	char *strptr;
	int x, y, i;
	int keep;

	/* don't strip for now... */
	return;

	for(strptr = cgivars[i = 0]; strptr != NULL; strptr = cgivars[++i]) {

		for(x = 0, y = 0; strptr[x] != '\x0'; x++) {

			keep = 1;

			/* remove potentially nasty characters */
			if(strptr[x] == ';' || strptr[x] == '|' || strptr[x] == '&' || strptr[x] == '<' || strptr[x] == '>')
				keep = 0;
#ifdef PARANOID_CGI_INPUT
			else if(strptr[x] == '/' || strptr[x] == '\\')
				keep = 0;
#endif
			if(keep == 1)
				strptr[y++] = strptr[x];
			}

		strptr[y] = '\x0';
		}
	}


/* convert encoded hex string (2 characters representing an 8-bit number) to its ASCII char equivalent */
unsigned char hex_to_char(char *input) {
	unsigned char outchar = '\x0';
	unsigned int outint = 0;
	char tempbuf[3];

	/* NULL or empty string */
	if(input == NULL)
		return '\x0';
	if(input[0] == '\x0' || input[1] == '\x0' ||
			!isxdigit((unsigned char)input[0]) || !isxdigit((unsigned char)input[1]))
		return '\x0';

	tempbuf[0] = input[0];
	tempbuf[1] = input[1];
	tempbuf[2] = '\x0';

	sscanf(tempbuf, "%X", &outint);

	/* only convert "normal" ASCII characters - we don't want the rest.  Normally you would
	   convert all characters (i.e. for allowing users to post binary files), but since we
	   aren't doing this, stay on the cautious side of things and reject outsiders... */
#ifdef PARANOID_CGI_INPUT
	if(outint < 32 || outint > 126)
		outint = 0;
#endif

	outchar = (unsigned char)outint;

	return outchar;
	}



/* unescape hex characters in CGI input */
void unescape_cgi_input(char *input) {
	int x, y;
	int len;

	if(input == NULL)
		return;

	len = strlen(input);
	for(x = 0, y = 0; x < len; x++, y++) {

		if(input[x] == '\x0')
			break;
		else if(input[x] == '%' && x + 2 < len &&
				isxdigit((unsigned char)input[x + 1]) && isxdigit((unsigned char)input[x + 2])) {
			input[y] = hex_to_char(&input[x + 1]);
			x += 2;
			}
		else
			input[y] = input[x];
		}
	input[y] = '\x0';
	}



/* read the CGI input and place all name/val pairs into list. returns list containing name1, value1, name2, value2, ... , NULL */
/* this is a hacked version of a routine I found a long time ago somewhere - can't remember where anymore */
char **getcgivars(void) {
	register int i;
	char *accept_lang;
	char *request_method;
	char *content_type;
	char *content_length_string;
	int content_length;
	char *cgiinput;
	char **cgivars;
	char **pairlist;
	int paircount;
	int pairlist_capacity;
	char *nvpair;
	char *eqpos;
	char *cookies, *formid, *cookie_cursor, *cookie_end;
	char *decoded_name, *raw_equals;
	const char *cookie_prefix;
	size_t formid_len, cookie_prefix_len, cookie_segment_len, raw_name_len, canonical_prefix_len;

	/* initialize char variable(s) */
	cgiinput = "";

	/* Attempt to set the locale */
	accept_lang = getenv("HTTP_ACCEPT_LANGUAGE");
	process_language(accept_lang);

	/* depending on the request method, read all CGI input into cgiinput */

	request_method = getenv("REQUEST_METHOD");
	if(request_method == NULL)
		request_method = "";

	if(!strcmp(request_method, "GET") || !strcmp(request_method, "HEAD")) {

		/* check for NULL query string environment variable - 04/28/00 (Ludo Bosmans) */
		if(getenv("QUERY_STRING") == NULL) {
			cgiinput = (char *)malloc(1);
			if(cgiinput != NULL)
				cgiinput[0] = '\x0';
			}
		else
			cgiinput = strdup(getenv("QUERY_STRING"));
		if(cgiinput == NULL) {
			printf("getcgivars(): Could not allocate memory for CGI input.\n");
			exit(1);
			}
		}

	else if(!strcmp(request_method, "POST") || !strcmp(request_method, "PUT")) {

		/* if CONTENT_TYPE variable is not specified, RFC-2068 says we should assume it is "application/octet-string" */
		/* mobile (WAP) stations generate CONTENT_TYPE with charset, we we should only check first 33 chars */

		content_type = getenv("CONTENT_TYPE");
		if(content_type == NULL)
			content_type = "";

		if(strlen(content_type) && strncasecmp(content_type, "application/x-www-form-urlencoded", 33)) {
			printf("getcgivars(): Unsupported Content-Type.\n");
			exit(1);
			}

		content_length_string = getenv("CONTENT_LENGTH");
		if(content_length_string == NULL)
			content_length_string = "0";

		if(!(content_length = atoi(content_length_string))) {
			printf("getcgivars(): No Content-Length was sent with the POST request.\n") ;
			exit(1);
			}
		/* suspicious content length */
		if((content_length < 0) || (content_length >= INT_MAX - 1)) {
			printf("getcgivars(): Suspicious Content-Length was sent with the POST request.\n");
			exit(1);
			}

		if(!(cgiinput = (char *)malloc(content_length + 1))) {
			printf("getcgivars(): Could not allocate memory for CGI input.\n");
			exit(1);
			}
		if(!fread(cgiinput, content_length, 1, stdin)) {
			printf("getcgivars(): Could not read input from STDIN.\n");
			exit(1);
			}
		cgiinput[content_length] = '\0';
		}
	else {

		printf("getcgivars(): Unsupported REQUEST_METHOD -> '%s'\n", request_method);
		printf("\n");
		printf("I'm guessing you're trying to execute the CGI from a command line.\n");
		printf("In order to do that, you need to set the REQUEST_METHOD environment\n");
		printf("variable to either \"GET\", \"HEAD\", or \"POST\".  When using the\n");
		printf("GET and HEAD methods, arguments can be passed to the CGI\n");
		printf("by setting the \"QUERY_STRING\" environment variable.  If you're\n");
		printf("using the POST method, data is read from standard input.  Also of\n");
		printf("note: if you've enabled authentication in the CGIs, you must set the\n");
		printf("\"REMOTE_USER\" environment variable to be the name of the user you're\n");
		printf("\"authenticated\" as.\n");
		printf("\n");

		exit(1);
		}

	/* change all plus signs back to spaces */
	for(i = 0; cgiinput[i]; i++) {
		if(cgiinput[i] == '+')
			cgiinput[i] = ' ';
		}

	/* first, split on ampersands (&) to extract the name-value pairs into pairlist */
	/* allocate memory for 256 name-value pairs at a time, increasing by same
	   amount as necessary... */
	pairlist_capacity = 256;
	pairlist = (char **)malloc(pairlist_capacity * sizeof(char *));
	if(pairlist == NULL) {
		printf("getcgivars(): Could not allocate memory for name-value pairlist.\n");
		exit(1);
		}
	paircount = 0;
	nvpair = strtok(cgiinput, "&");
	while(nvpair) {
		/* NagFormId must come from a cookie. Decode the complete parameter name
		 * before comparing so percent-encoding cannot manufacture the trusted name. */
		raw_equals = strchr(nvpair, '=');
		raw_name_len = raw_equals == NULL ? strlen(nvpair) : (size_t)(raw_equals - nvpair);
		decoded_name = (char *)malloc(raw_name_len + 1);
		if(decoded_name == NULL) {
			printf("getcgivars(): Could not allocate memory for CGI parameter name.\n");
			exit(1);
			}
		memcpy(decoded_name, nvpair, raw_name_len);
		decoded_name[raw_name_len] = '\0';
		unescape_cgi_input(decoded_name);
		if(!strcmp(decoded_name, NAGFORMID_COOKIE_NAME) || !strcmp(decoded_name, NAGFORMID_SECURE_COOKIE_NAME)) {
			free(decoded_name);
			nvpair = strtok(NULL, "&");
			continue;
			}
		free(decoded_name);

		/* Always retain room for this entry and the terminating NULL. */
		if(paircount + 2 > pairlist_capacity) {
			pairlist_capacity += 256;
			pairlist = (char **)realloc(pairlist, pairlist_capacity * sizeof(char *));
			if(pairlist == NULL) {
				printf("getcgivars(): Could not re-allocate memory for name-value pairlist.\n");
				exit(1);
				}
			}
		pairlist[paircount] = strdup(nvpair);
		if( NULL == pairlist[paircount]) {
			printf("getcgivars(): Could not allocate memory for name-value pair #%d.\n", paircount);
			exit(1);
			}
		paircount++;
		nvpair = strtok(NULL, "&");
		}

	/* See if there is a NagFormId cookie & get it if it's available */

	cookies = getenv("HTTP_COOKIE");
	if (cookies && *cookies) {
		cookie_prefix = nagformid_request_is_https() ? NAGFORMID_SECURE_COOKIE_PREFIX : NAGFORMID_COOKIE_PREFIX;
		cookie_prefix_len = strlen(cookie_prefix);
		canonical_prefix_len = strlen(NAGFORMID_COOKIE_PREFIX);
		formid = NULL;
		cookie_cursor = cookies;
		while(*cookie_cursor) {
			/* Cookie pairs are separated by semicolons with optional spaces/tabs.
			 * Whitespace inside another cookie's value is not a valid boundary. */
			while(*cookie_cursor == ' ' || *cookie_cursor == '\t')
				cookie_cursor++;
			cookie_end = strchr(cookie_cursor, ';');
			cookie_segment_len = cookie_end == NULL ? strlen(cookie_cursor) : (size_t)(cookie_end - cookie_cursor);
			if(cookie_segment_len == cookie_prefix_len + NAGFORMID_TOKEN_HEX_LENGTH &&
					!strncmp(cookie_cursor, cookie_prefix, cookie_prefix_len)) {
				formid = cookie_cursor;
				break;
				}
			if(cookie_end == NULL)
				break;
			cookie_cursor = cookie_end + 1;
			}
		if (formid) {
			if(paircount + 2 > pairlist_capacity) {
				pairlist_capacity += 256;
				pairlist = (char **)realloc(pairlist, pairlist_capacity * sizeof(char *));
				if(pairlist == NULL) {
					printf("getcgivars(): Could not re-allocate memory for name-value pairlist.\n");
					exit(1);
					}
				}

			formid_len = cookie_segment_len - cookie_prefix_len;
			if (formid_len == NAGFORMID_TOKEN_HEX_LENGTH) {
				for (i = 0; i < (int)formid_len; ++i)
					if (!isxdigit((unsigned char)formid[cookie_prefix_len + i]))
						break;
				if (i == (int)formid_len) {
					pairlist[paircount] = malloc(canonical_prefix_len + formid_len + 1);
					if (!pairlist[paircount]) {
						printf("getcgivars(): Could not allocate memory for name-value pair #%d.\n", paircount);
						exit(1);
					}
					memcpy(pairlist[paircount], NAGFORMID_COOKIE_PREFIX, canonical_prefix_len);
					memcpy(pairlist[paircount] + canonical_prefix_len, formid + cookie_prefix_len, formid_len);
					pairlist[paircount][canonical_prefix_len + formid_len] = '\0';
					paircount++;
					}
				}
			}
		}

	/* terminate the list */
	pairlist[paircount] = NULL;

	/* extract the names and values from the pairlist */
	cgivars = (char **)malloc((paircount * 2 + 1) * sizeof(char *));
	if(cgivars == NULL) {
		printf("getcgivars(): Could not allocate memory for name-value list.\n");
		exit(1);
		}
	for(i = 0; i < paircount; i++) {

		/* get the variable name preceding the equal (=) sign */
		if((eqpos = strchr(pairlist[i], '=')) != NULL) {
			*eqpos = '\0';
			cgivars[i * 2 + 1] = strdup(eqpos + 1);
			if( NULL == cgivars[ i * 2 + 1]) {
				printf("getcgivars(): Could not allocate memory for cgi value #%d.\n", i);
				exit(1);
				}
			unescape_cgi_input(cgivars[i * 2 + 1]);
			}
		else {
			cgivars[i * 2 + 1] = strdup("");
			if( NULL == cgivars[ i * 2 + 1]) {
				printf("getcgivars(): Could not allocate memory for empty stringfor variable value #%d.\n", i);
				exit(1);
				}
			unescape_cgi_input(cgivars[i * 2 + 1]);
			}

		/* get the variable value (or name/value of there was no real "pair" in the first place) */
		cgivars[i * 2] = strdup(pairlist[i]);
		if( NULL == cgivars[ i * 2]) {
			printf("getcgivars(): Could not allocate memory for cgi name #%d.\n", i);
			exit(1);
			}
		unescape_cgi_input(cgivars[i * 2]);
		}

	/* terminate the name-value list */
	cgivars[paircount * 2] = NULL;

	/* free allocated memory */
	free(cgiinput);
	for(i = 0; pairlist[i]; i++)
		free(pairlist[i]);
	free(pairlist);

	/* sanitize the name-value strings */
	sanitize_cgi_input(cgivars);

	/* return the list of name-value strings */
	return cgivars;
	}

/* Set the locale based on the HTTP_ACCEPT_LANGUAGE variable value sent by
	the browser */
void process_language( char * accept_lang) {
	accept_languages *accept_langs = NULL;
	int x;
	char	locale_string[ 64];
	char *	locale = NULL;
	char *	locale_failsafe[] = { "en_US.utf8", "POSIX", "C" };

	if( accept_lang != NULL) {
		accept_langs = parse_accept_languages( accept_lang);
	}

	if( NULL != accept_langs) {
		/* Sort the results */
		qsort( accept_langs->languages, accept_langs->count,
				sizeof( accept_langs->languages[ 0]), compare_accept_languages);

		/* Try each language in order of priority */
		for( x = 0; (( x < accept_langs->count) && ( NULL == locale)); x++) {
			accept_language *l;
			l = accept_langs->languages[x];
			if (!l || !l->locality || !l->language)
				continue;
			snprintf( locale_string, sizeof( locale_string), "%s_%s.%s",
					l->language, l->locality, "utf8");
			locale = setlocale( LC_ALL, locale_string);
		}

		free_accept_languages( accept_langs);
	}
	if( NULL == locale) { /* Still isn't set */
		/* Try the fail safe locales */
		for( x = 0; (( x < (int)( sizeof( locale_failsafe) / sizeof( char *))) &&
				( NULL == locale)); x++) {
			locale = setlocale( LC_ALL, locale_failsafe[ x]);
		}
	}
}

accept_languages * parse_accept_languages( char * acceptlang) {
	char *	langdup;	/* Duplicate of accept language for parsing */
	accept_languages	*langs = NULL;
	char *	langtok;	/* Language token (language + locality + q value) */
	char *	saveptr;	/* Save state of tokenization */
	char *	qdelim;		/* location of the delimiter ';q=' */
	char *	localitydelim;	/* Delimiter for locality specifier */
	int		x;
	char *	stp;

	/* If the browser did not pass an HTTP_ACCEPT_LANGUAGE variable, there
		is not much we can do */
	if( NULL == acceptlang) {
		return NULL;
	}

	/* Duplicate the browser supplied HTTP_ACCEPT_LANGUAGE variable */
	if( NULL == ( langdup = strdup( acceptlang))) {
		printf( "Unable to allocate memory for langdup\n");
		return NULL;
	}

	/* Initialize the structure to contain the parsed HTTP_ACCEPT_LANGUAGE
		information */
	if( NULL == ( langs = malloc( sizeof( accept_languages)))) {
		printf( "Unable to allocate memory for langs\n");
		free( langdup);
		return NULL;
	}
	langs->count = 0;
	langs->languages = ( accept_language **)NULL;

	/* Tokenize the HTTP_ACCEPT_LANGUAGE string */
	langtok = strtok_r( langdup, ",", &saveptr);
	while( langtok != NULL) {
		/* Bump the count and allocate memory for structures */
		langs->count++;
		if( NULL == langs->languages) {
			/* Adding first language */
			if( NULL == ( langs->languages =
					malloc( langs->count * sizeof( accept_language *)))) {
				printf( "Unable to allocate memory for first language\n");
				langs->count--;
				free_accept_languages( langs);
				free( langdup);
				return NULL;
			}
		}
		else {
			/* Adding additional language */
			if( NULL == ( langs->languages = realloc( langs->languages,
					langs->count * sizeof( accept_language *)))) {
				printf( "Unable to allocate memory for additional language\n");
				langs->count--;
				free_accept_languages( langs);
				free( langdup);
				return NULL;
			}
		}
		if( NULL == ( langs->languages[ langs->count - 1] =
				malloc( sizeof( accept_language)))) {
			printf( "Unable to allocate memory for language\n");
			langs->count--;
			free_accept_languages( langs);
			free( langdup);
			return NULL;
		}
		langs->languages[ langs->count - 1]->language = ( char *)NULL;
		langs->languages[ langs->count - 1]->locality = ( char *)NULL;
		langs->languages[ langs->count - 1]->q = 1.0;

		/* Check to see if there is a q value */
		qdelim = strstr( langtok, ACCEPT_LANGUAGE_Q_DELIMITER);

		if( NULL != qdelim) {	/* found a q value */
			langs->languages[ langs->count - 1]->q =
					strtod( qdelim + strlen( ACCEPT_LANGUAGE_Q_DELIMITER), NULL);
			langtok[ qdelim - langtok] = '\0';
		}

		/* Check to see if there is a locality specifier */
		if( NULL == ( localitydelim = strchr( langtok, '-'))) {
			localitydelim = strchr( langtok, '_');
		}

		if( NULL != localitydelim) {
			/* We have a locality delimiter, so copy it */
			if( NULL == ( langs->languages[ langs->count - 1]->locality =
					strdup( localitydelim + 1))) {
				printf( "Unable to allocate memory for locality '%s'\n",
						langtok);
				free_accept_languages( langs);
				free( langdup);
				return NULL;
			}

			/* Ensure it is upper case */
			for( x = 0, stp = langs->languages[ langs->count - 1]->locality;
					x < (int)strlen( langs->languages[ langs->count - 1]->locality);
					x++, stp++) {
				*stp = toupper( *stp);
			}

			/* Then null out the delimiter so the language copy works */
			*localitydelim = '\0';
		}
		if( NULL == ( langs->languages[ langs->count - 1]->language =
				strdup( langtok))) {
			printf( "Unable to allocate memory for language '%s'\n",
					langtok);
			free_accept_languages( langs);
			free( langdup);
			return NULL;
		}

		/* Get the next language token */
		langtok = strtok_r( NULL, ",", &saveptr);
	}

	free( langdup);
	return langs;
}

int compare_accept_languages( const void *p1, const void *p2) {
	accept_language *	lang1 = *( accept_language **)p1;
	accept_language *	lang2 = *( accept_language **)p2;

	if( lang1->q == lang2->q) {
		if((( NULL == lang1->locality) && ( NULL == lang2->locality)) ||
				(( NULL != lang1->locality) && ( NULL != lang2->locality))) {
			return 0;
		}
		else if( NULL != lang1->locality) {
			return -1;
		}
		else { /* NULL != lang2->locality */
			return 1;
		}
	}
	else {
		return( lang2->q > lang1->q);
	}
}


void free_accept_languages( accept_languages * langs) {
	int	x;

	if( NULL == langs) {
		return;
	}

	for( x = 0; x < langs->count; x++) {
		if( NULL != langs->languages[ x]) {
			if( langs->languages[ x]->language != NULL) {
				free( langs->languages[ x]->language);
			}
			if( langs->languages[ x]->locality != NULL) {
				free( langs->languages[ x]->locality);
			}
			free( langs->languages[ x]);
		}
	}
	if( langs->languages != NULL) {
		free( langs->languages);
	}
	free( langs);
}

/* free() memory allocated to storing the CGI variables */
void free_cgivars(char **cgivars) {
	register int x;

	for(x = 0; cgivars[x]; x++)
		free(cgivars[x]);

	free(cgivars);
	}
