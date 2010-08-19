#ifndef MONITOR_H
#define MONITOR_H

struct DUMB_IT_SIGDATA;
struct DUMB_IT_SIGRENDERER;

void monitor_start( DUMB_IT_SIGDATA *, DUMB_IT_SIGRENDERER *, const char * );
void monitor_update( DUMB_IT_SIGRENDERER * );
void monitor_stop( const DUMB_IT_SIGRENDERER * );

#endif