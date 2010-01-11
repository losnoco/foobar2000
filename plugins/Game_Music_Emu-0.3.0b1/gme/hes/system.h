#ifndef _SYSTEM_H_
#define _SYSTEM_H_

/* Function prototypes */
int system_init(FESTALON_HES *hes);
void audio_init(int rate);
void system_frame(FESTALON_HES *hes);
void system_reset(FESTALON_HES *);
void system_shutdown(FESTALON_HES *);

#endif /* _SYSTEM_H_ */

