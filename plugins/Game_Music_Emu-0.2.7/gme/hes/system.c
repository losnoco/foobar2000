/*
    Copyright (C) 2000, 2001  Charles MacDonald

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "shared.h"
#include <string.h>

int system_init(FESTALON_HES *hes)
{
    //pce_init();
    //vdc_init();
    psg_init(hes);
    return (1);
}

/* We want the timer to be reloaded and fire continually, not as one-shot */
#define TIMER_HACK

uint32 FESTAHES_Emulate(FESTALON_HES *hes)
{
    int line;

    hes->status &= ~STATUS_VD; 

    for(line = 0; line < 262; line += 1)
    {
        if((line + 64) == (hes->reg[6] & 0x3FF))
        {
            if(hes->reg[5] & 0x04)
            {
                hes->status |= STATUS_RR;
                h6280_set_irq_line(hes->h6280, 0, ASSERT_LINE);
            }
        }

        /* VBlank */
        if(line == 240)
        {
            /* Cause VBlank interrupt if necessary */
            hes->status |= STATUS_VD; 
            if(hes->reg[5] & 0x0008)
            {
                h6280_set_irq_line(hes->h6280, 0, ASSERT_LINE);
            }
        }

		if ( h6280_get_pc( hes->h6280 ) == 0x1C03 ) // idle detection
		{
			h6280_Regs * h6280 = hes->h6280;
			int32 cycles = 455 + h6280->extra_cycles;
			h6280->extra_cycles = cycles & 3;
			if ( h6280->timer_status )
			{
				int32 timestamp = h6280->timestamp + ( cycles & ~3 );
				while ( h6280->timestamp < timestamp )
				{
					int32 todo = timestamp - h6280->timestamp;
					if ( todo > h6280->timer_value ) todo = h6280->timer_value;
					h6280->timer_value -= todo;
					h6280->timestamp += todo;
					if ( h6280->timer_value <= 0 && h6280->timer_ack == 1 )
					{
						#ifndef TIMER_HACK
							h6280->timer_ack = h6280->timer_status = 0;
						#else
							h6280->timer_ack = 0;
						#endif
						h6280_set_irq_line( h6280, 2, ASSERT_LINE );
						h6280_execute( h6280, timestamp - h6280->timestamp );
						break;
					}
				}
			}
			else h6280->timestamp += cycles & ~3; // JMP ABS == 4 cycles
		}
        else h6280_execute(hes->h6280, 455);
    }

    return psg_flush(hes);
}


void system_reset(FESTALON_HES *hes)
{
    pce_reset(hes);
    vdc_reset(hes);
    psg_reset(hes);
}


void system_shutdown(FESTALON_HES *hes)
{
    //pce_shutdown();
    //vdc_shutdown();
    psg_shutdown(hes);
}


