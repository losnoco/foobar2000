
#ifndef _PSG_H_
#define _PSG_H_

/* Function prototypes */
int psg_init(FESTALON_HES *hes);
void psg_reset(FESTALON_HES *hes);
void psg_shutdown(FESTALON_HES *hes);
void psg_w(FESTALON_HES *hes, uint16 address, uint8 data);
void psg_update(FESTALON_HES *hes);
uint32 psg_flush(FESTALON_HES *hes);
#endif /* _PSG_H_ */

