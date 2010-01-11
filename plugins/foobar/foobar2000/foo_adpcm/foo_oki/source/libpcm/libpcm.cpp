/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE libpcm LIBRARY SOURCE CODE.             *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'LICENSE'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE libpcm SOURCE CODE IS COPYRIGHT (c) 2002-2003, <?>           *
 *                                                                  *
 ********************************************************************/


#include "libpcm.h"
#include "mbstr.h"
#include "memman.h"
#include "util.h"

/*
#undef LIBPCM_DISABLE_XPCM_SUPPORT
#undef LIBPCM_DISABLE_WPD_SUPPORT
#undef LIBPCM_DISABLE_MPF_SUPPORT
#undef LIBPCM_DISABLE_WDT_SUPPORT
#undef LIBPCM_DISABLE_KWF_SUPPORT
#undef LIBPCM_DISABLE_NWA_SUPPORT
#undef LIBPCM_DISABLE_PX_SUPPORT
#undef LIBPCM_DISABLE_BW_SUPPORT
#undef LIBPCM_DISABLE_GUNGAGE_SUPPORT
#undef LIBPCM_DISABLE_DRACULAX_SUPPORT
#undef LIBPCM_DISABLE_WAF_SUPPORT
#undef LIBPCM_DISABLE_ADP_SUPPORT
*/
#define LIBPCM_DISABLE_WAV_PCM_SUPPORT

//#define LIBPCM_MAX_TITLE (256)
#define LIBPCM_BUF_SIZE (1 << 14)
#define LIBPCM_DIC_SIZE (1 << 16) /* const */
#define LIBPCM_DIC_MASK (LIBPCM_DIC_SIZE - 1) /* const */
#define LIBPCM_MAX_CH (6)
#define LIBPCM_MAX_COEFF (0x100)

#define LIBPCM_INVALID_SAMPLE_VALUE (0x66666666UL)

#if defined(__BORLANDC__)

/* BorlandC++のフリー版ではインラインアセンブラが使えない */
#define SAR(s,c) ((s)>>(c))
/* 固定少数点数積算(ライブラリに委託) */
#define mulfixc(m,n) libpcm_mulfix((m),(n),0xc)

#elif defined(__GNUC__) && defined(__i386__)

#define SAR(s,c) ((s)>>(c))
/* 固定少数点数積算(ライブラリに委託、インラインアセンブラは準備中) */
#define mulfixc(m,n) libpcm_mulfix((m),(n),0xc)

#elif defined(_M_IX86)

#define SAR(s,c) ((s)>>(c))
#if (_MSC_VER >= 1200)
/* 固定少数点数積算 要 __forceinline サポート(VC6等) */
#pragma warning(push)
#pragma warning(disable: 4035) /* no return value */
__forceinline static sint32_t __fastcall mulfixc(sint32_t m, sint32_t n)
{
	__asm
	{
		mov eax,m
		imul n
		shrd eax,edx,0xc
	}
}
#pragma warning(pop)
#else
/* 固定少数点数積算 要 VC5互換インラインアセンブラ(VC5,DigitalMarsC++,OpenWatcom等) */
static sint32_t mulfixc(sint32_t m, sint32_t n)
{
	sint32_t r;
	__asm
	{
		mov eax,m
		imul n
		shrd eax,edx,0xc
		mov r,eax
	}
	return r;
}
#endif
#else
/* x86のsar互換の演算(処理系非依存) */
#define SAR(x, y) (((x) >= 0) ? ((x) >> (y)) : (-((-(x) - 1) >> (y)) - 1))
/* 固定少数点数積算(ライブラリに委託) */
#define mulfixc(m,n) libpcm_mulfix((m),(n),0xc)
#endif

/* エンディアンを考慮しバイト列にアクセスする関数群 */
static uint_t readword(const unsigned char *p)
{
	uint_t ret;
	ret  = ((uint_t)(const unsigned char)p[0]) << (0 * 8);
	ret |= ((uint_t)(const unsigned char)p[1]) << (1 * 8);
	return ret;
}
static uint32_t readdword(const unsigned char *p)
{
	uint32_t ret;
	ret  = ((uint32_t)(const unsigned char)p[0]) << (0 * 8);
	ret |= ((uint32_t)(const unsigned char)p[1]) << (1 * 8);
	ret |= ((uint32_t)(const unsigned char)p[2]) << (2 * 8);
	ret |= ((uint32_t)(const unsigned char)p[3]) << (3 * 8);
	return ret;
}
static uint32_t readdword_be(const unsigned char *p)
{
	uint32_t ret;
	ret  = ((uint32_t)(const unsigned char)p[0]) << (3 * 8);
	ret |= ((uint32_t)(const unsigned char)p[1]) << (2 * 8);
	ret |= ((uint32_t)(const unsigned char)p[2]) << (1 * 8);
	ret |= ((uint32_t)(const unsigned char)p[3]) << (0 * 8);
	return ret;
}
static void writeword(unsigned char *p, uint_t v)
{
	p[0] = (unsigned char)(v >> (8 * 0));
	p[1] = (unsigned char)(v >> (8 * 1));
}
#ifdef LIBPCM_WORDS_BIGENDIAN
static void writeword_be(unsigned char *p, uint_t v)
{
	p[0] = (unsigned char)(v >> (8 * 1));
	p[1] = (unsigned char)(v >> (8 * 0));
}
#define writeword_sample writeword_be
static void fix_outputs_endian(unsigned char *b, uint32_t n, uint_t t)
{
	uint32_t i, j;
	unsigned char uc;
	uint_t us;
	us  = (t + 7) >> 3;
	if (us > 1)
	{
		for (i = 0; i < n; i++)
		{
			for (j = 0; j < (us >> 1); j++)
			{
				uc = b[j];
				b[j] = b[us - 1 - j];
				b[us - 1 - j] = uc;
			}
			b += us;
		}
	}
}
#else
#define writeword_sample writeword
#define fix_outputs_endian(b, n, t)
#endif

enum
{
	LIBPCM_CODEC_RAW,
	LIBPCM_CODEC_ULAW,
	LIBPCM_CODEC_OKIADPCM,
	LIBPCM_CODEC_LZVQ,
	LIBPCM_CODEC_ADPCM,
	LIBPCM_CODEC_BWADPCM,
	LIBPCM_CODEC_PXADPCM,
	LIBPCM_CODEC_RLDPCM,
	LIBPCM_CODEC_MSADPCM,
	LIBPCM_CODEC_DVIADPCM,
	LIBPCM_CODEC_XAADPCM,
	LIBPCM_CODEC_IMAADPCM,
	LIBPCM_CODEC_NWADPCM,
	LIBPCM_CODEC_UNKNOWN
};

struct iir_tag
{
	uint32_t ch;
	uint32_t nch;
	sint32_t buf[LIBPCM_MAX_CH];
};


struct LIBPCM_DECODER_tag
{
	uint_t (*read_funcp)(struct LIBPCM_DECODER_tag *d, void *buf, uint_t nsamples);
	void   (*seek_funcp)(struct LIBPCM_DECODER_tag *d, uint_t nsamples);

	libpcm_IReader *s;

	uint32_t srate;
	uint32_t datalen;
	uint32_t slen;
	uint32_t bitrate;
	uint32_t loop_len;
	uint32_t cur;
	uint32_t cur2;
	uint32_t readbuf_p;
	uint32_t readbuf_l;
	sint32_t adp_delta[LIBPCM_MAX_CH];
	uint_t ba;
	uint_t nch;
	uint_t bps;
	uint_t headlen;
	uint_t loop_enable;

	/*
	signed char adp_scale[LIBPCM_MAX_CH];
	unsigned char bit_buf, bit_pos;
	unsigned char codec;
	*/

	sint_t adp_scale[LIBPCM_MAX_CH];
	uint_t bit_buf, bit_pos;
	uint_t codec;

	union
	{
#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
		struct
		{
			const unsigned char *xscale_table;
			struct iir_tag iir;
			uint32_t lz_dic_p;
			uint32_t lz_copy_len, lz_copy_pos;
			uint32_t xfrm_buf_p;
			uint_t xfrm_end_flag;
			unsigned char lz_dic[LIBPCM_DIC_SIZE];
			unsigned char lz_buf[0x2000];
			unsigned char xfrmbuf[0x2000];
			sint32_t xdec_buf[0x1001 + 0x1001 + 0x2002 + 0x20];
		} xpcm;
#endif
#ifndef LIBPCM_DISABLE_WPD_SUPPORT
		struct
		{
			unsigned char *mem_p;
			uint32_t mem_i;
		} mb;
#endif
#ifndef LIBPCM_DISABLE_MPF_SUPPORT
		struct
		{
			sint32_t ulawtbl[0x100];
		} mpf;
#endif
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
		struct
		{
			unsigned char *frm_buf;
			uint32_t *frm_tbl;
			uint32_t depth;
			uint32_t frm_num;
			uint32_t frm_len;
			uint32_t frm_blen;
			uint32_t frm_cblen;
			uint32_t last_frm_len;
			uint32_t frm_bufp;
			uint32_t frm_cur;
			unsigned char nfrmbuf[LIBPCM_DIC_SIZE];
			uint32_t nfrmtbl[0x1001 + 0x1001 + 0x2002 + 0x20];
		} nwa;
#endif
#ifndef LIBPCM_DISABLE_BW_SUPPORT
		struct
		{
			uint32_t bw_scale[2];
			uint32_t bw_scale_init[2];
		} bw;
#endif
#ifndef LIBPCM_DISABLE_GUNGAGE_SUPPORT
		struct
		{
			uint32_t gg_buf_p;
			sint32_t lp_blk;
			sint32_t ed_blk;
			uint32_t sm_blk;
			unsigned char gg_buf[(1 << 15)];
		} gg;
#endif
#ifndef LIBPCM_DISABLE_DRACULAX_SUPPORT
		struct
		{
			unsigned char dx_fbuf[8];
			signed char lp_scale[4];
		} dx;
#endif
#ifndef LIBPCM_DISABLE_WAF_SUPPORT
		struct
		{
			uint32_t wfrmlen;
			uint32_t wfrmnum;
			uint32_t wfrmsam;
			uint32_t wcoeff_num;
			uint32_t wfrmptr;
			uint32_t wcurchn;
			sint32_t wfilter[2];
			sint32_t widelta[2];
			sint32_t wsample[4];
			sint32_t wcoeff1[LIBPCM_MAX_COEFF];
			sint32_t wcoeff2[LIBPCM_MAX_COEFF];
			unsigned char wendflg;
			unsigned char wcoeff_buf[LIBPCM_MAX_COEFF * 4];
		} waf;
#endif
#ifndef LIBPCM_DISABLE_ADP_SUPPORT
		struct
		{
			sint_t ab_scale[89];
		} adp;
#endif
		int none;
	} work;

	unsigned char readbuf[LIBPCM_BUF_SIZE];
	//char title[LIBPCM_MAX_TITLE];
};

#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
/*
0000 FOURCC 'X' 'P' 'C' 'M'
0004 DWORD  data size in byte
0008 BYTE   type of compression (0:raw 1:voice 2:adpcm)
0009 BYTE   parameter of compression
000A WORD   unknown(0)
000C WORD   unknown(WAVE_FORMAT_PCM?)
000E WORD   number of channels
0010 DWORD  sampling rate
0014 DWORD  data transfer rate (byte per sec)
0018 WORD   block align
001A WORD   sampling bit depth
*/
/* XPCM01のデコードに必要なテーブルを事前に演算する関数 */
static void calc_dtable_sincos(sint32_t *sincostbl)
{
	int i;
	static const unsigned char sincosbase[] =
	{
#include "sincos.h"
	};
	int delta = 6;
	sincostbl[1] = 0;
	for (i = 0; i < 0x400; i++)
	{
		delta += (sincosbase[(i >> 3) + 0x00] >> (i & 7)) & 1;
		delta -= (sincosbase[(i >> 3) + 0x80] >> (i & 7)) & 1;
		sincostbl[3 + i * 2] = sincostbl[1 + i * 2] + delta;
	}
	for (i = 0; i <= 0x400; i++)
	{
		/* cos */
		sincostbl[0x0800 - i * 2] = +sincostbl[1 + i * 2];
		sincostbl[0x0800 + i * 2] = -sincostbl[1 + i * 2];
		sincostbl[0x1800 - i * 2] = -sincostbl[1 + i * 2];
		sincostbl[0x1800 + i * 2] = +sincostbl[1 + i * 2];
		/* sin */
		sincostbl[0x1001 - i * 2] = +sincostbl[1 + i * 2];
		sincostbl[0x1001 + i * 2] = -sincostbl[1 + i * 2];
		sincostbl[0x2001 - i * 2] = -sincostbl[1 + i * 2];
	}
}
#endif

#ifndef LIBPCM_DISABLE_MPF_SUPPORT
/* MPFのデコードに必要なテーブルを事前に演算する関数 */
static void calc_dtable_mpf(sint32_t *mpftbl)
{
	int i;
	static const unsigned char mpfbase[] =
	{
#include "mpftable.h"
	};
	int value = 0;
	mpftbl[0x7f] = 0;
	mpftbl[0xff] = 0;
	for (i = 0; i < 0x7f; i++)
	{
		value += (mpfbase[i] + 1) << 2;
		mpftbl[0x7e - i] = -value;
		mpftbl[0xfe - i] = +value;
	}
}
#endif

/* 読み込みストリームのバッファリング関連関数 */
static void buffer_fill(LIBPCM_DECODER *d)
{
	d->readbuf_p = 0;
	d->readbuf_l = d->s->lpVtbl->Read(d->s, d->readbuf, LIBPCM_BUF_SIZE);
}
static int buffer_getc(LIBPCM_DECODER *d)
{
	if (d->readbuf_p >= d->readbuf_l)
	{
		if (d->readbuf_l != LIBPCM_BUF_SIZE) return -1;
		buffer_fill(d);
		if (d->readbuf_l == 0) return -1;
	}
	return d->readbuf[d->readbuf_p++];
}

#ifndef LIBPCM_DISABLE_NWA_SUPPORT
/* NWAを1フレームデコードするための関数群 */
static int nwa_frame_getbit(LIBPCM_DECODER *d)
{
	if (++d->bit_pos > 7)
	{
		int v;
		d->bit_pos = 0;
		v = buffer_getc(d);
		if (v < 0) return -1;
		d->bit_buf = (unsigned char)v;
	}
	return (d->bit_buf >> d->bit_pos) & 1;
}
static int nwa_frame_getbits(LIBPCM_DECODER *d, uint_t len)
{
	uint_t ret = 0, pos = 0;
	do
	{
		int v;
		v = nwa_frame_getbit(d);
		if (v < 0) return -1;
		ret |= v << pos;
	} while (++pos < len);
	return ret;
}
static int nwa_frame_fill(LIBPCM_DECODER *d)
{
	uint_t ch;
	uint32_t idx, idxmax;
	unsigned char *frmbufp;
	d->work.nwa.frm_cblen = 0;
	if (d->work.nwa.frm_cur > d->work.nwa.frm_num - 1) return 0;
	d->s->lpVtbl->Seek(d->s, d->work.nwa.frm_tbl[d->work.nwa.frm_cur++]);
	buffer_fill(d);
	frmbufp = d->work.nwa.frm_buf;
	for (ch = 0; ch < d->nch; ch++)
	{
		int v;
		if (d->bps == 16)
		{
			v  = buffer_getc(d);
			v += buffer_getc(d) << 8;
			v = (v ^ 0x8000) - 0x8000;
		}
		else
		{
			v  = buffer_getc(d);
			v = (v ^ 0x80) - 0x80;
		}
		d->adp_delta[ch] = v;
	}
	if (d->work.nwa.frm_cur == d->work.nwa.frm_num)
		idxmax = d->work.nwa.last_frm_len / d->nch;
	else
		idxmax = d->work.nwa.frm_len / d->nch;
	d->work.nwa.frm_cblen = idxmax * d->ba;
	d->bit_pos = 8;
	for (idx = 0; idx < idxmax; idx++)
	{
		for (ch = 0; ch < d->nch; ch++)
		{
			int scale = nwa_frame_getbits(d, 3) & 7;
			if (scale == 7)
			{
				if (nwa_frame_getbit(d))
				{
					d->adp_delta[ch] = 0;
				}
				else
				{
					int delta = nwa_frame_getbits(d, 7 - d->work.nwa.depth) << (2 + 7 + d->work.nwa.depth);
					if (nwa_frame_getbit(d))
						d->adp_delta[ch] -= delta;
					else
						d->adp_delta[ch] += delta;
				}
			}
			else if (scale)
			{
				int delta = nwa_frame_getbits(d, 4 - d->work.nwa.depth) << (2 + scale + d->work.nwa.depth);
				if (nwa_frame_getbit(d))
					d->adp_delta[ch] -= delta;
				else
					d->adp_delta[ch] += delta;
			}
			if (d->bps == 16)
			{
				writeword_sample(frmbufp, d->adp_delta[ch]);
				frmbufp += 2;
			}
			else
			{
				*frmbufp++ = (unsigned char)d->adp_delta[ch];
			}
		}
	}
	d->work.nwa.frm_bufp = 0;
	return 1;
}
static int nwa_frame_getc(LIBPCM_DECODER *d)
{
	if (d->work.nwa.frm_bufp >= d->work.nwa.frm_cblen)
	{
		if (!nwa_frame_fill(d)) return -1;
	}
	return d->work.nwa.frm_buf[d->work.nwa.frm_bufp++];
}
#endif

#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
/* XPCM01を1フレームデコードするための関数群 */
static void xpcm01_frame_decode_lz(LIBPCM_DECODER *d)
{
	sint32_t idx = 0;
	do
	{
		if (d->work.xpcm.lz_copy_len)
		{
			d->work.xpcm.lz_copy_len--;
			d->work.xpcm.lz_buf[idx] = d->work.xpcm.lz_dic[d->work.xpcm.lz_dic_p] = d->work.xpcm.lz_dic[d->work.xpcm.lz_copy_pos];
			d->work.xpcm.lz_copy_pos = (d->work.xpcm.lz_copy_pos + 1) & LIBPCM_DIC_MASK;
			d->work.xpcm.lz_dic_p = (d->work.xpcm.lz_dic_p + 1) & LIBPCM_DIC_MASK;
			idx++;
		}
		else
		{
			d->bit_pos <<= 1;
			if (d->bit_pos == 0)
			{
				int v = buffer_getc(d);
				if (v < 0) break;
				d->bit_buf = (unsigned char)v;
				d->bit_pos = 1;
			}
			if (d->bit_buf & d->bit_pos)
			{
				int v = buffer_getc(d);
				if (v < 0) break;
				d->work.xpcm.lz_buf[idx] = d->work.xpcm.lz_dic[d->work.xpcm.lz_dic_p] = (unsigned char)v;
				d->work.xpcm.lz_dic_p = (d->work.xpcm.lz_dic_p + 1) & LIBPCM_DIC_MASK;
				idx++;
			}
			else
			{
				int v = buffer_getc(d);
				if (v < 0) break;
				if ((v & 0xc0) == 0x80)
				{
					d->work.xpcm.lz_copy_len = (v & 0x20) ? 3 : 2;
					d->work.xpcm.lz_copy_pos = v & 0x1f;
					if (d->work.xpcm.lz_copy_pos == 0)
					{
						v = buffer_getc(d);
						if (v < 0) break;
						d->work.xpcm.lz_copy_pos = v;
					}
				}
				else if ((v & 0xc0) == 0xc0)
				{
					d->work.xpcm.lz_copy_len = ((v & 0x3c) >> 2) + 4;
					d->work.xpcm.lz_copy_pos = ((v & 0x03) << 8);
					v = buffer_getc(d);
					if (v < 0) break;
					d->work.xpcm.lz_copy_pos |= v & 255;
				}
				else
				{
					if (v == 0x7f)
					{
						v = buffer_getc(d);
						if (v < 0) break;
						d->work.xpcm.lz_copy_len = v;
						v = buffer_getc(d);
						if (v < 0) break;
						d->work.xpcm.lz_copy_len |= v << 8;
						d->work.xpcm.lz_copy_len += 2;
					}
					else
					{
						d->work.xpcm.lz_copy_len = (v & 0x7f) + 4;
					}
					v = buffer_getc(d);
					if (v < 0) break;
					d->work.xpcm.lz_copy_pos = v;
					v = buffer_getc(d);
					if (v < 0) break;
					d->work.xpcm.lz_copy_pos |= v << 8;
				}
				d->work.xpcm.lz_copy_pos = (d->work.xpcm.lz_dic_p - d->work.xpcm.lz_copy_pos) & LIBPCM_DIC_MASK;
			}
		}
	} while (idx < 0x2000);
	d->work.xpcm.xfrm_end_flag = (idx != 0x2000);
	while (idx < 0x2000) d->work.xpcm.lz_buf[idx++] = 0;
}
static void xpcm01_frame_decode_pass1(LIBPCM_DECODER *d)
{
	sint32_t idx;
	for (idx = 0; idx < 0x1000; idx++) d->work.xpcm.xfrmbuf[idx * 2 + 1] = d->work.xpcm.lz_buf[idx];
	for (idx = 0; idx < 0x800; idx++)
	{
		unsigned char h,l;
		h = d->work.xpcm.lz_buf[idx + 0x1000];
		l = d->work.xpcm.lz_buf[idx + 0x1800];
		d->work.xpcm.xfrmbuf[idx * 4 + 0] = (h & 0xf0) ^ (l >> 4);
		d->work.xpcm.xfrmbuf[idx * 4 + 2] = (h << 4) ^ (l & 0x0f);
	}
}
static void xpcm01_frame_decode_pass2(LIBPCM_DECODER *d)
{
	sint32_t idx = 0, v, scale;
	do
	{
		scale = 1 << d->work.xpcm.xscale_table[(idx >> 0x8) & 0x7];
		v = readword(d->work.xpcm.xfrmbuf + idx * 4 + 0);
		v = (v & 1) ? (-1 - (v >> 1)) : (v >> 1);
		d->work.xpcm.xdec_buf[idx + 0x0000] = v * scale;
		v = readword(d->work.xpcm.xfrmbuf + idx * 4 + 2);
		v = (v & 1) ? (-1 - (v >> 1)) : (v >> 1);
		d->work.xpcm.xdec_buf[idx + 0x1001] = v * scale;
	} while (++idx < 0x800);
	do
	{
		d->work.xpcm.xdec_buf[idx + 0x0000] = 0;
		d->work.xpcm.xdec_buf[idx + 0x1001] = 0;
	} while (++idx < 0x1000);
}
static void xpcm01_frame_decode_pass3(sint32_t *finbuf, sint32_t *decbuf, sint32_t *sincostbl)
{
	uint32_t lpc1,scale,step,stepo,steph;

	scale = 1;
	stepo = 0x1000;
	steph = 0x1000 >> 1;
	lpc1 = 12 - 2;

	do
	{
		uint32_t lpc2;
		uint32_t jj1,jj2,jj3,jj4;
		sint32_t wcos, wsin;
		wcos = sincostbl[scale * 2 + 0];
		wsin = sincostbl[scale * 2 + 1];

		step = stepo;
		stepo = steph;
		steph >>= 1;

		jj1 = 0;
		jj2 = stepo;
		jj3 = steph;
		jj4 = stepo + steph;

		lpc2 = 0;

		do
		{
			sint32_t ww1, ww2;
			ww1 = finbuf[jj1] - finbuf[jj2];
			ww2 = decbuf[jj1] - decbuf[jj2];
			finbuf[jj1] += finbuf[jj2];
			decbuf[jj1] += decbuf[jj2];
			finbuf[jj2] = ww1;
			decbuf[jj2] = ww2;
			ww1 = finbuf[jj1 + 1] - finbuf[jj2 + 1];
			ww2 = decbuf[jj1 + 1] - decbuf[jj2 + 1];
			finbuf[jj1 + 1] += finbuf[jj2 + 1];
			decbuf[jj1 + 1] += decbuf[jj2 + 1];
			finbuf[jj2 + 1] = + mulfixc(wcos, ww1) + mulfixc(wsin, ww2);
			decbuf[jj2 + 1] = + mulfixc(wcos, ww2) - mulfixc(wsin, ww1);

			ww1 = finbuf[jj3] - finbuf[jj4];
			ww2 = decbuf[jj3] - decbuf[jj4];
			finbuf[jj3] += finbuf[jj4];
			decbuf[jj3] += decbuf[jj4];
			finbuf[jj4] = + ww2;
			decbuf[jj4] = - ww1;
			ww1 = finbuf[jj3 + 1] - finbuf[jj4 + 1];
			ww2 = decbuf[jj3 + 1] - decbuf[jj4 + 1];
			finbuf[jj3 + 1] += finbuf[jj4 + 1];
			decbuf[jj3 + 1] += decbuf[jj4 + 1];
			finbuf[jj4 + 1] = + mulfixc(wcos, ww2) - mulfixc(wsin, ww1);
			decbuf[jj4 + 1] = - mulfixc(wcos, ww1) - mulfixc(wsin, ww2);

			jj1 += step;
			jj3 += step;
			jj2 += step;
			jj4 += step;
			lpc2 += step;
		} while (lpc2 < 0x1000);

		if (steph > 2)
		{
			uint32_t ii0 = 1;
			uint32_t lpc3 = steph - 2;
			sint32_t *sincosp = &sincostbl[scale * 4];

			do
			{
				uint32_t lpc4;
				uint32_t ii1,ii2,ii3,ii4;
				sint32_t vcos, vsin;

				vcos = sincosp[0];
				vsin = sincosp[1];
				sincosp += scale + scale;
				lpc4 = 0;
				ii1 = ++ii0;
				ii2 = ii0 + stepo;
				ii3 = ii0 + steph;
				ii4 = ii0 + stepo + steph;
				do
				{
					sint32_t vv1, vv2;
					vv1 = finbuf[ii1] - finbuf[ii2];
					vv2 = decbuf[ii1] - decbuf[ii2];
					finbuf[ii1] += finbuf[ii2];
					decbuf[ii1] += decbuf[ii2];
					finbuf[ii2] = + mulfixc(vcos, vv1) + mulfixc(vsin, vv2);
					decbuf[ii2] = + mulfixc(vcos, vv2) - mulfixc(vsin, vv1);

					vv1 = finbuf[ii3] - finbuf[ii4];
					vv2 = decbuf[ii3] - decbuf[ii4];
					finbuf[ii3] += finbuf[ii4];
					decbuf[ii3] += decbuf[ii4];
					finbuf[ii4] = + mulfixc(vcos, vv2) - mulfixc(vsin, vv1);
					decbuf[ii4] = - mulfixc(vcos, vv1) - mulfixc(vsin, vv2);

					ii1 += step;
					ii2 += step;
					ii3 += step;
					ii4 += step;
					lpc4 += step;
				} while (lpc4 < 0x1000);
			} while (--lpc3 != 0);
		}
		scale += scale;
	} while (--lpc1 != 0);
}
static void xpcm01_frame_decode_pass4(sint32_t *finbuf, sint32_t *decbuf)
{
	uint32_t idx = 0;
	do
	{
		sint32_t v1, v2;
		v1 = finbuf[idx + 0];
		v2 = finbuf[idx + 2];
		finbuf[idx + 0] = v1 + v2;
		finbuf[idx + 2] = v1 - v2;

		v1 = decbuf[idx + 0];
		v2 = decbuf[idx + 2];
		decbuf[idx + 0] = v1 + v2;
		decbuf[idx + 2] = v1 - v2;

		v1 = finbuf[idx + 3];
		v2 = finbuf[idx + 1];
		finbuf[idx + 1] = v1 + v2;
		finbuf[idx + 3] = v1 - v2;
		v1 = decbuf[idx + 1];
		v2 = decbuf[idx + 3];
		decbuf[idx + 1] = v1 + v2;
		finbuf[idx + 3] = v1 - v2;

		idx += 4;
	} while (idx < 0x1000);
}
static void xpcm01_frame_decode_pass5(sint32_t *finbuf, sint32_t *decbuf)
{
	uint32_t idx = 0;
	do
	{
		sint32_t v1, v2;
		v1 = finbuf[idx];
		v2 = finbuf[idx + 1];
		finbuf[idx] = v1 + v2;
		finbuf[idx + 1] = v1 - v2;
		v1 = decbuf[idx];
		v2 = decbuf[idx + 1];
		decbuf[idx] = v1 + v2;
		decbuf[idx + 1] = v1 - v2;
		idx += 2;
	} while (idx < 0x1000);
}
static void xpcm01_frame_decode_pass6(sint32_t *finbuf, sint32_t *decbuf)
{
	uint32_t idx1 = 0;
	uint32_t idx2 = 0;
	do
	{
		uint32_t vpow = 0x1000 >> 1;
		while (vpow <= idx2)
		{
			idx2 -= vpow;
			vpow >>= 1;
		}
		idx2 += vpow;
		if (idx1++ < idx2)
		{
			sint32_t swap1,swap2;
			swap1 = finbuf[idx1];
			swap2 = finbuf[idx2];
			finbuf[idx2] = swap1;
			finbuf[idx1] = swap2;
			swap1 = decbuf[idx1];
			swap2 = decbuf[idx2];
			decbuf[idx2] = swap1;
			decbuf[idx1] = swap2;
		}
	} while (idx1 < 0xfff);
}
static void xpcm01_frame_decode_pass7(sint32_t *finbuf, unsigned char *frm_buf, sint32_t *overlap, struct iir_tag *piir)
{
	sint32_t idx, v;
	for (idx = 0; idx < 0x1000; idx++)
	{
		v = finbuf[idx];
		v = SAR(v, 10);
		if (idx < 0x20)
		{
			sint32_t ov1, ov2;
			ov1 = overlap[idx] * (0x20 - idx);
			ov2 = v * idx;
			v = ov1 + ov2;
			v = SAR(v, 5);
		}
		if (v < -0x8000)
			v = -0x8000;
		else if (v > 0x7fff)
			v = 0x7fff;
		if (idx >= (0x1000 - 0x20))
		{
			overlap[idx - (0x1000 - 0x20)] = v;
		}
		else
		{
			/* IIR filter */
#define IIR_N 0x7400 /* 0x5d00 */
			v = (v - piir->buf[piir->ch]) * IIR_N;
			v = piir->buf[piir->ch] = piir->buf[piir->ch] + SAR(v, 16);
			if (++piir->ch == piir->nch) piir->ch = 0;
			writeword_sample(frm_buf + idx * 2, v);
		}
	}
}
static void xpcm01_frame_fill(LIBPCM_DECODER *d)
{
	xpcm01_frame_decode_lz(d);
	xpcm01_frame_decode_pass1(d);
	xpcm01_frame_decode_pass2(d);
	xpcm01_frame_decode_pass3(d->work.xpcm.xdec_buf, d->work.xpcm.xdec_buf + 0x1001, d->work.xpcm.xdec_buf + 0x2002);
	xpcm01_frame_decode_pass4(d->work.xpcm.xdec_buf, d->work.xpcm.xdec_buf + 0x1001);
	xpcm01_frame_decode_pass5(d->work.xpcm.xdec_buf, d->work.xpcm.xdec_buf + 0x1001);
	xpcm01_frame_decode_pass6(d->work.xpcm.xdec_buf, d->work.xpcm.xdec_buf + 0x1001);
	xpcm01_frame_decode_pass7(d->work.xpcm.xdec_buf, d->work.xpcm.xfrmbuf,  d->work.xpcm.xdec_buf + 0x4004, &d->work.xpcm.iir);
	d->work.xpcm.xfrm_buf_p = 0;
}
static int xpcm01_frame_getc(LIBPCM_DECODER *d)
{
	if (d->work.xpcm.xfrm_buf_p >= (0x1000 - 0x20) * 2)
	{
		if (d->work.xpcm.xfrm_end_flag) return -1;
		xpcm01_frame_fill(d);
	}
	return d->work.xpcm.xfrmbuf[d->work.xpcm.xfrm_buf_p++];
}
static void xpcm01_initoverlap(sint32_t *overlapp)
{
	int i;
	for (i = 0 ; i < 0x20; i++) overlapp[i] = 0;
}
/* XPCM01のデコードに用いるテーブル */
static const unsigned char xpcm01_scale_table[8 * 4] =
{
	11,11,12,12,13,13,14,14,
	11,11,11,12,12,12,13,14,
	11,11,11,11,11,12,12,13,
	10,10,10,11,11,11,11,11,
};
#endif

#ifndef LIBPCM_DISABLE_WAF_SUPPORT
static const uint16_t msadpcm_delta_table[] =
{
	230, 230, 230, 230, 307, 409, 512, 614,
	768, 614, 512, 409, 307, 230, 230, 230
};
static sint32_t msadpcm_getword(LIBPCM_DECODER *d)
{
	sint32_t v;
	v  = buffer_getc(d);
	v += buffer_getc(d) << 8;
	v = (v ^ 0x8000) - 0x8000;
	return v;
}
static void msadpcm_frame_init(LIBPCM_DECODER *d)
{
	sint32_t v;
	if (d->nch == 2)
	{
		d->work.waf.wfilter[0] = v = buffer_getc(d);
		d->work.waf.wfilter[1] = buffer_getc(d);
		d->work.waf.widelta[0] = msadpcm_getword(d);
		d->work.waf.widelta[1] = msadpcm_getword(d);
		d->work.waf.wsample[2] = msadpcm_getword(d);
		d->work.waf.wsample[3] = msadpcm_getword(d);
		d->work.waf.wsample[0] = msadpcm_getword(d);
		d->work.waf.wsample[1] = msadpcm_getword(d);
	}
	else
	{
		d->work.waf.wfilter[0] = v = buffer_getc(d);
		d->work.waf.widelta[0] = msadpcm_getword(d);
		d->work.waf.wsample[2] = msadpcm_getword(d);
		d->work.waf.wsample[0] = msadpcm_getword(d);
	}
	d->work.waf.wfrmptr = 0;
	d->work.waf.wcurchn = 0;
	d->bit_pos = 0;
	d->work.waf.wendflg = (v < 0);
}
static sint32_t get_sample_msadpcm(LIBPCM_DECODER *d)
{
	sint32_t v, vs;
	if (d->work.waf.wfrmptr < 2)
	{
		if (d->work.waf.wendflg) return LIBPCM_INVALID_SAMPLE_VALUE;
		vs = d->work.waf.wsample[d->work.waf.wcurchn + (d->work.waf.wfrmptr << 1)];
	}
	else
	{
		if ((d->bit_pos & 1) == 0)
		{
			v = buffer_getc(d);
			if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
			d->bit_buf = (v >> 0) & 0xf;
			v = (v >> 4) & 0xf;
		}
		else
		{
			v = d->bit_buf;
		}
		d->bit_pos ^= 1;
		vs  = d->work.waf.wsample[d->work.waf.wcurchn + 2] * d->work.waf.wcoeff1[d->work.waf.wfilter[d->work.waf.wcurchn]];
		vs += d->work.waf.wsample[d->work.waf.wcurchn + 0] * d->work.waf.wcoeff2[d->work.waf.wfilter[d->work.waf.wcurchn]];
		vs = SAR(vs, 8);
		vs += ((v ^ 8) - 8) * d->work.waf.widelta[d->work.waf.wcurchn];
		if (vs < -0x8000)
			vs = -0x8000;
		else if (vs > 0x7fff)
			vs = 0x7fff;
		d->work.waf.wsample[d->work.waf.wcurchn + 0] = d->work.waf.wsample[d->work.waf.wcurchn + 2];
		d->work.waf.wsample[d->work.waf.wcurchn + 2] = vs;
		d->work.waf.widelta[d->work.waf.wcurchn] = (d->work.waf.widelta[d->work.waf.wcurchn] * msadpcm_delta_table[v]) >> 8;
		if (d->work.waf.widelta[d->work.waf.wcurchn] < 16)
			d->work.waf.widelta[d->work.waf.wcurchn] = 16;
	}
	if (++d->work.waf.wcurchn == d->nch)
	{
		d->work.waf.wcurchn = 0;
		if (++d->work.waf.wfrmptr == d->work.waf.wfrmsam)
			msadpcm_frame_init(d);
	}
	return vs;
}
#endif

/* デコード前準備関数 */
static void decoder_init(LIBPCM_DECODER *d)
{
	int i;
	for (i = 0; i < sizeof(d->adp_scale) / sizeof(d->adp_scale[0]); i++)
	{
		d->adp_scale[i] = 0;
		d->adp_delta[i] = 0;
	}
	d->bit_pos = 0;
	d->cur = 0;
	d->cur2 = 0;

#ifndef LIBPCM_DISABLE_WPD_SUPPORT
	if (d->codec == LIBPCM_CODEC_RLDPCM)
	{
		d->work.mb.mem_i = 0;
		return;
	}
#endif
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
	if (d->codec == LIBPCM_CODEC_NWADPCM)
	{
		d->work.nwa.frm_cur = 0;
		nwa_frame_fill(d);
		return;
	}
#endif
	d->s->lpVtbl->Seek(d->s, d->headlen);
	if (d->codec == LIBPCM_CODEC_RAW) return;
#ifndef LIBPCM_DISABLE_GUNGAGE_SUPPORT
	if (d->codec == LIBPCM_CODEC_XAADPCM)
	{
		d->work.gg.gg_buf_p = 0;
		return;
	}
#endif
	buffer_fill(d);
#ifndef LIBPCM_DISABLE_WAF_SUPPORT
	if (d->codec == LIBPCM_CODEC_MSADPCM)
	{
		msadpcm_frame_init(d);
		return;
	}
#endif
#ifndef LIBPCM_DISABLE_BW_SUPPORT
	if (d->codec == LIBPCM_CODEC_BWADPCM)
	{
		d->work.bw.bw_scale[0] = d->work.bw.bw_scale_init[0];
		d->work.bw.bw_scale[1] = d->work.bw.bw_scale_init[1];
	}
#endif
#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
	if (d->codec == LIBPCM_CODEC_LZVQ)
	{
		int i;
		for (i = 0; i < sizeof(d->work.xpcm.iir.buf) / sizeof(d->work.xpcm.iir.buf[0]); i++)
		{
			d->work.xpcm.iir.buf[i] = 0;
		}
		d->work.xpcm.iir.ch = 0;
		d->work.xpcm.iir.nch = d->nch;
		d->work.xpcm.lz_copy_len = d->work.xpcm.lz_copy_pos = 0;
		d->work.xpcm.lz_dic_p = LIBPCM_DIC_SIZE;
		while (d->work.xpcm.lz_dic_p)
			d->work.xpcm.lz_dic[--d->work.xpcm.lz_dic_p] = 0;
		xpcm01_initoverlap(d->work.xpcm.xdec_buf + 0x4004);
		xpcm01_frame_fill(d);
	}
#endif
}

/* WPD02を事前にデコードする関数(別ファイルから取り込み) */
#ifndef LIBPCM_DISABLE_WPD_SUPPORT
#include "wpd02.h"
#endif

/* デコード シーク関数 */
static uint_t libpcm_read_raw(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret = 0, len, rs;
	while ((d->loop_enable && d->loop_len) && d->cur2 + nsamples >= d->slen)
	{
		len = d->slen - d->cur2;
		rs = d->s->lpVtbl->Read(d->s, buf, len * d->ba) / d->ba;
		fix_outputs_endian(buf, rs * d->nch, d->bps);
		nsamples -= rs;
		buf = rs * d->ba + (unsigned char *)buf;
		ret += rs;
		d->cur2 = d->slen - d->loop_len;
		d->s->lpVtbl->Seek(d->s, d->headlen + d->cur2 * d->ba);
		if (rs < len) return ret;
	}
	if (nsamples > 0)
	{
		rs = d->s->lpVtbl->Read(d->s, buf, nsamples * d->ba) / d->ba;
		fix_outputs_endian(buf, rs * d->nch, d->bps);
		ret += rs;
		d->cur2 += rs;
	}
	return ret;
}
static void libpcm_seek_raw(LIBPCM_DECODER *d, uint_t nsamples)
{
	if ((d->loop_enable && d->loop_len) && nsamples >= d->slen)
	{
		d->cur = nsamples;
		d->cur2 = (d->slen - d->loop_len) + ((nsamples - d->slen) % d->loop_len);
	}
	else
		d->cur = d->cur2 = nsamples;
	d->s->lpVtbl->Seek(d->s, d->headlen + d->cur2 * d->ba);
}
#ifndef LIBPCM_DISABLE_WPD_SUPPORT
static uint_t libpcm_read_memory(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret;
	uint_t b;
	unsigned char *p = ( unsigned char * ) buf;
	b = ret = 0;
	while (ret < nsamples)
	{
		if (d->work.mb.mem_i >= d->datalen) break;
		*p++ = d->work.mb.mem_p[d->work.mb.mem_i++];
		if (++b == d->ba)
		{
			b = 0;
			ret++;
		}
	}
	fix_outputs_endian(buf, ret * d->nch, d->bps);
	return ret;
}
static void libpcm_seek_memory(LIBPCM_DECODER *d, uint_t nsamples)
{
	d->work.mb.mem_i = nsamples * d->ba;
	d->cur = nsamples;
}
#endif
#ifndef LIBPCM_DISABLE_MPF_SUPPORT
static uint_t libpcm_read_mpf(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret;
	uint_t idx;
	uint_t nbytes = nsamples * (d->ba >> 1);
	unsigned char *p = ( unsigned char * ) buf;
	ret = d->s->lpVtbl->Read(d->s, p + nbytes, nbytes);
	for (idx = 0; idx < ret; idx++)
	{
		writeword_sample(p + idx * 2, d->work.mpf.ulawtbl[p[nbytes + idx]]);
	}
	ret /= (d->ba >> 1);
	return ret;
}
static void libpcm_seek_mpf(LIBPCM_DECODER *d, uint_t nsamples)
{
	d->s->lpVtbl->Seek(d->s, d->headlen + (nsamples * (d->ba >> 1)));
	d->cur = nsamples;
}
#endif
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
static uint_t libpcm_read_nwa(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret;
	uint_t b;
	unsigned char *p = ( unsigned char * ) buf;
	b = ret = 0;
	while (ret < nsamples)
	{
		int v = nwa_frame_getc(d);
		if (v < 0) break;
		*p++ = v;
		if (++b == d->ba)
		{
			b = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_nwa(LIBPCM_DECODER *d, uint_t nsamples)
{
	uint32_t cblk, nblk, npos;
	unsigned char buf[16];
	if (nsamples == 0)
		decoder_init(d);
	else
	{
		cblk = ((d->cur) * d->ba + 1) / d->work.nwa.frm_blen;
		nblk = ((nsamples - 1) * d->ba) / d->work.nwa.frm_blen;
		npos = ((nsamples - 1) * d->ba) - (nblk * d->work.nwa.frm_blen);
		d->work.nwa.frm_cur = nblk;
		nwa_frame_fill(d);
		d->work.nwa.frm_bufp = npos;
		d->cur = nsamples - 1;
		libpcm_read(d, buf, 1);
	}
}
#endif
#ifndef LIBPCM_DISABLE_PX_SUPPORT
static sint32_t get_sample_pxv(LIBPCM_DECODER *d, int ch)
{
	int v, delta;
	if (d->bit_pos == 0)
	{
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		delta = v;
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		delta <<= (v & 7) + 1;
		d->adp_scale[ch] = (v >> 4) & 0xf;
		if (v & 8)
		{
			d->adp_delta[ch] -= delta;
			if (d->adp_delta[ch] < -0x8000) d->adp_delta[ch] = -0x8000;
		}
		else
		{
			d->adp_delta[ch] += delta;
			if (d->adp_delta[ch] > +0x7fff) d->adp_delta[ch] = +0x7fff;
		}
	}
	if ((d->bit_pos & 1) == 0)
	{
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		d->bit_buf = v;
		v = (v >> 4) & 0xf;
	}
	else
	{
		v = (d->bit_buf >> 0) & 0xf;
	}
	if (v & 8)
	{
		d->adp_delta[ch] -= (8 - (v & 7)) << d->adp_scale[ch];
		if (d->adp_delta[ch] < -0x8000) d->adp_delta[ch] = -0x8000;
	}
	else
	{
		d->adp_delta[ch] += (0 + (v & 7)) << d->adp_scale[ch];
		if (d->adp_delta[ch] > +0x7fff) d->adp_delta[ch] = +0x7fff;
	}
	if (++d->bit_pos == 0x1c) d->bit_pos = 0;
	return d->adp_delta[ch];
}
static uint_t libpcm_read_pxv(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_pxv(d, ch);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_pxv(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_pxv(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
static uint_t libpcm_read_voice(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret;
	uint_t b;
	unsigned char *p = ( unsigned char * ) buf;
	b = ret = 0;
	while (ret < nsamples)
	{
		int v = xpcm01_frame_getc(d);
		if (v < 0) break;
		*p++ = v;
		if (++b == d->ba)
		{
			b = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_voice(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint32_t cblk, nblk, npos;
		unsigned char buf[16];
		cblk = ((d->cur) * d->ba + 1) / ((0x1000 - 0x20) * 2);
		nblk = ((nsamples - 1) * d->ba) / ((0x1000 - 0x20) * 2);
		npos = ((nsamples - 1) * d->ba) - (nblk * ((0x1000 - 0x20) * 2));
		if (nblk > cblk)
		{
			while (nblk - 1 > cblk)
			{
				xpcm01_frame_decode_lz(d);
				cblk++;
			}
			xpcm01_initoverlap(d->work.xpcm.xdec_buf + 0x4004);
			xpcm01_frame_fill(d);
		}
		d->work.xpcm.xfrm_buf_p = npos;
		d->cur = nsamples - 1;
		libpcm_read(d, buf, 1);
	}
}
static sint32_t get_sample_adpcm(LIBPCM_DECODER *d, int ch)
{
	int v;
	v = buffer_getc(d);
	if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
	v = (v ^ 0x80) - 0x80;
	d->adp_delta[ch] += v << d->adp_scale[ch];
	if (v == 0)
	{
		if (d->adp_scale[ch]) d->adp_scale[ch]--;
	}
	else if (v == 0x7f || v == -0x80)
	{
		if (d->adp_scale[ch] < 8) d->adp_scale[ch]++;
	}
	return d->adp_delta[ch];
}
static uint_t libpcm_read_adpcm(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_adpcm(d, ch);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_adpcm(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_adpcm(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
#ifndef LIBPCM_DISABLE_BW_SUPPORT
static const unsigned char bw_scale_table[8] =
{
	0x39, 0x39, 0x39, 0x39,
	0x4d, 0x66, 0x80, 0x99,
};
static sint32_t get_sample_bw(LIBPCM_DECODER *d, int ch)
{
	sint32_t delta;
	sint_t v;
	if ((d->bit_pos & 1) == 0)
	{
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		d->bit_buf = v;
		v = (v >> 0) & 0xf;
	}
	else
	{
		v = (d->bit_buf >> 4) & 0xf;
	}
	d->bit_pos ^= 1;
	delta = ((((v & 7) << 1) + 1) * d->work.bw.bw_scale[ch]) >> 3;
#if 0
	/* 2の補数表現に依存 */
	sign = (v & 8) ? ~0 : 0;
	delta = (delta ^ sign) + (sign & 1);
	d->adp_delta[ch] += delta;
#else
	d->adp_delta[ch] += (v & 8) ? -delta : delta;
#endif
	d->work.bw.bw_scale[ch] = (d->work.bw.bw_scale[ch] * bw_scale_table[v & 7]) >> 6;
	if (d->work.bw.bw_scale[ch] < 0x7f)
		d->work.bw.bw_scale[ch] = 0x7f;
	else if (d->work.bw.bw_scale[ch] > 0x6000)
		d->work.bw.bw_scale[ch] = 0x6000;
	delta = d->adp_delta[ch];
	if (delta < -0x8000)
		delta = -0x8000;
	else if (delta > 0x7fff)
		delta = 0x7fff;
	return delta;
}
static uint_t libpcm_read_bw(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_bw(d, ch);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_bw(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_bw(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
/* X68Sound.dllから流用 */
static const signed char oki_scale_tbl[8] =
{
	-1,-1,-1,-1,+2,+4,+6,+8,
};
static const sint16_t oki_delta_tbl[48+1] =
{
	  16,  17,  19,  21,  23,  25,  28,  31,  34,  37,  41,  45,  50,  55,  60,  66,
	  73,  80,  88,  97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963,1060,1166,1282,1411,
	1552,
};
static sint32_t get_sample_oki(LIBPCM_DECODER *d, int ch)
{
	sint_t delta;
	sint_t v;
	if ((d->bit_pos & 1) == 0)
	{
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		d->bit_buf = v;
		v = (v >> 0) & 0xf;
	}
	else
	{
		v = (d->bit_buf >> 4) & 0xf;
	}
	d->bit_pos ^= 1;
	delta = oki_delta_tbl[d->adp_scale[ch]];
	/* X68Sound.dllから流用 */
	/* これでCPUやコンパイラによっては分岐が消える？ */
	delta = (delta & ((v & 4) ? ~0 : 0)) + ((delta >> 1) & ((v & 2) ? ~0 : 0)) + ((delta >> 2)&((v & 1) ? ~0 : 0)) + (delta >> 3);
#if 0
	/* 2の補数表現に依存 */
	sign = (v & 8) ? ~0 : 0;
	delta = (delta ^ sign) + (sign & 1);
	d->adp_delta[ch] += delta;
#else
	d->adp_delta[ch] += (v & 8) ? -delta : delta;
#endif
	if (d->adp_delta[ch] > 2047)
		d->adp_delta[ch] = 2047;
	else if (d->adp_delta[ch] < -2047)
		d->adp_delta[ch] = -2047;
	d->adp_scale[ch] += oki_scale_tbl[v & 7];
	if (d->adp_scale[ch] > 48)
		d->adp_scale[ch] = 48;
	else if (d->adp_scale[ch] < 0)
		d->adp_scale[ch] = 0;
	return d->adp_delta[ch] << 4;
}
static uint_t libpcm_read_oki(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_oki(d, ch);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_oki(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_oki(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#ifndef LIBPCM_DISABLE_GUNGAGE_SUPPORT
static const struct
{
	uint16_t sum;
	sint16_t blk;
} looptable_gungage_8[] =
{
/*
	ループポイントは自力で地道に調べたので
	本来とは異なる可能性がある
*/
	{ 0x2d00, 0x1a },	/* boss1.8 */
	{ 0x69b0, 0x0a },	/* boss2.8 */
	{ 0xe317, 0x16 },	/* boss3.8 */
	{ 0x3951, 0x56 },	/* boss4.8 */
/*	{ 0x3951, 0x25 },*/	/* boss4.8 ww */
	{ 0xeebb, 0x1a },	/* boss5.8 */
	{ 0x182e, 0x10 },	/* boss6.8 */
	{ 0xc067, 0x58 },	/* boss7.8 */
	{ 0x9727, 0x32 },	/* chika.8 */
	{ 0x7077, 0x48 },	/* desert_1.8 */
	{ 0x935d, 0x0f },	/* desert_2.8 */
	{ 0xbe2f, 0x14 },	/* heigen.8 */
	{ 0xb6e3, 0x52 },	/* kenkyu.8 */
	{ 0xc605, 0xaa },	/* last_1.8 */
	{ 0xf924, 0x32 },	/* last_2.8 */
	{ 0xc552, 0x24 },	/* name.8 */
	{ 0xe9d8, 0x2a },	/* omu.8 */
	{ 0xd8a0, 0x06 },	/* saiseki.8 */
	{ 0xae20, 0x02 },	/* select.8 */
	{ 0x2911, 0x0a },	/* sinpi.8 */
	{ 0x3f89, 0x6e },	/* subway.8 */
	{ 0x0d1e, 0x2a },	/* town.8*/
	{ 0, -1 },			/* end */
};

static sint32_t checkloop_gungage_8(LIBPCM_DECODER *d)
{
	int i;
	uint16_t sum = 0;
	d->s->lpVtbl->Seek(d->s, 0);
	d->s->lpVtbl->Read(d->s, d->work.gg.gg_buf, (1 << 15));
	for (i = 0; i < (1 << 15); i++)
		sum = (sum << 1) ^ ((sum >> 15) & 1) ^ d->work.gg.gg_buf[i];
	for (i = 0; looptable_gungage_8[i].blk >= 0; i++)
	{
		if (sum == looptable_gungage_8[i].sum) break;
	}
	return looptable_gungage_8[i].blk;
}
static const sint_t gungage_iir_filter_k0[5] =
{
	0,60,115,98,122
};
static const sint_t gungage_iir_filter_k1[5] =
{
	0,0,-52,-55,-60
};
static int get_sample_gungage(LIBPCM_DECODER *d)
{
	uint_t a;
	sint_t vr, vl, filter;
	if (d->bit_pos == 0)
	{
		if (d->work.gg.gg_buf_p == 0)
		{
			if ((1 << 15) != d->s->lpVtbl->Read(d->s, d->work.gg.gg_buf, (1 << 15)))
			{
				if (d->work.gg.lp_blk < 0)
					return 1;
				d->s->lpVtbl->Seek(d->s, d->work.gg.lp_blk << 15);
				d->s->lpVtbl->Read(d->s, d->work.gg.gg_buf, (1 << 15));
			}
		}
		d->adp_scale[0] = (d->work.gg.gg_buf[(0 << 14) + d->work.gg.gg_buf_p] & 0xf);
		d->adp_scale[1] = (d->work.gg.gg_buf[(1 << 14) + d->work.gg.gg_buf_p] & 0xf);
		d->adp_scale[2] = (d->work.gg.gg_buf[(0 << 14) + d->work.gg.gg_buf_p] >> 4) & 0xf;
		d->adp_scale[3] = (d->work.gg.gg_buf[(1 << 14) + d->work.gg.gg_buf_p] >> 4) & 0xf;
	}
	a = d->work.gg.gg_buf_p + 2 + (d->bit_pos >> 1);
	if (d->bit_pos & 1)
	{
		vr = (d->work.gg.gg_buf[(0 << 14) + a] << 8) & 0xf000;
		vl = (d->work.gg.gg_buf[(1 << 14) + a] << 8) & 0xf000;
	}
	else
	{
		vr = (d->work.gg.gg_buf[(0 << 14) + a] << 12) & 0xf000;
		vl = (d->work.gg.gg_buf[(1 << 14) + a] << 12) & 0xf000;
	}
	if (++d->bit_pos == 14 * 2)
	{
		d->bit_pos = 0;
		d->work.gg.gg_buf_p = (d->work.gg.gg_buf_p + 16) & ((1 << 14) - 1);
	}
	vr = (vr ^ 0x8000) - 0x8000;
	vr = SAR(vr, d->adp_scale[0]);
	filter = d->adp_delta[0] * gungage_iir_filter_k0[d->adp_scale[2]] + d->adp_delta[2] * gungage_iir_filter_k1[d->adp_scale[2]];
	vr += SAR(filter, 6);
	vl = (vl ^ 0x8000) - 0x8000;
	vl = SAR(vl, d->adp_scale[1]);
	filter= d->adp_delta[1] * gungage_iir_filter_k0[d->adp_scale[3]] + d->adp_delta[3] * gungage_iir_filter_k1[d->adp_scale[3]];
	vl += SAR(filter, 6);
	d->adp_delta[2] = d->adp_delta[0];
	d->adp_delta[0] = vr;
	d->adp_delta[3] = d->adp_delta[1];
	d->adp_delta[1] = vl;
	return 0;
}
static uint_t libpcm_read_gungage(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		if (get_sample_gungage(d)) break;
		v = d->adp_delta[0];
		if (v < -0x8000)
			v = -0x8000;
		else if (v > 0x7fff)
			v = 0x7fff;
		writeword_sample(p, v);
		p += 2;
		v = d->adp_delta[1];
		if (v < -0x8000)
			v = -0x8000;
		else if (v > 0x7fff)
			v = 0x7fff;
		writeword_sample(p, v);
		p += 2;
		ret++;
	}
	return ret;
}
static void libpcm_seek_gungage(LIBPCM_DECODER *d, uint_t nsamples)
{
	sint32_t newfrm, newfrm2;
	newfrm = nsamples / d->work.gg.sm_blk;
	if ((d->loop_enable && d->loop_len) && d->work.gg.lp_blk >= 0 && newfrm >= d->work.gg.ed_blk)
		newfrm2 = d->work.gg.lp_blk + (newfrm - d->work.gg.ed_blk) % (d->work.gg.ed_blk - d->work.gg.lp_blk);
	else
		newfrm2 = newfrm;
	d->cur  = newfrm  * d->work.gg.sm_blk;
	d->cur2 = newfrm2 * d->work.gg.sm_blk;
	d->s->lpVtbl->Seek(d->s, newfrm2 << 15);
	d->bit_pos = 0;
	d->work.gg.gg_buf_p = 0;

	if (nsamples > d->cur)
	{
		uint_t ret = 0;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			if (get_sample_gungage(d)) break;
			ret++;
		}
		if (!(d->loop_enable && d->loop_len) && ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
#ifndef LIBPCM_DISABLE_DRACULAX_SUPPORT
/*
	オリジナルとは異なる独自のテーブル
	デルタ値演算結果の92個所に+-1の誤差有り
*/
static const uint_t dvi_dlt_tbl[89] =
{
	0x000c, 0x0010, 0x0012, 0x0014, 0x0015, 0x0018, 0x0019, 0x001b,
	0x0020, 0x0022, 0x0025, 0x0029, 0x002c, 0x0032, 0x0038, 0x003c,
	0x0044, 0x0049, 0x0052, 0x0059, 0x0064, 0x006c, 0x0078, 0x0084,
	0x0092, 0x00a0, 0x00b0, 0x00c2, 0x00d5, 0x00eb, 0x0104, 0x011c,
	0x0139, 0x0159, 0x017b, 0x01a2, 0x01cb, 0x01f9, 0x022c, 0x0265,
	0x02a2, 0x02e5, 0x0330, 0x0382, 0x03db, 0x0440, 0x04ab, 0x0524,
	0x05a8, 0x0638, 0x06d8, 0x0785, 0x0848, 0x091b, 0x0a04, 0x0b05,
	0x0c20, 0x0d55, 0x0eab, 0x1024, 0x11c0, 0x1385, 0x1579, 0x17a0,
	0x19fc, 0x1c98, 0x1f74, 0x2298, 0x260c, 0x29db, 0x2e0b, 0x32a8,
	0x37b8, 0x3d49, 0x436b, 0x4a29, 0x5194, 0x59bc, 0x62b5, 0x6c95,
	0x7772, 0x8364, 0x9088, 0x9efb, 0xaee2, 0xc05c, 0xd39b, 0xe8c4,
	0xfffc,
};
static sint32_t get_sample_draculax(LIBPCM_DECODER *d, int ch)
{
	sint_t idx;
	sint_t delta;
	sint_t v;
	if (d->bit_pos == 0)
	{
		for (idx = 0; idx < 8; idx++)
		{
			v = buffer_getc(d);
			if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
			d->work.dx.dx_fbuf[idx] = v;
		}
	}
	if ((d->bit_pos & 2) == 0)
	{
		v = (d->work.dx.dx_fbuf[(ch << 2) + (d->bit_pos >> 2)] >> 4) & 0xf;
	}
	else
	{
		v = (d->work.dx.dx_fbuf[(ch << 2) + (d->bit_pos >> 2)] >> 0) & 0xf;
	}
	if (++d->bit_pos == 4 * 2 * 2)
		d->bit_pos = 0;
#ifndef LIBPCM_ENABLE_DRACULAX_ORIGINALTABLE
	delta = (dvi_dlt_tbl[d->adp_scale[ch]] * (((v & 7) << 1) + 1)) >> 4;
#else
	delta = dvi_delta_tbl[(d->adp_scale[ch] << 3) + (v & 7)];
#endif
	d->adp_scale[ch] += oki_scale_tbl[v & 7];
	if (d->adp_scale[ch] > 88)
		d->adp_scale[ch] = 88;
	else if (d->adp_scale[ch] < 0)
		d->adp_scale[ch] = 0;
	d->adp_delta[ch] += (v & 8) ? -delta : delta;
	if (d->adp_delta[ch] > 0x7fff)
		d->adp_delta[ch] = 0x7fff;
	else if (d->adp_delta[ch] < -0x8000)
		d->adp_delta[ch] = -0x8000;
	return d->adp_delta[ch];
}

static uint_t libpcm_read_draculax(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0; //, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		if ( d->nch == 2 )
		{
			v = get_sample_draculax(d, 0);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			writeword_sample(p+2, v);
			v = get_sample_draculax(d, 1);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			writeword_sample(p, v);
			p += 4;
		}
		else
		{
			v = get_sample_draculax(d, 0);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			writeword_sample(p, v);
			p += 2;
		}
		//if ((ch += 2) == d->nch)
		{
			//ch = 0;
			ret++;
			if (++d->cur2 >= d->slen && (d->loop_enable && d->loop_len))
			{
				d->cur2 = d->slen - d->loop_len;
				d->s->lpVtbl->Seek(d->s, d->headlen + d->cur2);
				buffer_fill(d);
				d->adp_scale[0] = d->work.dx.lp_scale[0];
				d->adp_scale[1] = d->work.dx.lp_scale[1];
			}
		}
	}
	return ret;
}
static void libpcm_seek_draculax(LIBPCM_DECODER *d, uint_t nsamples)
{
	if ((d->loop_enable && d->loop_len) && nsamples >= d->slen)
	{
		uint32_t lptop = (d->slen - d->loop_len);
		uint32_t lpcnt = (nsamples - d->slen) / d->loop_len;
		uint32_t lppos = lptop + (nsamples - lpcnt * d->loop_len);
		if (lppos < d->cur2)
		{
			d->cur  = lptop + lpcnt * d->loop_len;
			d->cur2 = lptop - d->loop_len;
			d->s->lpVtbl->Seek(d->s, d->headlen + d->cur2);
			buffer_fill(d);
			d->adp_scale[0] = d->work.dx.lp_scale[0];
			d->adp_scale[1] = d->work.dx.lp_scale[1];
		}
		else
		{
			d->cur = d->cur2 + lpcnt * d->loop_len;
		}
	}
	else if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_draculax(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
				if (++d->cur2 >= d->slen && (d->loop_enable && d->loop_len))
				{
					d->cur2 = d->slen - d->loop_len;
					d->s->lpVtbl->Seek(d->s, d->headlen + d->cur2);
					buffer_fill(d);
					d->adp_scale[0] = d->work.dx.lp_scale[0];
					d->adp_scale[1] = d->work.dx.lp_scale[1];
				}
			}
		}
		if (!(d->loop_enable && d->loop_len) && ret + d->cur >= d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
#ifndef LIBPCM_DISABLE_WAF_SUPPORT
static uint_t libpcm_read_waf(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_msadpcm(d);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_waf(LIBPCM_DECODER *d, uint_t nsamples)
{
	uint32_t frm = nsamples / d->work.waf.wfrmsam;
	d->s->lpVtbl->Seek(d->s, d->headlen + d->work.waf.wfrmlen * frm);
	buffer_fill(d);
	msadpcm_frame_init(d);
	d->cur = d->work.waf.wfrmsam * frm;
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_msadpcm(d);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif
#ifndef LIBPCM_DISABLE_ADP_SUPPORT
static sint32_t get_sample_adp(LIBPCM_DECODER *d, int ch)
{
	sint32_t delta;
	sint_t v;
	if ((d->bit_pos & 1) == 0)
	{
		v = buffer_getc(d);
		if (v < 0) return LIBPCM_INVALID_SAMPLE_VALUE;
		d->bit_buf = v;
		v = v >> 4;
	}
	else
	{
		v = d->bit_buf;
	}
	d->bit_pos ^= 1;
	delta = (((v & 7) * 2 + 1) * d->work.adp.ab_scale[d->adp_scale[ch]]) >> 3;
	d->adp_delta[ch] += (v & 8) ? -delta : delta;
	if (d->adp_delta[ch] > 0x7fff)
		d->adp_delta[ch] = 0x7fff;
	else if (d->adp_delta[ch] < -0x8000)
		d->adp_delta[ch] = -0x8000;
	d->adp_scale[ch] += oki_scale_tbl[v & 7];
	if (d->adp_scale[ch] > 88)
		d->adp_scale[ch] = 88;
	else if (d->adp_scale[ch] < 0)
		d->adp_scale[ch] = 0;
	return d->adp_delta[ch];
}
static uint_t libpcm_read_adp(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	unsigned char *p = ( unsigned char * ) buf;
	uint_t ret = 0, ch = 0;
	sint32_t v;
	while (ret < nsamples)
	{
		v = get_sample_adp(d, ch);
		if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
		writeword_sample(p, v);
		p += 2;
		if (++ch == d->nch)
		{
			ch = 0;
			ret++;
		}
	}
	return ret;
}
static void libpcm_seek_adp(LIBPCM_DECODER *d, uint_t nsamples)
{
	if (nsamples < d->cur) decoder_init(d);
	if (nsamples > d->cur)
	{
		uint_t ret = 0, ch = 0;
		sint32_t v;
		nsamples -= d->cur;
		while (ret < nsamples)
		{
			v = get_sample_adp(d, ch);
			if (v == LIBPCM_INVALID_SAMPLE_VALUE) break;
			if (++ch == d->nch)
			{
				ch = 0;
				ret++;
			}
		}
		if (ret + d->cur > d->slen)
			ret = d->slen - d->cur;
		d->cur += ret;
	}
}
#endif

uint_t LIBPCM_API libpcm_read(LIBPCM_DECODER *d, void *buf, uint_t nsamples)
{
	uint_t ret = d->read_funcp(d, buf, nsamples);
	if (!(d->loop_enable && d->loop_len) && ret + d->cur >= d->slen)
		ret = d->slen - d->cur;
	d->cur += ret;
	return ret;
}
void LIBPCM_API libpcm_seek(LIBPCM_DECODER *d, uint32_t nsamples)
{
	d->seek_funcp(d, nsamples);
}

/* フォーマット判別関数 */
#ifndef LIBPCM_DISABLE_WAV_PCM_SUPPORT
static int is_wav_pcm(const unsigned char *header)
{
	/* 'RIFF' */
	if (readdword(header) != 0x46464952UL) return 0;
	/* 'WAVE' */
	if (readdword(header + 0x08) != 0x45564157UL) return 0;
	/* 'fmt ' */
	if (readdword(header + 0x0c) != 0x20746d66UL) return 0;
	/* size of fmt  chunk */
	if (readdword(header + 0x10) != 0x10) return 0;
	/* WAVE_FORMAT_PCM */
	if (readword(header + 0x14) != 1) return 0;
	/* 'data' */
	if (readdword(header + 0x24) != 0x61746164UL) return 0;
	return 1;
}
#endif
#ifndef LIBPCM_DISABLE_WDT_SUPPORT
static int is_studio_miris_wdt(const unsigned char *header)
{
	/* 'wdt0' */
	if (readdword(header) == 0x30746477UL) return 0x18;
	/* 'wdt1' */
	if (readdword(header) == 0x31746477UL) return 0x1c;
	return 0;
}
#endif
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
/* 子の判別関数はデコーダ構造体のメンバを操作している点を注意 */
static int is_visualarts_nwa(const unsigned char *header, LIBPCM_DECODER *d, uint32_t size)
{
	d->headlen = 0;
	d->nch = readword(header + 0x00);
	d->bps = readword(header + 0x02);
	d->srate = readdword(header + 0x04);
	d->work.nwa.depth = readdword(header + 0x08);
	d->work.nwa.frm_num = readdword(header + 0x10);
	d->datalen = readdword(header + 0x14);
	d->work.nwa.frm_len = readdword(header + 0x20);
	d->work.nwa.last_frm_len = readdword(header + 0x24);
	d->ba = d->nch * ((d->bps + 7) >> 3);
	d->work.nwa.frm_blen = d->work.nwa.frm_len * ((d->bps + 7) >> 3);

	if (d->nch != 1 && d->nch != 2) return 0;
	if (d->bps != 8 && d->bps != 16) return 0;
	if (d->srate == 0) return 0;
	if (!d->datalen) return 0;
	if (d->datalen != (readdword(header + 0x1c) * ((d->bps + 7) >> 3))) return 0;
	if (d->work.nwa.depth <= 2)
	{
		if (size != readdword(header + 0x18)) return 0;
		if (!d->work.nwa.frm_num) return 0;
		if (!d->work.nwa.frm_len) return 0;
		if ((d->work.nwa.frm_num - 1) * d->work.nwa.frm_len + d->work.nwa.last_frm_len != readdword(header + 0x1c)) return 0;
		return 1;
	}
	else if (d->work.nwa.depth == 0xffffffffUL)
	{
		d->headlen = 0x2c;
		if (readdword(header + 0x18)) return 0;
		if (d->work.nwa.frm_num) return 0;
		if (d->work.nwa.frm_len) return 0;
		if (d->work.nwa.last_frm_len) return 0;
		if (size != d->datalen + d->headlen) return 0;
		return 2;
	}
	else
		return 0;
}
#endif
#ifndef LIBPCM_DISABLE_GUNGAGE_SUPPORT
static int is_gungage_8(const unsigned char *header)
{
	int i;
	for (i = 0; i < 0x40; i += 0x10)
	{
		if ( header[i + 0x01] != 0x02) return 0;
		if ((header[i + 0x00] &  0x0f) > 0x0c) return 0;
		if ( header[i + 0x00] >= 0x50) return 0;
	}
	return 1;
}
#endif

/* ヘッダからフォーマットを判別し情報を取得する関数 */
LIBPCM_DECODER * LIBPCM_API libpcm_open_from_stream(libpcm_IReader *preader)
{
	LIBPCM_DECODER *d = 0;
	try
	{
	do
	{
		uint32_t size;
		unsigned char header[0x100];
		//const char *t, *t2, *te;
		d = ( LIBPCM_DECODER * ) libpcm_malloc(sizeof(LIBPCM_DECODER));
		if (!d) break;
		d->loop_len = 0;
		d->loop_enable = 0;
		d->codec = LIBPCM_CODEC_RAW;
		d->read_funcp = libpcm_read_raw;
		d->seek_funcp = libpcm_seek_raw;
		d->s = ( libpcm_IReader * ) preader->lpVtbl->AddRef(preader);

		/*t = d->s->lpVtbl->Path(d->s);
		if (t)
		{
			for (te = t; *te; te++);
			for (t2 = t; t2; t2 = libpcm_charnext(t2, te - t2))
			{
				if (*t2 == '\\' || *t2 == '/' || *t2 == ':') t = t2 + 1;
			}
			libpcm_strncpy(d->title, t, LIBPCM_MAX_TITLE);
		}
		else
			d->title[0] = 0;*/

		size = d->s->lpVtbl->Size(d->s);
		d->s->lpVtbl->Seek(d->s, 0);

		if (d->s->lpVtbl->Read(d->s, header, sizeof(header)) < 0x40) break;
#ifndef LIBPCM_DISABLE_XPCM_SUPPORT
		if (readdword(header) == 0x4d435058UL)	/* XPCM */
		{
			/* WAVE_FORMAT_PCM */
			if (readword(header + 0x0c) != 1) break;
			d->headlen = 0x1c;
			d->datalen = readdword(header + 0x04);
			d->nch = readword(header + 0x0e);
			d->ba = readword(header + 0x18);
			d->bps = readword(header + 0x1a);
			d->srate = readdword(header + 0x10);
			if (header[0x08] == 1)
			{
				if (d->bps != 16) break;
				d->headlen += 0x4;
				d->work.xpcm.xscale_table = xpcm01_scale_table + ((header[0x09] & 3) << 3);
				calc_dtable_sincos(d->work.xpcm.xdec_buf + 0x2002);
				d->codec = LIBPCM_CODEC_LZVQ;
				d->read_funcp = libpcm_read_voice;
				d->seek_funcp = libpcm_seek_voice;
			}
			else if (header[0x08] == 2)
			{
				if (d->bps != 16) break;
				if ((d->nch - 1) > 1) break;
				d->read_funcp = libpcm_read_adpcm;
				d->seek_funcp = libpcm_seek_adpcm;
				d->codec = LIBPCM_CODEC_ADPCM;
			}
			else if (header[0x08] != 0)
				break;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_WPD_SUPPORT
		if (size > 0x44 && readdword(header) == 0x57504420UL)	/* WPD */
		{
			unsigned char *cdata;
			if (readdword(header + 0x04) != 1) break;	/* version ? */
			if (readdword(header + 0x08) != 2) break;	/* compression method ? */
			d->headlen = 0x44;
			d->nch = readdword(header + 0x0c);
			d->bps = readdword(header + 0x10);
			d->srate = readdword(header + 0x14);
			d->datalen = readdword(header + 0x18);
			d->ba = readword(header + 0x2c);
			d->codec = LIBPCM_CODEC_RLDPCM;
			d->work.mb.mem_p = ( unsigned char * ) libpcm_malloc(d->datalen);
			if (!d->work.mb.mem_p) break;
			cdata = ( unsigned char * ) libpcm_malloc(size - d->headlen);
			if (!cdata) break;
			d->s->lpVtbl->Seek(d->s, d->headlen);
			if ((size - d->headlen) != d->s->lpVtbl->Read(d->s, cdata, (size - d->headlen)))
			{
				libpcm_mfree(cdata);
				break;
			}
			wpd_decode02(cdata, d->work.mb.mem_p, d->datalen);
			libpcm_mfree(cdata);
			d->s->lpVtbl->Release(d->s);
			d->s = 0;
			d->read_funcp = libpcm_read_memory;
			d->seek_funcp = libpcm_seek_memory;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_MPF_SUPPORT
		if (readdword(header) == 0x2046504dUL)	/* MPF */
		{
			d->headlen = 0x18;
			d->nch = readword(header + 0x06);
			d->srate = readdword(header + 0x08);
			d->ba = readword(header + 0x10);
			d->bps = readword(header + 0x12);
			d->datalen = readdword(header + 0x14) * 2;
			if (readword(header + 0x04) > 1)
			{
				d->bps = 16;
				d->ba = d->nch * ((d->bps + 7) >> 3);
				calc_dtable_mpf(d->work.mpf.ulawtbl);
				d->read_funcp = libpcm_read_mpf;
				d->seek_funcp = libpcm_seek_mpf;
				d->codec = LIBPCM_CODEC_ULAW;
			}
		}
		else
#endif
#ifndef LIBPCM_DISABLE_WDT_SUPPORT
		if ((d->headlen = is_studio_miris_wdt(header)) != 0) /* WDT */
		{
			uint_t headofs = (d->headlen != 0x18) ? 8 : 4;
			d->nch = readword(header + headofs + 0x06);
			d->srate = readdword(header + headofs + 0x08);
			d->ba = readword(header + headofs + 0x10);
			d->bps = readword(header + headofs + 0x12);
			d->datalen = readdword(header + headofs);
			d->loop_len = 0;
			if (d->headlen != 0x18)
			{
				uint32_t looptop = readdword(header + 4);
				d->datalen += looptop;
				d->loop_len = (d->datalen - looptop) / d->ba;
			}
		}
		else
#endif
#ifndef LIBPCM_DISABLE_KWF_SUPPORT
		if (readdword(header) == 0x3046574bUL)	/* KWF */
		{
			d->headlen = 0x40;
			d->nch = readword(header + 0x2a);
			d->srate = readdword(header + 0x2c);
			d->ba = readword(header + 0x34);
			d->bps = readword(header + 0x36);
			d->datalen = readdword(header + 0x0c);
		}
		else
#endif
#ifndef LIBPCM_DISABLE_PX_SUPPORT
		if (readdword(header) == 0x20585066UL && readdword(header + 8) == 0x4b525463UL)	/* PX */
		{
			d->headlen = 8 + readdword(header + 8 + 0x0c);
			d->datalen = readdword(header + 8 + 0x04) - (d->headlen - 8);
			d->srate = readdword(header + 8 + 0x14);
			if (d->srate == 0) d->srate = 44100;
			d->nch = readword(header + 8 + 0x1a);
			if (d->nch == 0) break;
			d->bps = readword(header + 8 + 0x1c);
			if (header[8 + 0x1e] == 2 && header[8 + 0x1f] == 0)
			{
				/* voice */
				d->bps = 16;
				d->ba = d->nch << 1;
				d->datalen = (d->datalen >> 4) * 0x1c * 2;
				d->read_funcp = libpcm_read_pxv;
				d->seek_funcp = libpcm_seek_pxv;
				d->codec = LIBPCM_CODEC_PXADPCM;
			} else if (header[8 + 0x1e] != 0 || d->bps != 16)
				break;
			else
			{
				unsigned char buf[0x18];
				d->ba = d->nch * ((d->bps + 7) >> 3);
				d->s->lpVtbl->Seek(d->s, d->headlen + d->datalen);
				if (0x18 == d->s->lpVtbl->Read(d->s, buf, 0x18) && readdword(buf) == 0x504c5363UL)
				{
					uint32_t looptop = readdword(buf + 0xc);
					d->loop_len = (d->datalen / d->ba) - looptop;
				}
			}
		}
		else
#endif
#ifndef LIBPCM_DISABLE_BW_SUPPORT
		if (readdword(header + 4) == 0x20207762UL)
		{
			d->headlen = readdword(header);
			d->datalen = readdword(header + 0x8) << 2;
			d->srate = readdword(header + 0x10);
			d->nch = readdword(header + 0x14);
			d->work.bw.bw_scale_init[0] = readdword(header + 0x24);
			d->work.bw.bw_scale_init[1] = readdword(header + 0x2c);
			d->bps = 16;
			d->ba = d->nch * ((d->bps + 7) >> 3);
			d->read_funcp = libpcm_read_bw;
			d->seek_funcp = libpcm_seek_bw;
			d->codec = LIBPCM_CODEC_BWADPCM;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_DRACULAX_SUPPORT
		if (readdword(header + 0) == 0x2E495644UL && size >= 0x800)
		{
			d->headlen = readdword_be(header + 4);
			d->datalen = readdword_be(header + 8) << 2;
			d->loop_len = readdword_be(header + 12);
			d->work.dx.lp_scale[0] = header[0x17];
			d->work.dx.lp_scale[1] = header[0x27];
			if (d->loop_len == 0xFFFFFFFFUL)
			{
				d->loop_len = 0;
			}
			else
			{
				d->loop_len = readdword_be(header + 8) - d->loop_len;
			}
			d->nch = 2;
			d->ba = 2 * 2;
			d->bps = 16;
			d->srate = 44100;
			d->read_funcp = libpcm_read_draculax;
			d->seek_funcp = libpcm_seek_draculax;
			d->codec = LIBPCM_CODEC_DVIADPCM;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_WAV_PCM_SUPPORT
		if (is_wav_pcm(header))
		{
			d->headlen = 0x2c;
			d->datalen = readdword(header + 0x28);
			d->nch = readword(header + 0x16);
			d->ba = readword(header + 0x20);
			d->bps = readword(header + 0x22);
			d->srate = readdword(header + 0x18);
		}
		else
#endif
#ifndef LIBPCM_DISABLE_WAF_SUPPORT
		if (readdword(header + 0) == 0x00464157UL)
		{
			uint_t idx;
			d->work.waf.wcoeff_num = readword(header + 0x16);
			if (d->work.waf.wcoeff_num > LIBPCM_MAX_COEFF) break;
			d->s->lpVtbl->Seek(d->s, 0x18);
			if ((d->work.waf.wcoeff_num << 2) != d->s->lpVtbl->Read(d->s, d->work.waf.wcoeff_buf, (d->work.waf.wcoeff_num << 2))) break;
			for (idx = 0; idx < d->work.waf.wcoeff_num; idx++)
			{
				d->work.waf.wcoeff1[idx] = (readword(d->work.waf.wcoeff_buf + 0 + (idx << 2)) ^ 0x8000) - 0x8000;
				d->work.waf.wcoeff2[idx] = (readword(d->work.waf.wcoeff_buf + 2 + (idx << 2)) ^ 0x8000) - 0x8000;
			}
			d->headlen = 0x18 + d->work.waf.wcoeff_num * 4 + 4;
			d->srate = readdword(header + 0x8);
			d->nch = readword(header + 0x6);
			d->ba = d->nch << 1;
			d->bps = 16;
			d->work.waf.wfrmsam = readword(header + 0x14);
			d->work.waf.wfrmlen = readword(header + 0x10);
			d->work.waf.wfrmnum = readdword(header + d->headlen - 4) / d->work.waf.wfrmlen;
			d->datalen = d->work.waf.wfrmnum * d->work.waf.wfrmsam * d->ba;

			d->read_funcp = libpcm_read_waf;
			d->seek_funcp = libpcm_seek_waf;
			d->codec = LIBPCM_CODEC_MSADPCM;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
		/* 条件が不明確なので最後に調べるべき */
		if (is_visualarts_nwa(header, d, size))
		{
			if (d->work.nwa.depth != 0xffffffffUL)
			{
				uint_t idx;
				unsigned char nwatblbuf[4];
				d->codec = LIBPCM_CODEC_NWADPCM;
				if (d->work.nwa.frm_num <= sizeof(d->work.nwa.nfrmtbl) / sizeof(d->work.nwa.nfrmtbl[0]))
					d->work.nwa.frm_tbl = d->work.nwa.nfrmtbl;
				else
					d->work.nwa.frm_tbl = ( uint32_t * ) libpcm_malloc(sizeof(sint32_t) * d->work.nwa.frm_num);
				if (d->work.nwa.frm_blen <= sizeof(d->work.nwa.nfrmbuf))
					d->work.nwa.frm_buf = d->work.nwa.nfrmbuf;
				else
					d->work.nwa.frm_buf = ( unsigned char * ) libpcm_malloc(d->work.nwa.frm_blen);
				if (!d->work.nwa.frm_tbl || !d->work.nwa.frm_buf) break;
				d->s->lpVtbl->Seek(d->s, 0x2c);
				for (idx = 0; idx < d->work.nwa.frm_num; idx++)
				{
					if (4 != d->s->lpVtbl->Read(d->s, nwatblbuf, 4)) break;
					d->work.nwa.frm_tbl[idx] = readdword(nwatblbuf);
					if (d->work.nwa.frm_tbl[idx] >= size) break;
				}
				if (idx < d->work.nwa.frm_num) break;
				d->read_funcp = libpcm_read_nwa;
				d->seek_funcp = libpcm_seek_nwa;
			}
		}
		else
#endif
#ifndef LIBPCM_DISABLE_GUNGAGE_SUPPORT
		/* 条件がかなり甘いのでかなり最後に調べるべき */
		if (is_gungage_8(header))
		{
			d->headlen = 0;
			d->datalen = (size >> 4) * 14 * 4;
			d->nch = 2;
			d->ba = 4;
			d->bps = 16;
			d->srate = 44100;
			d->work.gg.sm_blk = ((1 << 15) >> 4) * 14;;
			d->work.gg.ed_blk = size >> 15;
			d->work.gg.lp_blk = checkloop_gungage_8(d);
			/* 42 TOWN.8 */
			if (d->work.gg.lp_blk >= 0)
				d->loop_len = ((d->work.gg.ed_blk - d->work.gg.lp_blk) << (15 - 4)) * 14;
			d->read_funcp = libpcm_read_gungage;
			d->seek_funcp = libpcm_seek_gungage;
			d->codec = LIBPCM_CODEC_XAADPCM;
		}
		else
#endif
#ifndef LIBPCM_DISABLE_ADP_SUPPORT
		/* 条件がとてつもなく甘いのでとてつもなく最後に調べるべき */
		if (((readdword(header + 0) == 44100) || (readdword(header + 0) == 22050)) && ((readdword(header + 4) == 0x00010002UL) || (readdword(header + 4) == 0x00010001UL)))
		{
			uint_t idx;
			for (idx = 0; idx < 89; idx++)
				d->work.adp.ab_scale[idx] = readword(header + 8 + (idx << 1));
			d->headlen = readdword(header + 0xc0);
			d->srate = readdword(header);
			d->nch = readword(header + 4);
			d->bps = 16;
			d->ba = d->nch * ((d->bps + 7) >> 3);
			d->datalen = readdword(header + 0xbc) * d->ba;
			d->read_funcp = libpcm_read_adp;
			d->seek_funcp = libpcm_seek_adp;
			d->codec = LIBPCM_CODEC_IMAADPCM;
		}
		else
#endif
		{
			/* OKI-ADPCM */
			/* 判別する確実な方法はない */
			/* 先頭や末尾の無音部に00 08 80 88が多い */
			d->headlen = 0;
			d->datalen = size << 2;
			d->nch = 1;
			d->ba = 2;
			d->bps = 16;
			d->srate = 15625;
			d->read_funcp = libpcm_read_oki;
			d->seek_funcp = libpcm_seek_oki;
			d->codec = LIBPCM_CODEC_OKIADPCM;
		}
		d->slen = d->datalen / d->ba;
		d->bitrate = libpcm_muldiv(size - d->headlen, 8000, libpcm_get_length_ms(d));
		decoder_init(d);
		return d;
	} while (0);
	}
	catch (...)
	{
		libpcm_close(d);
		throw;
	}
	libpcm_close(d);
	return 0;
}

/* 不要になったメモリを開放する関数 */
void LIBPCM_API libpcm_close(LIBPCM_DECODER *d)
{
	if (d)
	{
#ifndef LIBPCM_DISABLE_NWA_SUPPORT
		if (d->codec == LIBPCM_CODEC_NWADPCM)
		{
			if (d->work.nwa.frm_buf && d->work.nwa.frm_buf != d->work.nwa.nfrmbuf) libpcm_mfree(d->work.nwa.frm_buf);
			if (d->work.nwa.frm_tbl && d->work.nwa.frm_tbl != d->work.nwa.nfrmtbl) libpcm_mfree(d->work.nwa.frm_tbl);
		}
#endif
#ifndef LIBPCM_DISABLE_WPD_SUPPORT
		if (d->codec == LIBPCM_CODEC_RLDPCM)
		{
			if (d->work.mb.mem_p) libpcm_mfree(d->work.mb.mem_p);
		}
#endif
		if (d->s) d->s->lpVtbl->Release(d->s);
		libpcm_mfree(d);
	}
}

/* 情報を取得する関数 */
/*const char * LIBPCM_API libpcm_get_title(LIBPCM_DECODER *d)
{
	return d->title;
}*/
uint32_t LIBPCM_API libpcm_get_length(LIBPCM_DECODER *d)
{
	return d->slen;
}
void LIBPCM_API libpcm_seek_ms(LIBPCM_DECODER *d, uint32_t ms)
{
	libpcm_seek(d, libpcm_muldiv(ms, d->srate, 1000));
}
uint32_t LIBPCM_API libpcm_get_length_ms(LIBPCM_DECODER *d)
{
	return libpcm_muldiv(d->slen, 1000, d->srate);
}
uint_t LIBPCM_API libpcm_get_samplerate(LIBPCM_DECODER *d)
{
	return d->srate;
}
uint_t LIBPCM_API libpcm_get_bitspersample(LIBPCM_DECODER *d)
{
	return d->bps;
}
uint_t LIBPCM_API libpcm_get_numberofchannels(LIBPCM_DECODER *d)
{
	return d->nch;
}
uint_t LIBPCM_API libpcm_get_blockalign(LIBPCM_DECODER *d)
{
	return d->ba;
}
uint32_t LIBPCM_API libpcm_get_currentposition(LIBPCM_DECODER *d)
{
	return d->cur;
}
uint32_t LIBPCM_API libpcm_get_currentposition_ms(LIBPCM_DECODER *d)
{
	return libpcm_muldiv(d->cur, 1000, d->srate);
}
uint32_t LIBPCM_API libpcm_get_bitrate(LIBPCM_DECODER *d)
{
	return d->bitrate;
}
void LIBPCM_API libpcm_switch_loop(LIBPCM_DECODER *d, int sw)
{
	d->loop_enable = sw;
}
uint32_t LIBPCM_API libpcm_get_looplength(LIBPCM_DECODER *d)
{
	return d->loop_len;
}
uint32_t LIBPCM_API libpcm_get_looplength_ms(LIBPCM_DECODER *d)
{
	return libpcm_muldiv(d->loop_len, 1000, d->srate);
}
uint_t LIBPCM_API libpcm_get_codec(LIBPCM_DECODER *d)
{
	return d->codec;
}
