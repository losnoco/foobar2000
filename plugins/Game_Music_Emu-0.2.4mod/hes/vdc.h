
#ifndef _VDC_H_
#define _VDC_H_

/* Status register flags */
#define STATUS_VD       (0x20)  /* Vertical blanking */
#define STATUS_RR       (0x04)  /* Line interrupt */

/* Function prototypes */
int vdc_r(FESTALON_HES *, int offset);
void vdc_w(FESTALON_HES *, int offset, int data);
int vdc_init(void);
void vdc_reset(FESTALON_HES *);
void vdc_shutdown(void);

#endif /* _VDC_H_ */
