#include <zlib.h>


int main(void)
{
	unsigned char feh[256];
	
	memset(feh, 0, sizeof(feh));
	
	printf("Block of zeroes: 0x%X\n", crc32(0, &feh, sizeof(feh)));
	
	memset(feh, 255, sizeof(feh));
	
	printf("Block of 255: 0x%X\n", crc32(0, &feh, sizeof(feh)));
	
	return 0;
}

