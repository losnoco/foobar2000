#ifndef MONITOR_H
#define MONITOR_H

struct Music_Emu;

void monitor_start( Music_Emu *, const char * );
void monitor_update( Music_Emu * );
void monitor_stop( const Music_Emu * );

#endif