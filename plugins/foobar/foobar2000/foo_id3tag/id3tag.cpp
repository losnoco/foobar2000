#define MYVERSION "1.0"

/*
	change log

2004-07-26 11:04 UTC - kode54
- id3lib sucks, and so does everyone who requires v2.3 tags that don't follow spec, so here's a
  tag reader that's anal about the specifications. look at how much code I blow on URL checking!

- Coming soon, maybe: bleh, writing. ID3v2 is too anal of a standard. and you'll get v2.4 only, nyeah.

Shit that ID3v2 must follow:

- URLs follow a strict standard (validator done)
- So do timestamps, per se (need validator)

- There can be only one UFID (unique file identifier) with a given owner string
- There can be only one TXXX (user text) with a given description field - and unlike all other
  text fields, this one does not support a list of strings. nor does it officially allow linefeeds.
- There can be only one WXXX (user URL) with a given description field - but I don't support
  description reading, so why should I support writing? meh...
- Comment calls for a language field, and I don't really check that on load. Also the only field
  that allows linefeeds. Probably the same-fieldid-language-description rules apply here...
- Picture / binary attachments .... no intention of adding a textable version of this to store in
  the DB, just so I can parse it back into binary on write
- Play counter / Popularimeter, variable length binary data, minimum four bytes (no plans to support)
- There can be one LINK frame per any other frame ID, and it must precede the frame it is linked to (no link support planned)
- RGAD, binary ReplayGain frame standard
- So many other spoony, limited, specific binary and/or text chunks that Foobar2000 would not really
  be able to explicitly support

- Maybe I should support writing a limited amount of data according to the standard, for all of the
  fields I feel like reading as well. Then, I can also store all of the unmodified, untouched,
  unscanned tag data as native ordered key/value pairs, in a "secure" format with a unique header,
  in a PRIV chunk. Then I compress the whole tag.

  Ultimately, here's a basic roadmap:

  (header - probably make this 16 bytes based on a generated GUID)
  (CRC32 of data which follows - we know how long it is thanks to total size of binary chunk)
  (key/value pairs, in the form of null-terminated UTF-8 strings)

  Variable count omitted, since the chunk will ultimately be compressed anyway, and I know how much
  data there is, so I will just pull as many even pairs of strings as I can from the available block
  of data.

  This chunk will not be included in the parsers, since it needs to override any other fields. The
  frame finder will be used to scan for all PRIV chunks first, then pull the tag data from the first
  matching chunk with a valid checksum. If no header is found, or the checksum is invalid, it tries
  the next chunk. If no matches are found, it should continue with normal hash-based field recognition
  and handling.

  Oh, and maybe I'll also support footer tag writing by default, since this will only write v2.4
  tags.... Mwahahahahahaha

  *Whew*

*/

#include "foobar2000.h"

#include "../foo_input_std/id3v2_hacks.h"

//#include "resource.h"

#include <id3tag.h>

#include "lookup.h"
#include "rg_check.h"

/*
bool is_iso88591(const char * src, unsigned len = ~0)
{
	len = strlen_max(src, len);
	while (*src && len)
	{
		unsigned c, d;
		d = utf8_decode_char(src, &c, len);
		if (d == 0 || d > len) break;
		src += d;
		len -= d;
		if ((c >= 128 && c <= 159) || (c >= 256)) break;
	}
	return !len;
}
*/

extern "C" int is_replaygain(const char * name);

void id3v2_process_callback(void * ctx, const char * name, const char * value)
{
	file_info * info = (file_info *) ctx;

	if (is_replaygain(name))
		info->info_set_replaygain(name, value);
	else
		info->meta_add(name, value);
}

class tag_reader_id3v2 : public tag_processor_id3v2
{
	id3_byte_t * mem;
	id3_tag * tag;

public:
	tag_reader_id3v2() : mem(0), tag(0) {}
	~tag_reader_id3v2()
	{
		if (tag) id3_tag_delete(tag);
		if (mem) free(mem);
	}

	t_io_result read( const service_ptr_t<file> & p_file, file_info & p_info, abort_callback & p_abort )
	{
		t_uint64 skipped;
		status = g_skip( p_file, skipped, p_abort );
		if ( io_result_failed( status ) || !skipped ) return status;

		status = p_file->seek( 0, p_abort );
		if ( io_result_failed( status ) ) return status;

		mem = (id3_byte_t *) malloc( skipped );
		if ( !mem ) return io_result_error_out_of_memory;

		status = p_file->read_object( mem, skipped, p_abort );
		if ( io_result_failed( status ) ) return status;

		tag = id3_tag_parse(mem, skip);
		if ( ! tag ) return io_result_error_data;

		for (unsigned i = 0, count = tag->nframes; i < count; i++)
		{
			id3_frame * frame;
			const t_read_lookup * read_info;

			frame = tag->frames[i];
			read_info = read_lookup(frame->id, strlen(frame->id));

			if (read_info)
			{
				if (read_info->validate)
				{
					if (!read_info->validate(frame))
					{
						console::formatter() << "Invalid " << frame->id << " frame (" << i << " of " << count << ")";
						continue;
					}
				}

				if (read_info->process)
				{
					read_info->process(frame, read_info, id3v2_process_callback, info);
				}
			}
		}

		return 1;
	}

	t_io_result write(const service_ptr_t<file> & p_file,const file_info & p_info,abort_callback & p_abort)
	{
		return io_result_error_data;
	}
};

static service_factory_t<tag_processor_id3v2,tag_reader_id3v2> foo2;

DECLARE_COMPONENT_VERSION("ID3v2 tag support",MYVERSION,"ID3v2 sucks sucks sucks");
