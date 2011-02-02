#ifndef MONITOR_H
#define MONITOR_H

struct gme_t;

void monitor_start( gme_t *, const char * );
void monitor_apply( gme_t * );
void monitor_update( gme_t * );
void monitor_stop( const gme_t * );

#endif