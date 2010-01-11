/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * kemuopl.h - Emulated OPL using Ken Silverman's emulator, by Simon Peter
 *             <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_KEMUOPL
#define H_ADPLUG_KEMUOPL

#include "opl.h"
#include <stdlib.h>
extern "C" {
#include "adlibemu.h"
}

class CKemuopl: public Copl
{
public:
	CKemuopl(int rate, bool bit16, bool usestereo)
		: use16bit(bit16), stereo(usestereo)
	{
		opl[0] = adlibinit(rate, usestereo ? 2 : 1, bit16 ? 2 : 1);
		opl[1] = adlibinit(rate, usestereo ? 2 : 1, bit16 ? 2 : 1);
		currType = TYPE_OPL2;
		mixbuf = 0;
		mixbufSize = 0;
	};

	~CKemuopl()
	{
		adlibfree(opl[0]);
		adlibfree(opl[1]);
		if (mixbuf) free(mixbuf);
	}

	void update(short *buf, int samples)
	{
		if(use16bit) samples *= 2;
		if(stereo) samples *= 2;

		switch (currType)
		{
		case TYPE_OPL2:
			adlibgetsample(opl[0], buf, samples);
			break;

		case TYPE_DUAL_OPL2:
			if (samples > mixbufSize)
			{
				mixbufSize = samples;
				mixbuf = realloc( mixbuf, mixbufSize );
			}
			adlibgetsample(opl[0], buf, samples);
			adlibgetsample(opl[1], mixbuf, samples);
			if (use16bit)
			{
				short * temp_s = (short *) mixbuf;
				for (unsigned i = 0, j = samples / 2; i < j; i++)
				{
					int sample = buf [i] + temp_s [i];
					if ( ( short ) sample != sample )
						sample = 0x7FFF ^ ( sample >> 31 );
					buf [i] = ( short ) sample;
				}
			}
			else
			{
				unsigned char * buf_s = (unsigned char *) buf;
				unsigned char * temp_s = (unsigned char *) mixbuf;
				for (unsigned i = 0; i < samples; i++)
				{
					unsigned sample = ( ( buf_s [i] ^ 0x80 ) + ( temp_s [i] ^ 0x80 ) ) ^ 0x80;
					if ( sample < 0 )
						sample = 0;
					else if ( sample > 255 )
						sample = 255;
					buf_s [i] = sample;
				}
			}
		}
	}

	// template methods
	void write(int reg, int val)
	{
		adlib0(opl[currChip], reg, val);
	};

	void init() {};

	void settype(ChipType type)
	{
		currType = type;
	}

private:
	bool   use16bit,stereo;
    void  *mixbuf;
	int    mixbufSize;
	void  *opl[2];
};

#endif
