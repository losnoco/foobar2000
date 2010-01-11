
/*
0000 FOURCC ' ' 'D' 'P' 'W' 0x57504420
0004 DWORD  version?(1)
0008 DWORD  compressing method(2)
000C DWORD  channel(1)
0010 DWORD  sampling bit
0014 DWORD  sampling rate
0018 DWORD  original size
001C DWORD  compressed size
0020 WORD   unknown(1)
0022 WORD   unknown(1)
0024 DWORD  sampling rate?
0028 DWORD  byte rate?
002c WORD   block align(2)
002e WORD   unknown(1)
0030 WORD   extra header size(0x12)
*/


typedef struct
{
	const unsigned char *data;
	uint32_t octbuf_b;
	uint32_t octbuf_i;
	const unsigned char *octbuf_p;
	uint32_t hexbuf_b;
	uint32_t hexbuf_i;
	const unsigned char *hexbuf_p;
	uint32_t bytebuf_i;
	const unsigned char *bytebuf_p;
	uint32_t wordbuf_i;
	const unsigned char *wordbuf_p;
} WPD_DECODER;

static void wpd_decode02_get_init(WPD_DECODER *d)
{
	d->wordbuf_i = 0;
	d->bytebuf_i = 0;
	d->hexbuf_b = 0;
	d->hexbuf_i = 0;
	d->octbuf_b = 0;
	d->octbuf_i = 0;
	d->octbuf_p = d->data + readdword(d->data + 0x00);
	d->hexbuf_p = d->data + readdword(d->data + 0x04);
	d->bytebuf_p = d->data + readdword(d->data + 0x08);
	d->wordbuf_p = d->data + readdword(d->data + 0x0c);
}

static uint32_t wpd_decode02_get_oct(WPD_DECODER *d)
{
	uint32_t ret = 0, bit = 0;
	do
	{
		if ((1 << d->octbuf_b) & d->octbuf_p[d->octbuf_i])
		{
			ret |= 1 << bit;
		}
		d->octbuf_b = (d->octbuf_b + 1) & 7;
		if (d->octbuf_b == 0) d->octbuf_i++;
	} while (++bit < 3);
	return ret;
}
static uint32_t wpd_decode02_get_hex(WPD_DECODER *d)
{
	uint32_t ret;
	ret = d->hexbuf_p[d->hexbuf_i] >> d->hexbuf_b;
	d->hexbuf_b ^= 4;
	if (d->hexbuf_b == 0) d->hexbuf_i++;
	return ret & 0xf;
}
static uint32_t wpd_decode02_get_byte(WPD_DECODER *d)
{
	return d->bytebuf_p[d->bytebuf_i++];
}
static uint32_t wpd_decode02_get_twe(WPD_DECODER *d)
{
	return (wpd_decode02_get_hex(d) << 8) + wpd_decode02_get_byte(d);
}
static uint32_t wpd_decode02_get_word(WPD_DECODER *d)
{
	uint32_t ret;
	ret  = d->wordbuf_p[d->wordbuf_i++];
	ret += d->wordbuf_p[d->wordbuf_i++] << 8;
	return ret;
}

static void wpd_decode02(const unsigned char *s, unsigned char *d, uint32_t l)
{
	WPD_DECODER work;
	sint32_t prev = 0;
	unsigned char *e = d + l;
	work.data = s;
	wpd_decode02_get_init(&work);
	while (d < e)
	{
		uint32_t mode;
		mode = wpd_decode02_get_oct(&work);
		if (mode == 7)
		{
			uint32_t cnt = wpd_decode02_get_byte(&work);
			if (cnt > 1)
			{
				do
				{
					*d++ = (unsigned char)(prev >> 0);
					*d++ = (unsigned char)(prev >> 8);
				} while (--cnt > 1);
			}
		}
		else if (mode == 0)
			prev = wpd_decode02_get_word(&work);
		else if (mode == 1)
			prev += wpd_decode02_get_hex(&work);
		else if (mode == 2)
			prev -= wpd_decode02_get_hex(&work);
		else if (mode == 3)
			prev += wpd_decode02_get_byte(&work);
		else if (mode == 4)
			prev -= wpd_decode02_get_byte(&work);
		else if (mode == 5)
			prev += wpd_decode02_get_twe(&work);
		else/* if (mode == 6)*/
			prev -= wpd_decode02_get_twe(&work);
		*d++ = (unsigned char)(prev >> 0);
		*d++ = (unsigned char)(prev >> 8);
	}
}
