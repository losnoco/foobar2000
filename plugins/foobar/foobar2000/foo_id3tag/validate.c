#include "lookup.h"

#include "url.h"

int text_validate(frame)
	struct id3_frame * frame;
{
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_TEXTENCODING) return 0;

	field = id3_frame_field(frame, 1);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRINGLIST) return 0;

	field = id3_frame_field(frame, 2);

	if (field) return 0;

	return 1;
}

int textx_validate(frame)
	struct id3_frame * frame;
{
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_TEXTENCODING) return 0;

	field = id3_frame_field(frame, 1);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRING) return 0;

	field = id3_frame_field(frame, 2);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRING) return 0;

	field = id3_frame_field(frame, 3);

	if (field) return 0;

	return 1;
}

int url_validate(frame)
	struct id3_frame * frame;
{
	const id3_latin1_t * url;
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_LATIN1) return 0;

	url = id3_field_getlatin1(field);

	if (!url || !is_valid_url(url)) return 0;

	field = id3_frame_field(frame, 1);

	if (field) return 0;

	return 1;
}

int urlx_validate(frame)
	struct id3_frame * frame;
{
	const id3_latin1_t * url;
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_TEXTENCODING) return 0;

	field = id3_frame_field(frame, 1);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRING) return 0;

	field = id3_frame_field(frame, 2);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_LATIN1) return 0;

	url = id3_field_getlatin1(field);

	if (!url || !is_valid_url(url)) return 0;

	field = id3_frame_field(frame, 3);

	if (field) return 0;

	return 1;
}

int comment_validate(frame)
	struct id3_frame * frame;
{
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_TEXTENCODING) return 0;

	field = id3_frame_field(frame, 1);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_LANGUAGE) return 0;

	field = id3_frame_field(frame, 2);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRING) return 0;

	field = id3_frame_field(frame, 3);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_STRINGFULL) return 0;

	field = id3_frame_field(frame, 4);

	if (field) return 0;

	return 1;
}

int uniqueid_validate(frame)
	struct id3_frame * frame;
{
	id3_length_t length;
	union id3_field * field = id3_frame_field(frame, 0);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_LATIN1) return 0;

	field = id3_frame_field(frame, 1);

	if (!field || id3_field_type(field) != ID3_FIELD_TYPE_BINARYDATA) return 0;

	id3_field_getbinarydata(field, &length);

	if (length > 64) return 0;

	return 1;
}
