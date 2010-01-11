#ifndef _LOOKUP_H_
#define _LOOKUP_H_

#include <id3tag.h>

# ifdef __cplusplus
extern "C" {
# endif

typedef void (* t_process_callback)(void *, const char *, const char *);

typedef int (* t_validate)(struct id3_frame *);
typedef void (* t_process)(struct id3_frame *, const struct t_read_lookup *, t_process_callback, void *);
typedef void (* t_render)(struct id3_frame *, const struct t_write_lookup *, int, const char **);

#define DECLAREVALIDATE(type) int  type##_validate(struct id3_frame *);
#define DECLAREPROCESS(type)  void type##_process(struct id3_frame *, const struct t_read_lookup *, t_process_callback, void *);
#define DECLARERENDER(type)   void type##_render(struct id3_frame *, const struct t_write_lookup *, int, const char **);

#define DECLAREFUNCS(type) \
	DECLAREVALIDATE(type) \
	DECLAREPROCESS(type) \
	DECLARERENDER(type)

#define DECLAREFUNCSNV(type) \
	DECLAREPROCESS(type) \
	DECLARERENDER(type)

typedef struct t_read_lookup {
    const char * short_name;
    const char * long_name;
    t_validate validate;
    t_process process;
} t_read_lookup;

typedef struct t_write_lookup {
    const char * long_name;
    const char * short_name;
    t_validate validate;
    t_render render;
} t_write_lookup;

DECLAREFUNCS(text)
DECLAREFUNCS(textx)
DECLAREFUNCS(url)
DECLAREFUNCS(urlx)
DECLAREFUNCS(comment)
DECLAREFUNCS(uniqueid)
//DECLAREFUNCS(unsyncedlyrics)

const struct t_read_lookup * read_lookup(register const char * str, register unsigned int len);
const struct t_write_lookup * write_lookup(register const char * str, register unsigned int len);

# ifdef __cplusplus
}
# endif

#endif
