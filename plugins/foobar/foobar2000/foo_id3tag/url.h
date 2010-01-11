#ifndef _URL_H_
#define _URL_H_

# ifdef __cplusplus
extern "C" {
# endif

typedef struct php_url {
	char *scheme;
	char *user;
	char *pass;
	char *host;
	unsigned short port;
	char *path;
	char *query;
	char *fragment;
} php_url;

void php_url_free(php_url *theurl);
php_url *php_url_parse(char const *str);

int is_valid_url(const char * str);

# ifdef __cplusplus
}
# endif

#endif