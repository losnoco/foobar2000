#include <stdio.h>

int main(void)
{
	signed short cubic[514];

	for (int i = 0; i <= 256; i++)
	{
		cubic [i]       = -(  i*i*i >> 14) + (  i*i >> 5) - (i << 2);
		cubic [i + 257] =  (3*i*i*i >> 14) - (5*i*i >> 6)            + (1 << 11);
	}

	fputs("static short const cubic [514] =\n{", stdout);

	for (unsigned i = 0; i < 257; i++)
	{
		if (!(i & 15)) fputs("\n", stdout);
		fprintf(stdout, "%4i", cubic [i]);
		fputs(",", stdout);
	}

	fputs("\n", stdout);

	for (unsigned i = 0; i < 257; i++)
	{
		if (!(i & 15)) fputs("\n", stdout);
		fprintf(stdout, "%4i", cubic [i + 257]);
		if (i < 256) fputs(",", stdout);
	}

	fputs("\n};\n", stdout);

	return 0;
}