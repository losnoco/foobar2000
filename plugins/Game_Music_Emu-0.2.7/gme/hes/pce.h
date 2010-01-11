
#ifndef _PCE_H_
#define _PCE_H_

#define UNMAPPED_MEM    (0xFF)  /* Unmapped memory addresses */
#define UNMAPPED_IO     (0x00)  /* Unmapped I/O addresses */


/* Function prototypes */
int pce_init(void);
void pce_reset(FESTALON_HES *hes);
void pce_shutdown(void);
void io_page_w(FESTALON_HES *hes, int address, int data);
int io_page_r(FESTALON_HES *hes, int address);
void bank_reset(FESTALON_HES *hes);
void bank_set(FESTALON_HES *hes, int bank, int value);
void input_w(FESTALON_HES *hes, uint8 data);
uint8 input_r(FESTALON_HES *hes);

#endif /* _PCE_H_ */
