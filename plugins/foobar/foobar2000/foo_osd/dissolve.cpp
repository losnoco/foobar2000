/*

I'd slap the BSD license on this file as well, but I can't because I originally converted it from a
public domain "fire" demo, which was written in Pascal. This is mostly the same code I contributed
to the ZSNES project in the form of the "burning" GUI effect.

*/

#include <stdlib.h>

#include "dissolve.h"

#define randint(a) (rand() % (a))

typedef struct tag_rgba
{
	union
	{
		struct
		{
			unsigned char r, g, b, a;
		};
		unsigned int v;
	};
} rgba, *prgba;

dissolve::dissolve(void * in, unsigned w, unsigned h, unsigned d)
{
	ptr = in;
	width = w;
	height = h;
	decay = d;
}

bool dissolve::draw()
{
	prgba pt = (prgba) ptr;
	rgba v;
	unsigned x, y, tx, a, ta;
	bool clear = true;
	
    for( x = 0; x < width; x++)
	{
		for( y = height - 1; y-- != (unsigned)-1;)
		{
			if (y != (unsigned)-1) v = pt[y*width + x];
			else v.v = 0;
			if (!v.a)
				pt[(y+1)*width + x].v = 0;
			else
			{
				tx = x - (randint(3) - 1);
				if (tx < width)
				{
					a = v.a;
					ta = a - randint(decay);
					if (ta > 255) ta = 0;
					// voila! since the RGB values are prescaled according to the alpha level,
					// some precision is lost... if you actually notice it during the ~1.5s
					// it takes for the effect to erase the image
					v.r = (unsigned char)((int)v.r * ta / a);
					v.g = (unsigned char)((int)v.g * ta / a);
					v.b = (unsigned char)((int)v.b * ta / a);
					v.a = ta;
					pt[((y+1)*width) + tx] = v;
					if (ta) clear = false;
				}
				else
					clear = false;
			}
		}
	}
	return clear;
}
