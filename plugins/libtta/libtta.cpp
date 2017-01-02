/*
 * libtta.cpp
 *
 * Description: TTA1 library functions
 * Copyright (c) 2010 Aleksander Djuric. All rights reserved.
 * Based on Buffer aware TTA encoder/decoder, developed by
 * Christophe Paris (christophe.paris <at> free.fr)
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#include "libtta.h"

using namespace tta;

//////////////////////// constants and definitions //////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define FRAME_TIME 1.04489795918367346939
#define DEC(x) ((x & 1)?((x + 1) >> 1):(-x >> 1))
#define ENC(x) ((x > 0)?((x << 1) - 1):(-x << 1))
#define PREDICTOR1(x, k) ((x * ((1 << k) - 1)) >> k)

#ifdef __GNUC__
#ifdef CARIBBEAN
#define tta_memclear(__dest,__length) RMMemset(__dest,0,__length)
#define tta_malloc RMMalloc
#else // GNUC
#define tta_memclear(__dest,__length) memset(__dest,0,__length)
#define tta_malloc malloc
#endif
#else // MSVC
#define tta_memclear(__dest,__length) ZeroMemory(__dest,__length)
#define tta_malloc malloc
#endif

#if 0 // optimized, requires additional 3 bytes at the tail of a buffers
#define READ_BUFFER(x, p, d, s) { \
	x = *(TTAint32 *)p << s; \
	p += d; }
#define WRITE_BUFFER(x, p, d) { \
	*(TTAint32 *)p = *x; \
	p += d; }
#else // not optimized, but accurate
#define READ_BUFFER(x, p, d, s) { \
	if (d == 2) x = *(TTAint16 *)p << s; \
	else if (d == 1) x = *p << s; \
	else x = *(TTAint32 *)p << s; \
	p += d; }

#define WRITE_BUFFER(x, p, d) { \
	if (d == 2) *(TTAint16 *)p = 0xffff & *x; \
	else if (d == 1) *p = 0xff & *x; \
	else *(TTAint32 *)p = *x; \
	p += d; }
#endif

const TTAuint32 bit_mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
}; // bit_mask

const TTAuint32 bit_shift[] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000,
	0x80000000, 0x80000000, 0x80000000, 0x80000000
}; // bit_shift

const TTAuint32 *shift_16 = bit_shift + 4;

const TTAuint32 crc32_table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
	0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
	0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
}; // crc32_table

/////////////////////////// TTA common functions ////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static inline void rice_init (TTA_adapt *rice, TTAuint32 k0, TTAuint32 k1) {
	rice->k0 = k0;
	rice->k1 = k1;
	rice->sum0 = shift_16[k0];
	rice->sum1 = shift_16[k1];
} // rice_init

/////////////////////////// TTA hybrid filter ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////

////////// Filter Settings //////////
const TTAint32 flt_set[3] = {10, 9, 10};

static inline void memshl(register TTAint32 *pA) {
	register TTAint32 *pB = pA + 16;

	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA++ = *pB++;
	*pA   = *pB;
} // memshl

static inline void hybrid_filter(TTA_fltst *fs, TTAint32 *in, TTAint32 mode) {
	register TTAint32 *pA = fs->dl + fs->index;
	register TTAint32 *pB = fs->qm;
	register TTAint32 *pM = fs->dx + fs->index;
	register TTAint32 sum = fs->round;

	if (!fs->error) {
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA++ * *pB++;
		sum += *pA   * *pB;
		pM += 8;
	} else if (fs->error < 0) {
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA++ * (*pB++ -= *pM++);
		sum += *pA   * (*pB   -= *pM++);
	} else {
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA++ * (*pB++ += *pM++);
		sum += *pA   * (*pB   += *pM++);
	}

	pB = pA++;
	if (mode) {
		*pA = *in;
		*in -= (sum >> fs->shift);
		fs->error = *in;
	} else {
		fs->error = *in;
		*in += (sum >> fs->shift);
		*pA = *in;
	}

	*pM-- = ((*pB-- >> 30) | 1) << 2;
	*pM-- = ((*pB-- >> 30) | 1) << 1;
	*pM-- = ((*pB-- >> 30) | 1) << 1;
	*pM   = ((*pB   >> 30) | 1);

	*(pA-1) = *(pA-0) - *(pA-1);
	*(pA-2) = *(pA-1) - *(pA-2);
	*(pA-3) = *(pA-2) - *(pA-3);

	if (++fs->index == 16) {
		memshl(fs->dl);
		memshl(fs->dx);
		fs->index = 0;
	}
} // hybrid_filter

static inline void filter_init(TTA_fltst *fs, TTAint32 shift) {
	tta_memclear (fs, sizeof(TTA_fltst));
	fs->shift = shift;
	fs->round = 1 << (shift - 1);
	fs->index = 0;
} // filter_init

//////////////////////////// decoder functions //////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void reader_start(TTA_fifo *s) { s->pos = &s->end; }

static void reader_reset(TTA_fifo *s) {
	// init crc32, reset counter
	s->crc = 0xffffffffUL;
	s->count = 0;
} // reader_reset

static inline TTAuint8 read_byte(TTA_fifo *s) {
	if (s->pos == &s->end) {
		if (!s->io->read(s->io, s->buffer, TTA_FIFO_BUFFER_SIZE))
			throw tta_exception(TTA_READ_ERROR);
		s->pos = s->buffer;
	}

	// update crc32 and statistics
	s->crc = crc32_table[(s->crc ^ *s->pos) & 0xff] ^ (s->crc >> 8);
	s->count++;

	return *s->pos++;
} // read_byte

static inline TTAuint32 read_uint16(TTA_fifo *s) {
	TTAuint32 value = 0;

	value |= read_byte(s);
	value |= read_byte(s) << 8;

	return value;
} // read_uint16

static inline TTAuint32 read_uint32(TTA_fifo *s) {
	TTAuint32 value = 0;

	value |= read_byte(s);
	value |= read_byte(s) << 8;
	value |= read_byte(s) << 16;
	value |= read_byte(s) << 24;

	return value;
} // read_uint32

static inline bool read_crc32(TTA_fifo *s) {
	TTAuint32 crc = s->crc ^ 0xffffffffUL;
	return (crc != read_uint32(s));
} // read_crc32

static void reader_skip_bytes(TTA_fifo *s, TTAuint32 size) {
	while (size--) read_byte(s);
} // reader_skip_bytes

static TTAuint32 skip_id3v2(TTA_fifo *s) {
	TTAuint32 size = 0;

	reader_reset(s);

	// id3v2 header must be at start
	if ('I' != read_byte(s) ||
		'D' != read_byte(s) ||
		'3' != read_byte(s)) {
			s->pos = s->buffer;
			return 0;
	}

	s->pos += 2; // skip version bytes
	if (read_byte(s) & 0x10) size += 10;

	size += (read_byte(s) & 0x7f);
	size = (size << 7) | (read_byte(s) & 0x7f);
	size = (size << 7) | (read_byte(s) & 0x7f);
	size = (size << 7) | (read_byte(s) & 0x7f);

	reader_skip_bytes(s, size);

	return (size + 10);
} // skip_id3v2

static TTAuint32 read_tta_header(TTA_fifo *s, TTA_info *info) {
	TTAuint32 size = skip_id3v2(s);

	reader_reset(s);

	if ('T' != read_byte(s) ||
		'T' != read_byte(s) ||
		'A' != read_byte(s) ||
		'1' != read_byte(s)) throw tta_exception(TTA_FORMAT_ERROR); 

	info->format = read_uint16(s);
	info->nch = read_uint16(s);
	info->bps = read_uint16(s);
	info->sps = read_uint32(s);
	info->samples = read_uint32(s);

	if (read_crc32(s))
		throw tta_exception(TTA_FILE_ERROR);

	size += 22;  // sizeof TTA1 header

	return size;
} // read_tta_header

bool tta_decoder::read_seek_table() {
	if (!seek_table) return false;

	reader_reset(&fifo);

	TTAuint64 tmp = info.offset + (info.frames + 1) * 4;
	for (TTAuint32 i = 0; i < info.frames; i++) {
		seek_table[i] = tmp;
		tmp += read_uint32(&fifo);
	} 

	if (read_crc32(&fifo)) return false;

	return true;
} // read_seek_table

void tta_decoder::frame_init(TTAuint32 frame, bool seek_needed) {
	if (frame >= info.frames) return;

	fnum = frame;

	if (seek_needed && seek_allowed) {
		TTAuint64 offset = seek_table[fnum];
		if (offset && fifo.io->seek(fifo.io, offset) < 0 &&
			errno != 0) throw tta_exception(TTA_SEEK_ERROR);
		reader_start(&fifo);
	}

	if (fnum == info.frames - 1)
		info.flen = info.flen_last;
	else info.flen = info.flen_std;

	// init entropy decoder
	for (TTAuint32 i = 0; i < info.nch; i++) {
		filter_init(&codec[i].fst, flt_set[info.depth - 1]);
		rice_init(&codec[i].rice, 10, 10);
		codec[i].prev = 0;
	}

	fpos = 0;
	bcount = 0;
	bcache = 0;

	reader_reset(&fifo);
} // frame_init

void tta_decoder::frame_init(TTAuint32 frame, TTA_io_callback *iocb) {
	fifo.io = iocb;
	reader_start(&fifo);
	frame_init(frame, false);
} // frame_init

double tta_decoder::set_position(double seconds) {
	TTAuint32 frame = (TTAuint32)(seconds / FRAME_TIME);

	if (seek_allowed && frame < info.frames)
		frame_init(frame, true);

	return (frame * FRAME_TIME);
} // set_position

void tta_decoder::decoder_init(TTAuint32 nch, TTAuint32 bps,
	TTAuint32 sps, TTAuint32 length) {

	// check for supported formats
	if (bps < MIN_BPS || bps > MAX_BPS || nch > MAX_NCH)
		throw tta_exception(TTA_FORMAT_ERROR);

	info.nch = nch;
	info.bps = bps;
	info.sps = sps;
	info.samples = length;
	info.format = 1; // pcm

	info.depth = (info.bps + 7) / 8;
	info.flen_std = get_frame_length(info.sps);
	info.flen_last = info.samples % info.flen_std;
	info.frames = info.samples / info.flen_std + (info.flen_last ? 1 : 0);
	if (!info.flen_last) info.flen_last = info.flen_std;
	info.rate = 0;

	codec_last = codec + info.nch - 1;

	reader_start(&fifo);
	frame_init(0, false);
}

void tta_decoder::decoder_init() {
	reader_start(&fifo);

	info.offset = read_tta_header(&fifo, &info);

	// check for supported formats
	if (info.format != 1 ||
		info.bps < MIN_BPS ||
		info.bps > MAX_BPS ||
		info.nch > MAX_NCH) throw tta_exception(TTA_FORMAT_ERROR);

	info.depth = (info.bps + 7) / 8;
	info.flen_std = get_frame_length(info.sps);
	info.flen_last = info.samples % info.flen_std;
	info.frames = info.samples / info.flen_std + (info.flen_last ? 1 : 0);
	if (!info.flen_last) info.flen_last = info.flen_std;
	info.rate = 0;

	// allocate memory for seek table data
	seek_table = (TTAuint64 *) tta_malloc(info.frames * sizeof(TTAuint64));
	if (seek_table == NULL) throw tta_exception(TTA_MEMORY_ERROR);

	seek_allowed = read_seek_table();

	codec_last = codec + info.nch - 1;

	frame_init(0, false);
} // decoder_init

TTAuint32 tta_decoder::get_frame_length(TTAuint32 sps) {
	return (TTAuint32) (FRAME_TIME * sps);
} // get_frame_length

TTAint32 tta_decoder::decode_value(TTA_codec *dec) {
	TTA_fltst *fst = &dec->fst;
	TTA_adapt *rice = &dec->rice;
	TTAuint32 k, level;
	TTAint32 value = 0;

	// decode Rice unsigned
	while (!(bcache ^ bit_mask[bcount])) {
		value += bcount;
		bcache = read_byte(&fifo);
		bcount = 8;
	}

	while (bcache & 1) {
		value++;
		bcache >>= 1;
		bcount--;
	}
	bcache >>= 1;
	bcount--;

	switch (value) {
	case 0: level = 0; k = rice->k0; break;
	default:
			level = 1; k = rice->k1;
			value--;
	}

	if (k) {
		while (bcount < k) {
			TTAuint32 tmp = read_byte(&fifo);
			bcache |= tmp << bcount;
			bcount += 8;
		}
		value = (value << k) + (bcache & bit_mask[k]);
		bcache >>= k;
		bcount -= k;
		bcache &= bit_mask[bcount];
	}

	switch (level) {
	case 1: 
		rice->sum1 += value - (rice->sum1 >> 4);
		if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
			rice->k1--;
		else if (rice->sum1 > shift_16[rice->k1 + 1])
			rice->k1++;
		value += bit_shift[rice->k0];
	default:
		rice->sum0 += value - (rice->sum0 >> 4);
		if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
			rice->k0--;
		else if (rice->sum0 > shift_16[rice->k0 + 1])
		rice->k0++;
	}

	value = DEC(value);

	// decompress stage 1: adaptive hybrid filter
	hybrid_filter(fst, &value, 0);

	// decompress stage 2: fixed order 1 prediction
	value += PREDICTOR1(dec->prev, 5);
	dec->prev = value;

	return value;
} // decode_value

TTAuint32 tta_decoder::decode_stream(TTAuint8 *output, TTAuint32 count,
	TTA_CALLBACK tta_callback) {
	TTA_codec *dec = codec;
	TTAuint32 buf_size = (count - 1) * info.depth * info.nch;
	TTAint32 cache[MAX_NCH];
	TTAint32 *cp = cache;
	TTAuint8 *ptr = output;
	TTAuint32 ret = 0;

	while (fpos < info.flen
		&& ptr < output + buf_size) {
		TTAint32 value = decode_value(dec);

		if (dec < codec_last) {
			*cp++ = value;
			dec++;
		} else {
			*cp = value;

			if (info.nch == 1) {
				WRITE_BUFFER(cp, ptr, info.depth);
			} else {
				TTAint32 *end = cp;
				TTAint32 *smp = cp - 1;

				*cp += *smp / 2;
				while (smp > cache) {
					*smp = *cp-- - *smp;
					smp--;
				}
				*smp = *cp - *smp;

				while (smp <= end) {
					WRITE_BUFFER(smp, ptr, info.depth);
					smp++;
				}
			}

			cp = cache;
			fpos++;
			ret++;
			dec = codec;
		}

		if (fpos == info.flen) {
			// check frame crc
			bool crc_flag = read_crc32(&fifo);
			if (crc_flag) {
				tta_memclear(output, buf_size);
				if (!seek_allowed) break;
			}

			fnum++;

			// update dynamic info
			info.rate = (fifo.count << 3) / 1070;
			if (tta_callback)
				tta_callback(info.rate, fnum, info.frames);

			if (fnum == info.frames) break;
			frame_init(fnum, crc_flag);
		}
	}

	return ret;
} // decode_stream

TTAuint32 tta_decoder::decode_frame(TTAuint32 in_bytes, TTAuint8 *output,
	TTAuint32 out_bytes, TTA_CALLBACK tta_callback) {
	TTA_codec *dec = codec;
	TTAint32 cache[MAX_NCH];
	TTAint32 *cp = cache;
	TTAuint8 *ptr = output;
	TTAuint32 ret = 0;

	while (fifo.count < in_bytes
		&& ptr < output + out_bytes) {
		TTAint32 value = decode_value(dec);

		if (dec < codec_last) {
			*cp++ = value;
			dec++;
		} else {
			*cp = value;

			if (info.nch == 1) {
				WRITE_BUFFER(cp, ptr, info.depth);
			} else {
				TTAint32 *end = cp;
				TTAint32 *smp = cp - 1;

				*cp += *smp / 2;
				while (smp > cache) {
					*smp = *cp-- - *smp;
					smp--;
				}
				*smp = *cp - *smp;

				while (smp <= end) {
					WRITE_BUFFER(smp, ptr, info.depth);
					smp++;
				}
			}

			cp = cache;
			fpos++;
			ret++;
			dec = codec;
		}

		if (fpos == info.flen ||
			fifo.count == in_bytes - 4) {
			// check frame crc
			if (read_crc32(&fifo))
				tta_memclear(output, out_bytes);

			// update dynamic info
			info.rate = (fifo.count << 3) / 1070;
			if (tta_callback)
				tta_callback(info.rate, fnum, info.frames);

			break;
		}
	}

	return ret;
} // decode_frame

tta_decoder::tta_decoder(TTA_io_callback *iocb) {
	seek_table = NULL;
	seek_allowed = false;
	fifo.io = iocb;
} // tta_decoder

tta_decoder::~tta_decoder() {
	if (seek_table) delete [] seek_table;
} // ~tta_decoder

///////////////////////////// encoder functions /////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static void writer_start(TTA_fifo *s) { s->pos = s->buffer; }

static void writer_reset(TTA_fifo *s) {
	// init crc32, reset counter
	s->crc = 0xffffffffUL;
	s->count = 0;
} // writer_reset

static void writer_done(TTA_fifo *s) {
	TTAint32 buffer_size = s->pos - s->buffer;

	if (buffer_size) {
		if (s->io->write(s->io, s->buffer, buffer_size) != buffer_size)
			throw tta_exception(TTA_WRITE_ERROR);
		s->pos = s->buffer;
	}
} // writer_done

static inline void write_byte(TTA_fifo *s, TTAuint32 value) {
	if (s->pos == &s->end) {
		if (s->io->write(s->io, s->buffer, TTA_FIFO_BUFFER_SIZE) != TTA_FIFO_BUFFER_SIZE)
			throw tta_exception(TTA_WRITE_ERROR);
		s->pos = s->buffer;
	}

	// update crc32 and statistics
	s->crc = crc32_table[(s->crc ^ value) & 0xff] ^ (s->crc >> 8);
	s->count++;

	*s->pos++ = (value & 0xff);
} // write_byte

static inline void write_uint16(TTA_fifo *s, TTAuint32 value) {
	write_byte(s, value);
	write_byte(s, value >>= 8);
} // write_uint16

static inline void write_uint32(TTA_fifo *s, TTAuint32 value) {
	write_byte(s, value);
	write_byte(s, value >>= 8);
	write_byte(s, value >>= 8);
	write_byte(s, value >>= 8);
} // write_uint32

static inline void write_crc32(TTA_fifo *s) {
	TTAuint32 crc = s->crc ^ 0xffffffffUL;
	write_uint32(s, crc);
} // write_crc32

static void writer_skip_bytes(TTA_fifo *s, TTAuint32 size) {
	while (size--) write_byte(s, 0);
} // writer_skip_bytes

static TTAuint32 write_tta_header(TTA_fifo *s, TTA_info *info) {
	writer_reset(s);

	// write TTA1 signature
	write_byte(s, 'T');
	write_byte(s, 'T');
	write_byte(s, 'A');
	write_byte(s, '1');

	write_uint16(s, info->format);
	write_uint16(s, info->nch);
	write_uint16(s, info->bps);
	write_uint32(s, info->sps);
	write_uint32(s, info->samples);

	write_crc32(s);

	return 22; // sizeof TTA1 header
} // write_tta_header

void tta_encoder::write_seek_table() {
	if (!seek_allowed) return;

	if (fifo.io->seek(fifo.io, info.offset) < 0 &&
		errno != 0) throw tta_exception(TTA_SEEK_ERROR);

	writer_start(&fifo);
	writer_reset(&fifo);

	for (TTAuint32 i = 0; i < info.frames; i++) {
		TTAuint32 tmp = seek_table[i] & 0xffffffffUL;
		write_uint32(&fifo, tmp);
	}

	write_crc32(&fifo);
	writer_done(&fifo);
} // write_seek_table

void tta_encoder::frame_init(TTAuint32 frame) {
	if (frame >= info.frames) return;

	fnum = frame;

	if (fnum == info.frames - 1)
		info.flen = info.flen_last;
	else info.flen = info.flen_std;

	// init entropy encoder
	for (TTAuint32 i = 0; i < info.nch; i++) {
		filter_init(&codec[i].fst, flt_set[info.depth - 1]);
		rice_init(&codec[i].rice, 10, 10);
		codec[i].prev = 0;
	}

	bcount = 0;
	bcache = 0;
	fpos = 0;

	writer_reset(&fifo);
} // frame_init

void tta_encoder::frame_init(TTAuint32 frame, TTA_io_callback *iocb) {
	fifo.io = iocb;
	writer_start(&fifo);
	frame_init(frame);
} // frame_init

void tta_encoder::encoder_init(TTAuint32 nch, TTAuint32 bps,
	TTAuint32 sps, TTAuint32 length, TTAuint32 offset) {

	// check for supported formats
	if (bps < MIN_BPS || bps > MAX_BPS || nch > MAX_NCH)
		throw tta_exception(TTA_FORMAT_ERROR);

	// set start position if required
	if (seek_allowed && fifo.io->seek(fifo.io, offset) < 0 &&
		errno != 0) throw tta_exception(TTA_WRITE_ERROR);

	writer_start(&fifo);

	info.nch = nch;
	info.bps = bps;
	info.sps = sps;
	info.samples = length;
	info.format = 1; // pcm

	// write TTA1 header
	offset += write_tta_header(&fifo, &info);

	info.offset = offset; // size of headers
	info.depth = (bps + 7) / 8;
	info.flen_std = (TTAuint32) (FRAME_TIME * sps);
	info.flen_last = length % info.flen_std;
	info.frames = length / info.flen_std + (info.flen_last ? 1 : 0);
	if (!info.flen_last) info.flen_last = info.flen_std;
	info.rate = 0;

	// allocate memory for seek table data
	seek_table = (TTAuint64 *) tta_malloc(info.frames * sizeof(TTAuint64));
	if (seek_table == NULL) throw tta_exception(TTA_MEMORY_ERROR);

	writer_skip_bytes(&fifo, (info.frames + 1) * 4);

	codec_last = codec + info.nch - 1;
	shift_bits = (4 - info.depth) << 3;

	frame_init(0);
} // encoder_init

void tta_encoder::encoder_finalize() {
	writer_done(&fifo);
	write_seek_table();
} // encoder_finalize

void tta_encoder::encode_value(TTA_codec *enc, TTAint32 value) {
	TTA_fltst *fst = &enc->fst;
	TTA_adapt *rice = &enc->rice;
	TTAuint32 k, unary, outval;

	// compress stage 1: fixed order 1 prediction
	TTAint32 tmp = value;
	value -= PREDICTOR1(enc->prev, 5);
	enc->prev = tmp;

	// compress stage 2: adaptive hybrid filter
	hybrid_filter(fst, &value, 1);

	outval = ENC(value);

	// encode Rice unsigned
	k = rice->k0;

	rice->sum0 += outval - (rice->sum0 >> 4);
	if (rice->k0 > 0 && rice->sum0 < shift_16[rice->k0])
		rice->k0--;
	else if (rice->sum0 > shift_16[rice->k0 + 1])
		rice->k0++;

	if (outval >= bit_shift[k]) {
		outval -= bit_shift[k];
		k = rice->k1;

		rice->sum1 += outval - (rice->sum1 >> 4);
		if (rice->k1 > 0 && rice->sum1 < shift_16[rice->k1])
			rice->k1--;
		else if (rice->sum1 > shift_16[rice->k1 + 1])
			rice->k1++;

		unary = 1 + (outval >> k);
	} else unary = 0;

	// put unary
	do {
		while (bcount >= 8) {
			write_byte(&fifo, bcache);
			bcache >>= 8;
			bcount -= 8;
		}

		if (unary > 23) {
			bcache |= bit_mask[23] << bcount;
			bcount += 23;
			unary -= 23;
		} else {
			bcache |= bit_mask[unary] << bcount;
			bcount += unary + 1;
			unary = 0;
		}
	} while (unary);

	if (k) {
		// put binary
		while (bcount >= 8) {
			write_byte(&fifo, bcache);
			bcache >>= 8;
			bcount -= 8;
		}
		bcache |= (outval & bit_mask[k]) << bcount;
		bcount += k;
	}
} // encode_value

void tta_encoder::encode_stream(TTAuint8 *input, TTAuint32 in_bytes,
	TTA_CALLBACK tta_callback) {
	TTA_codec *enc = codec;
	TTAuint8 *ptr = input;
	TTAuint8 *pend = input + in_bytes;
	TTAint32 curr, next, temp;
	TTAint32 res = 0;

	if (!in_bytes) return;

	READ_BUFFER(temp, ptr, info.depth, shift_bits);
	next = temp >> shift_bits;

	do {
		curr = next;
		if (ptr <= pend) {
			READ_BUFFER(temp, ptr, info.depth, shift_bits);
			next = temp >> shift_bits;
		}

		// transform data
		if (info.nch != 1) {
			if (enc < codec_last) {
				curr = res = next - curr;
			} else curr -= res / 2;
		}

		encode_value(enc, curr);

		if (enc < codec_last) {
			enc++;
		} else {
			enc = codec;
			fpos++;
		}

		if (fpos == info.flen) {
			// flush bit cache
			while (bcount) {
				write_byte(&fifo, bcache);
				bcache >>= 8;
				bcount = (bcount > 8) ? (bcount - 8) : 0;
			}
			write_crc32(&fifo);

			// save frame size
			seek_table[fnum++] = fifo.count;

			// update dynamic info
			info.rate = (fifo.count << 3) / 1070;
			if (tta_callback)
				tta_callback(info.rate, fnum, info.frames);

			frame_init(fnum);
		}
	} while (ptr <= pend);
} // encode_stream

TTAuint32 tta_encoder::encode_frame(TTAuint8 *input, TTAuint32 in_bytes,
	TTA_CALLBACK tta_callback) {
	TTA_codec *enc = codec;
	TTAuint8 *ptr = input;
	TTAuint8 *pend = input + in_bytes;
	TTAint32 curr, next, temp;
	TTAint32 res = 0;

	if (!in_bytes) return 0;

	READ_BUFFER(temp, ptr, info.depth, shift_bits);
	next = temp >> shift_bits;

	do {
		curr = next;
		if (ptr <= pend) {
			READ_BUFFER(temp, ptr, info.depth, shift_bits);
			next = temp >> shift_bits;
		}

		// transform data
		if (info.nch != 1) {
			if (enc < codec_last) {
				curr = res = next - curr;
			} else curr -= res / 2;
		}

		encode_value(enc, curr);

		if (enc < codec_last) {
			enc++;
		} else {
			enc = codec;
			fpos++;
		}

		if (fpos == info.flen) {
			// flush bit cache
			while (bcount) {
				write_byte(&fifo, bcache);
				bcache >>= 8;
				bcount = (bcount > 8) ? (bcount - 8) : 0;
			}
			write_crc32(&fifo);

			// update dynamic info
			info.rate = (fifo.count << 3) / 1070;
			if (tta_callback)
				tta_callback(info.rate, fnum, info.frames);

			break;
		}
	} while (ptr <= pend);

	return fifo.count;
} // encode_frame

tta_encoder::tta_encoder(TTA_io_callback *iocb) {
	seek_table = NULL;
	seek_allowed = true;
	fifo.io = iocb;
} // tta_encoder

tta_encoder::~tta_encoder() {
	if (seek_table) delete [] seek_table;
} // ~tta_encoder

/* eof */
