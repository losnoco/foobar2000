#include <string.h>

#include "racloops.h"

typedef struct racloop
{
	int offset;
	int length;
	char * filename;
} racloop;

static const racloop rac_loops[] = {

	{ 0xAB40, 170810, "adamb01"},
	{ 0xAE80, 172020, "adamb03"},
	{ 0xCBC0, 217822, "adamb04"},
	{ 0xA1C0, 208980, "adamb08"},
	{ 0xAB40, 170138, "adamb11"},
	{0x1B1EE, 624252, "zoobirds"},
	{ 0x548E, 263672, "zootree"},
	{ 0x5B9C, 181330, "zoolab"},
	{ 0x8B82, 313712, "zoodesrt"},
	{ 0x5C08, 307042, "zooswamp"},
	{ 0x419C, 290100, "zoosulfr"},
	{ 0xAD80, 210924, "powersta"},
	{ 0x3160,  41186, "lbbigfar"},
	{ 0x5A00,  74714, "probroom"},
	{ 0x4EBC,  48060, "lbbignr"},
	{ 0x4E9C,  55036, "lbctrfar"},
	{ 0x4896,  82368, "lbctnear"},
	{ 0xA7C0, 697176, "s5amb01"},
	{ 0xEB80, 485212, "s5amb03"},
	{ 0xB778, 532032, "s5amb04"},
	{ 0x9A00, 160960, "s5amb05"},
	{ 0xB0C0, 109710, "s5amb06"},
	{ 0xB980, 593396, "s5amb07"},
	{0x13388, 617410, "s5amb08"},
	{ 0x4C00, 454266, "udamb01"},
	{ 0x6C32, 465786, "udamb02"},
	{ 0x7A40, 469232, "udamb06"},
	{ 0x62C0, 463066, "udamb09"},
	{ 0x5C80, 160676, "udamb12"},
	{0x162B0, 528616, "h3amb01"},
	{0x1B140, 838494, "astro"},
	{0x19740, 834924, "compute"},
	{      0,  32582, "bridgamb"},

	{     -1,     -1, 0}
};

int find_loop_start(const char * name, int len)
{
	const racloop * loop = rac_loops;

	while (loop->offset >= 0 && loop->length >= 0 && loop->filename) {
		if (loop->length == len && !stricmp(loop->filename, name)) return loop->offset;
		loop++;
	}

	return -1;
}
