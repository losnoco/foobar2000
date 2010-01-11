/*
    Stripped-down VDC emulation, only enough to manage frame
    and line interrupts.
*/

#include "shared.h"
#include <string.h>

int vdc_init(void)
{
    return (0);
}

void vdc_reset(FESTALON_HES *hes)
{
	memset(hes->reg, 0, 0x20);
	hes->status = hes->latch = 0;
}

void vdc_shutdown(void)
{

}


int vdc_r(FESTALON_HES *hes, int offset)
{
    uint8 temp;

    switch(offset)
    {
        case 0x0000: /* Register latch / status flags */
            temp = hes->status;
            hes->status = (hes->status & STATUS_VD);
            h6280_set_irq_line(hes->h6280, 0, CLEAR_LINE);
            return (temp);

        case 0x0002: /* Data port (LSB) */
        case 0x0003: /* Data port (MSB) */
            break;
    }

    return (0xFF);
}

void vdc_w(FESTALON_HES *hes, int offset, int data)
{
    uint8 msb = (offset & 1);

    switch(offset)
    {
        case 0x0000: /* Register latch / status flags */
            hes->latch = (data & 0x1F);
            break;

        case 0x0002: /* Data port (LSB) */
        case 0x0003: /* Data port (MSB) */
            if(msb)
                hes->reg[hes->latch] = (hes->reg[hes->latch] & 0x00FF) | (data << 8);
            else
                hes->reg[hes->latch] = (hes->reg[hes->latch] & 0xFF00) | (data);
            break;
    }
}
