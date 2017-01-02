/*
 * libtta.h
 *
 * Description: TTA1 library interface
 * Copyright (c) 2010 Aleksander Djuric. All rights reserved.
 * Based on Buffer aware TTA encoder/decoder, developed by
 * Christophe Paris (christophe.paris <at> free.fr)
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the COPYING
 * file included in the distribution.
 *
 */

#ifndef _TTA_LIB_H
#define _TTA_LIB_H

#ifdef __GNUC__
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <string.h>
#include <stdlib.h> 
#else
#include <windows.h>
#endif
#include <stdexcept>

#ifdef CARIBBEAN
#define ALLOW_OS_CODE 1

#include "../../../rmdef/rmdef.h"
#include "../../../rmlibcw/include/rmlibcw.h"
#include "../../../rmcore/include/rmcore.h"
#endif

#define MAX_DEPTH 3
#define MAX_BPS (MAX_DEPTH*8)
#define MIN_BPS 16
#define MAX_NCH 6
#define TTA_FIFO_BUFFER_SIZE 5120

#ifdef __GNUC__
#define CALLBACK
#define TTA_EXTERN_API __attribute__((visibility("default")))
#else // MSVC
#define TTA_EXTERN_API __declspec(dllexport)
#endif

// portability definitions
#ifdef __GNUC__
#ifdef CARIBBEAN
typedef RMint8 (TTAint8);
typedef RMint16 (TTAint16);
typedef RMint32 (TTAint32);
typedef RMint64 (TTAint64);
typedef RMuint8 (TTAuint8);
typedef RMuint32 (TTAuint32);
typedef RMuint64 (TTAuint64);
#else // GNUC
typedef int8_t (TTAint8);
typedef int16_t (TTAint16);
typedef int32_t (TTAint32);
typedef int64_t (TTAint64);
typedef uint8_t (TTAuint8);
typedef uint32_t (TTAuint32);
typedef uint64_t (TTAuint64);
#endif
#else // MSVC
typedef __int8 (TTAint8);
typedef __int16 (TTAint16);
typedef __int32 (TTAint32);
typedef __int64 (TTAint64);
typedef unsigned __int8 (TTAuint8);
typedef unsigned __int32 (TTAuint32);
typedef unsigned __int64 (TTAuint64);
#endif

namespace tta
{
	typedef enum tta_error {
		TTA_NO_ERROR,
		TTA_OPEN_ERROR,
		TTA_FORMAT_ERROR,
		TTA_FILE_ERROR,
		TTA_READ_ERROR,
		TTA_WRITE_ERROR,
		TTA_SEEK_ERROR,
		TTA_MEMORY_ERROR,
	} TTA_CODEC_STATUS;

	typedef struct _tag_TTA_io_callback {
		TTAint32 (CALLBACK *read)(struct _tag_TTA_io_callback *, TTAuint8 *, TTAuint32);
		TTAint32 (CALLBACK *write)(struct _tag_TTA_io_callback *, TTAuint8 *, TTAuint32);
		TTAint64 (CALLBACK *seek)(struct _tag_TTA_io_callback *, TTAint64 offset);
	} TTA_io_callback;

	typedef struct {
		TTAuint8 buffer[TTA_FIFO_BUFFER_SIZE];
		TTAuint8 end;
		TTAuint8 *pos;
		TTAuint32 crc;
		TTAuint32 count;
		TTA_io_callback *io;
	} TTA_fifo;

	typedef struct {
		TTAuint32 format;	// audio format
		TTAuint32 nch;	// number of channels
		TTAuint32 bps;	// bits per sample
		TTAuint32 sps;	// samplerate (sps)
		TTAuint32 samples;	// data length in samples
		TTAuint32 rate;	// bitrate (kbps)
		TTAuint32 offset; // data start position (header size, bytes)
		TTAuint32 frames;	// total count of frames
		TTAuint32 depth;	// bytes per sample
		TTAuint32 flen_std;	// default frame length in samples
		TTAuint32 flen_last;	// last frame length in samples
		TTAuint32 flen;	// current frame length in samples
	} TTA_info;

	typedef struct {
		TTAuint32 k0;
		TTAuint32 k1;
		TTAuint32 sum0;
		TTAuint32 sum1;
	} TTA_adapt;

	typedef struct {
		TTAint32 index;
		TTAint32 shift;
		TTAint32 round;
		TTAint32 error;
		TTAint32 qm[8];
		TTAint32 dx[24];
		TTAint32 dl[24];
	} TTA_fltst;

	typedef struct {
		TTA_fltst fst;
		TTA_adapt rice;
		TTAint32 prev;
	} TTA_codec;

	// progress callback
	typedef void (CALLBACK *TTA_CALLBACK)(TTAuint32, TTAuint32, TTAuint32);

	///////////////////////// TTA decoder class ///////////////////////////
	class TTA_EXTERN_API tta_decoder {
	public:
		TTA_info info;	// TTA track info (nch, bps, etc.)
		bool seek_allowed;

		tta_decoder(TTA_io_callback *iocb);
		virtual ~tta_decoder();

		void decoder_init();
		void decoder_init(TTAuint32 nch, TTAuint32 bps, TTAuint32 sps, TTAuint32 length);
		void frame_init(TTAuint32 frame, bool seek_needed);
		void frame_init(TTAuint32 frame, TTA_io_callback *iocb);
		TTAuint32 decode_stream(TTAuint8 *output, TTAuint32 count, TTA_CALLBACK tta_callback=NULL);
		TTAuint32 decode_frame(TTAuint32 in_bytes, TTAuint8 *output, TTAuint32 out_bytes, TTA_CALLBACK tta_callback=NULL);
		double set_position(double seconds);
		static TTAuint32 get_frame_length(TTAuint32 sps);

	protected:
		TTA_fifo fifo;
		TTA_codec codec[MAX_NCH]; // decoder (1 per channel)
		TTA_codec *codec_last;
		TTAuint64 *seek_table;	// the playing position table
		TTAuint32 bcount;	// count of bits in cache
		TTAuint32 bcache;	// bit cache
		TTAuint32 fnum;	// currently playing frame index
		TTAuint32 fpos;	// the current position in frame

		bool read_seek_table();
		TTAint32 decode_value(TTA_codec *dec);
	}; // class tta_decoder

	///////////////////////// TTA encoder class ///////////////////////////
	class TTA_EXTERN_API tta_encoder {
	public:
		TTA_info info;	// TTA track info (nch, bps, etc.)
		bool seek_allowed;

		tta_encoder(TTA_io_callback *iocb);
		virtual ~tta_encoder();

		void encoder_init(
			TTAuint32 nch,	// number of channels
			TTAuint32 bps,	// bits per sample
			TTAuint32 sps,	// samplerate (sps)
			TTAuint32 length,	// data length in samples
			TTAuint32 offset);	// size of id3v2 header if present
		void frame_init(TTAuint32 frame);
		void frame_init(TTAuint32 frame, TTA_io_callback *iocb);
		void encode_stream(TTAuint8 *input, TTAuint32 in_bytes, TTA_CALLBACK tta_callback=NULL);
		TTAuint32 encode_frame(TTAuint8 *input, TTAuint32 in_bytes, TTA_CALLBACK tta_callback=NULL);
		void encoder_finalize();

	protected:
		TTA_fifo fifo;
		TTA_codec codec[MAX_NCH]; // decoder (1 per channel)
		TTA_codec *codec_last;
		TTAuint64 *seek_table;	// the playing position table
		TTAuint32 bcount;	// count of bits in cache
		TTAuint32 bcache;	// bit cache
		TTAuint32 fnum;	// currently playing frame index
		TTAuint32 fpos;	// the current position in frame
		TTAuint32 shift_bits; // packing int to pcm

		void write_seek_table();
		void encode_value(TTA_codec *enc, TTAint32 value);
	}; // class tta_encoder

	//////////////////////// TTA exception class //////////////////////////
	class tta_exception : public std::exception {
		tta_error err_code;

	public:
		tta_exception(tta_error code) : err_code(code) {}
		tta_error code() const { return err_code; }
	}; // class tta_exception
} // namespace tta

#endif // _TTA_LIB_H
