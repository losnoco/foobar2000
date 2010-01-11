#include "url.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Basically, this is copied from PHP. I don't want Electric Fence, so I define this instead
   of modifying the code. */

#define ecalloc     calloc
#define efree       free
#define estrndup    strndup
#define strncasecmp strnicmp

#define STR_FREE(ptr) if (ptr) { free(ptr); }

static char * strndup(const char * str, size_t max_len)
{
	char * ret;
	size_t len = strlen(str);
	len = min(len, max_len);
	ret = malloc(len + 1);
	memcpy(ret, str, len);
	ret[len] = 0;
	return ret;
}

void php_url_free(php_url *theurl)
{
	if (theurl->scheme)
		efree(theurl->scheme);
	if (theurl->user)
		efree(theurl->user);
	if (theurl->pass)
		efree(theurl->pass);
	if (theurl->host)
		efree(theurl->host);
	if (theurl->path)
		efree(theurl->path);
	if (theurl->query)
		efree(theurl->query);
	if (theurl->fragment)
		efree(theurl->fragment);
	efree(theurl);
}

static char *php_replace_controlchars(char *str)
{
	unsigned char *s = (unsigned char *)str;

	if (!str) {
		return (NULL);
	}

	while (*s) {

		if (iscntrl(*s)) {
			*s='_';
		}	
		s++;
	}

	return (str);
}

php_url *php_url_parse(char const *str)
{
	int length = strlen(str);
	char port_buf[6];
	php_url *ret = ecalloc(1, sizeof(php_url));
	char const *s, *e, *p, *pp, *ue;
		
	s = str;
	ue = s + length;

	/* parse scheme */
	if ((e = strchr(s, ':')) && (e-s)) {
		/* 
		 * certain schemas like mailto: and zlib: may not have any / after them
		 * this check ensures we support those.
		 */
		if (*(e+1) != '/') {
			/* check if the data we get is a port this allows us to 
			 * correctly parse things like a.com:80
			 */
			p = e + 1;
			while (isdigit(*p)) {
				p++;
			}
			
			if ((*p) == '\0' || *p == '/') {
				goto parse_port;
			}
			
			ret->scheme = estrndup(s, (e-s));
			php_replace_controlchars(ret->scheme);
			
			length -= ++e - s;
			s = e;
			goto just_path;
		} else {
			ret->scheme = estrndup(s, (e-s));
			php_replace_controlchars(ret->scheme);
		
			if (*(e+2) == '/') {
				s = e + 3;
				if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
					if (*(e + 3) == '/') {
						/* support windows drive letters as in:
						   file:///c:/somedir/file.txt
						*/
						if (*(e + 5) == ':') {
							s = e + 4;
						}
						goto nohost;
					}
				}
			} else {
				s = e + 1;
				if (!strncasecmp("file", ret->scheme, sizeof("file"))) {
					goto nohost;
				} else {
					length -= ++e - s;
					s = e;
					goto just_path;
				}	
			}
		}	
	} else if (e) { /* no scheme, look for port */
		parse_port:
		p = e + 1;
		pp = p;
		
		while (pp-p < 6 && isdigit(*pp)) {
			pp++;
		}
		
		if (pp-p < 6 && (*pp == '/' || *pp == '\0')) {
			memcpy(port_buf, p, (pp-p));
			port_buf[pp-p] = '\0';
			ret->port = atoi(port_buf);
		} else {
			goto just_path;
		}
	} else {
		just_path:
		ue = s + length;
		goto nohost;
	}
	
	e = ue;
	
	if (!(p = strchr(s, '/'))) {
		if ((p = strchr(s, '?'))) {
			e = p;
		}
	} else {
		e = p;
	}	
		
	/* check for login and password */
	if ((p = memchr(s, '@', (e-s)))) {
		if ((pp = memchr(s, ':', (p-s)))) {
			if ((pp-s) > 0) {
				ret->user = estrndup(s, (pp-s));
				php_replace_controlchars(ret->user);
			}	
		
			pp++;
			if (p-pp > 0) {
				ret->pass = estrndup(pp, (p-pp));
				php_replace_controlchars(ret->pass);
			}	
		} else {
			ret->user = estrndup(s, (p-s));
			php_replace_controlchars(ret->user);
		}
		
		s = p + 1;
	}

	/* check for port */
	if (*s == '[' && *(e-1) == ']') {
		/* Short circuit portscan, 
		   we're dealing with an 
		   IPv6 embedded address */
		p = s;
	} else {
		/* memrchr is a GNU specific extension
		   Emulate for wide compatability */
		for(p = e; *p != ':' && p >= s; p--);
	}

	if (p >= s && *p == ':') {
		if (!ret->port) {
			p++;
			if (e-p > 5) { /* port cannot be longer then 5 characters */
				STR_FREE(ret->scheme);
				STR_FREE(ret->user);
				STR_FREE(ret->pass);
				efree(ret);
				return NULL;
			} else if (e - p > 0) {
				memcpy(port_buf, p, (e-p));
				port_buf[e-p] = '\0';
				ret->port = atoi(port_buf);
			}
			p--;
		}	
	} else {
		p = e;
	}
	
	/* check if we have a valid host, if we don't reject the string as url */
	if ((p-s) < 1) {
		STR_FREE(ret->scheme);
		STR_FREE(ret->user);
		STR_FREE(ret->pass);
		efree(ret);
		return NULL;
	}

	ret->host = estrndup(s, (p-s));
	php_replace_controlchars(ret->host);
	
	if (e == ue) {
		return ret;
	}
	
	s = e;
	
	nohost:
	
	if ((p = strchr(s, '?'))) {
		pp = strchr(s, '#');
		
		if (pp && pp < p) {
			p = pp;
			pp = strchr(pp+2, '#');
		}
	
		if (p - s) {
			ret->path = estrndup(s, (p-s));
			php_replace_controlchars(ret->path);
		}	
	
		if (pp) {
			if (pp - ++p) { 
				ret->query = estrndup(p, (pp-p));
				php_replace_controlchars(ret->query);
			}
			p = pp;
			goto label_parse;
		} else if (++p - ue) {
			ret->query = estrndup(p, (ue-p));
			php_replace_controlchars(ret->query);
		}
	} else if ((p = strchr(s, '#'))) {
		if (p - s) {
			ret->path = estrndup(s, (p-s));
			php_replace_controlchars(ret->path);
		}	
		
		label_parse:
		p++;
		
		if (ue - p) {
			ret->fragment = estrndup(p, (ue-p));
			php_replace_controlchars(ret->fragment);
		}	
	} else {
		ret->path = estrndup(s, (ue-s));
		php_replace_controlchars(ret->path);
	}

	return ret;
}

/* This method is based on what getID3() does, because I'm too lazy to wade through so
   many RFC docs. Besides, getID3() seems to know its stuff... */

static int is_valid_user(const char * str)
{
	while (*str)
	{
		if (!((*str >= '0' && *str <= '9') ||
			  (*str >= 'A' && *str <= 'Z') ||
			  (*str >= 'a' && *str <= 'z') ||
			  *str == '-')) return 0;
		str++;
	}
	return 1;
}

static int is_valid_host(const char * str)
{
	size_t part = 0;
	while (*str)
	{
		if (*str == '.') part = 0;
		else if ((*str >= '0' && *str <= '9') ||
				 (*str >= 'A' && *str <= 'Z') ||
				 (*str >= 'a' && *str <= 'z') ||
				 *str == '-')
		{
			part++;
		}
		else return 0;
		str++;
	}
	return (part > 1);
}

static int is_valid_dotted_ip(const char * str)
{
	size_t part = 0;
	while (*str)
	{
		if (*str == '.') part = 0;
		else if (*str >= '0' && *str <= '9')
		{
			if (part < 3) part++;
			else return 0;
		}
		else return 0;
		str++;
	}
	return 1;
}

static int is_valid_path(const char * str)
{
	while (*str)
	{
		if (!((*str >= '0' && *str <= '9') ||
			  (*str >= 'A' && *str <= 'Z') ||
			  (*str >= 'a' && *str <= 'z') ||
			  *str == '/' ||
			  *str == '_' ||
			  *str == '\\' ||
			  *str == '.' ||
			  *str == '@' ||
			  *str == '~' ||
			  *str == '-')) return 0;
		str++;
	}
	return 1;
}

static int is_valid_query(const char * str)
{
	while (*str)
	{
		if (!((*str >= '0' && *str <= '9') ||
			  (*str >= 'A' && *str <= 'Z') ||
			  (*str >= 'a' && *str <= 'z') ||
			  *str == '?' ||
			  *str == '&' ||
			  *str == '=' ||
			  *str == '+' ||
			  *str == ':' ||
			  *str == ';' ||
			  *str == '_' ||
			  *str == '(' ||
			  *str == ')' ||
			  *str == '%' ||
			  *str == '#' ||
			  *str == '/' ||
			  *str == ',' ||
			  *str == '\\' ||
			  *str == '.' ||
			  *str == '-')) return 0;
		str++;
	}
	return 1;
}

int is_valid_url(const char * str)
{
	php_url * url;

	if (!str || !*str) return 0;

	url = php_url_parse(str);

	if (!url) return 0;

	if (!url->scheme) return 0;

	if (!stricmp(url->scheme, "mailto"))
	{
		/* address should be in path, query and fragment should be unset, unless there's garbage */

		char * temp;

		if (!url->path || url->query || url->fragment) goto fail;

		temp = strchr(url->path, '@');

		if (!temp) goto fail;

		*temp++ = 0;

		if (!is_valid_user(url->path) || (!is_valid_host(temp) && !is_valid_dotted_ip(temp))) goto fail;
	}
	else if (!stricmp(url->scheme, "http") ||
			 !stricmp(url->scheme, "https") ||
			 !stricmp(url->scheme, "ftp") ||
			 !stricmp(url->scheme, "gopher"))
	{
		if (!url->host) goto fail;
		if (!is_valid_host(url->host) && !is_valid_dotted_ip(url->host)) goto fail;
		if (url->user && !is_valid_user(url->user)) goto fail;
		if (url->pass && !is_valid_user(url->pass)) goto fail;
		if (url->path && !is_valid_path(url->path)) goto fail;
		if (url->query && !is_valid_query(url->query)) goto fail;
		if (url->fragment && !is_valid_user(url->fragment)) goto fail;
	}
	else
		goto fail;

	/* success */

	php_url_free(url);
	return 1;

fail:
	php_url_free(url);
	return 0;
}
