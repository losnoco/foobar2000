#ifndef _FILEHACKS_H_
#define _FILEHACKS_H_

#include <reader.h>
typedef reader WFILE;

int wfgetlen(WFILE *f);
int wfread( void *buffer, int size, int count, WFILE *stream );
int wfseek( WFILE *stream, long offset, int origin );
long wftell(WFILE *);
int wfgetc(WFILE *);
int wfeof(WFILE *);

#endif