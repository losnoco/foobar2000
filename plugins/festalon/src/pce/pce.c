
#include "shared.h"
#include <string.h>

/*--------------------------------------------------------------------------*/
/* Init, reset, shutdown functions                                          */
/*--------------------------------------------------------------------------*/

int pce_init(void)
{
    return (1);
}

void pce_reset(FESTALON_HES *hes)
{
    memset(&hes->ctrl, 0, sizeof(hes->ctrl));
    memset(hes->ram, 0, sizeof(hes->ram));
    bank_reset(hes);
    h6280_reset(hes->h6280, hes, hes->ram, hes->read_ptr, hes->write_ptr);
}

void pce_shutdown(void)
{
}

/*--------------------------------------------------------------------------*/
/* Hardware page handlers                                                   */
/*--------------------------------------------------------------------------*/

void io_page_w(FESTALON_HES *hes, int address, int data)
{
    switch(address & 0x1C00)
    {
        case 0x0000: /* VDC */
            if(address <= 0x0003)
            {
                vdc_w(hes, address, data);
                return;
            }
            break;

        case 0x0400: /* VCE */
            break;

        case 0x0800: /* PSG */
            if(address <= 0x0809)
            {
                psg_w(hes, address, data);
                return;
            }
            break;

        case 0x0C00: /* Timer */
            if(address == 0x0C00 || address == 0x0C01)
            {
                H6280_timer_w(hes->h6280, address & 1, data);
                return;
            }
            break;

        case 0x1000: /* I/O */
            if(address == 0x1000)
            {
                input_w(hes, data);
                return;
            }
            break;

        case 0x1400: /* IRQ control */
            if(address == 0x1402 || address == 0x1403)
            {
                H6280_irq_status_w(hes->h6280, address & 1, data);
                return;
            }
            break;

        case 0x1800: /* CD-ROM */
            return;

        case 0x1C00: /* Expansion */
            break;
    }
}


int io_page_r(FESTALON_HES *hes, int address)
{
    switch(address & 0x1C00)
    {
        case 0x0000: /* VDC */
            if(address <= 0x0003)
            {
                return (vdc_r(hes, address));
            }
            break;

        case 0x0400: /* VCE */
            return (0x00);

        case 0x0800: /* PSG */
            break;

        case 0x0C00: /* Timer */
            if(address == 0x0C00 || address == 0x0C01)
            {
                return (H6280_timer_r(hes->h6280, address & 1));
            }
            break;

        case 0x1000: /* I/O */
            if(address == 0x1000)
            {
                return (input_r(hes));
            }
            break;

        case 0x1400: /* IRQ control */
            if(address == 0x1402 || address == 0x1403)
            {
                return (H6280_irq_status_r(hes->h6280, address & 1));
            }
            break;

        case 0x1800: /* CD-ROM */
            break;

        case 0x1C00: /* Expansion */
	    return(hes->IBP[address&0x1FFF]);
            break;
    }
    return (UNMAPPED_IO);
}


/*--------------------------------------------------------------------------*/
/* Memory pointer management                                                */
/*--------------------------------------------------------------------------*/

/* Reset all pointers */
void bank_reset(FESTALON_HES *hes)
{
    int i;

    /* Return value for unmapped reads */
    memset(hes->dummy_read, UNMAPPED_MEM, sizeof(hes->dummy_read));

    /* Set all 256 banks to unused */
    for(i = 0; i < 0x100; i += 1)
    {
        hes->bank_map[i].read = hes->dummy_read;
        hes->bank_map[i].write = hes->dummy_write;
    }

    /* ROM */
    for(i = 0; i < 0x80; i += 1)
    {
        hes->bank_map[0x00 + i].read = &hes->rom[(i << 13)];
    }

    /* Work RAM (+ SGX) */
    for(i = 0; i < 4; i += 1)
    {
        hes->bank_map[0xF8+i].write = &hes->ram[i << 13];
        hes->bank_map[0xF8+i].read = &hes->ram[i << 13];
    }

    /* I/O area */
    hes->bank_map[0xFF].read = hes->IBP;
    hes->bank_map[0xFF].write = NULL;

    /* Set up logical mapping */
    for(i = 0; i < 8; i += 1)
    {
        hes->read_ptr[i] = hes->bank_map[0].read;     /* ROM bank #0 */
        hes->write_ptr[i] = hes->bank_map[0].write;   /* Ignored */
    }
}

/* Set a memory pointer to a certain bank */
void bank_set(FESTALON_HES *hes, int bank, int value)
{
    hes->read_ptr[bank] = hes->bank_map[value].read;
    hes->write_ptr[bank] = hes->bank_map[value].write;
}

/*--------------------------------------------------------------------------*/
/* Input routines                                                           */
/*--------------------------------------------------------------------------*/

void input_w(FESTALON_HES *hes, uint8 data)
{
    /* New state of SEL and CLR lines */
    int new_sel = (data >> 0) & 1;
    int new_clr = (data >> 1) & 1;

    /* Shift in new bits */
    hes->ctrl.sel = (hes->ctrl.sel << 1) | (new_sel);
    hes->ctrl.clr = (hes->ctrl.clr << 1) | (new_clr);
}

uint8 input_r(FESTALON_HES *hes)
{
    uint8 temp = 0xFF;

    if((hes->ctrl.sel & 1) == 0) temp >>= 4;
    temp &= 0x0F;
    return (temp);
}
