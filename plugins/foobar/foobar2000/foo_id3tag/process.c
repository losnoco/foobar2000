#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <id3tag.h>
#include "lookup.h"

/* validate functions handle all field count and frame presence/type checks */

void text_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	unsigned i, count;
	union id3_field * field = id3_frame_field(frame, 1);

	for (i = 0, count = id3_field_getnstrings(field); i < count; i++)
	{
		const id3_ucs4_t * string = id3_field_getstrings(field, i);

		if (string)
		{
			id3_utf8_t * string8 = id3_ucs4_utf8duplicate(string);
			callback(callback_context, lookup->long_name, string8);
			free(string8);
		}
	}
}

void textx_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	union id3_field * field;
	const id3_ucs4_t * name, * value;
	
	field = id3_frame_field(frame, 1);
	name = id3_field_getstring(field);

	field = id3_frame_field(frame, 2);
	value = id3_field_getstring(field);

	if (name && value)
	{
		id3_utf8_t * name8 = id3_ucs4_utf8duplicate(name);
		id3_utf8_t * value8 = id3_ucs4_utf8duplicate(value);

		callback(callback_context, name8, value8);

		free(value8);
		free(name8);
	}
}

void url_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	union id3_field * field = id3_frame_field(frame, 0);
	const id3_latin1_t * value = id3_field_getlatin1(field);

	if (value)
	{
		id3_ucs4_t * value4 = id3_latin1_ucs4duplicate(value);
		id3_utf8_t * value8 = id3_ucs4_utf8duplicate(value4);

		callback(callback_context, lookup->long_name, value8);

		free(value8);
		free(value4);
	}
}

void urlx_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	union id3_field * field;
	const id3_ucs4_t * description;
	const id3_latin1_t * value;

	field = id3_frame_field(frame, 1);
	description = id3_field_getstring(field);

	field = id3_frame_field(frame, 2);
	value = id3_field_getlatin1(field);

	if (description && value)
	{
		id3_ucs4_t * value4 = id3_latin1_ucs4duplicate(value);
		id3_utf8_t * value8 = id3_ucs4_utf8duplicate(value4);

		callback(callback_context, lookup->long_name, value8);

		free(value8);
		free(value4);
	}
}

void comment_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	union id3_field * field;
	const id3_ucs4_t * description;
	const id3_ucs4_t * value;

	field = id3_frame_field(frame, 2);
	description = id3_field_getstring(field);

	field = id3_frame_field(frame, 3);
	value = id3_field_getstring(field);

	if (description && value)
	{
		id3_utf8_t * value8 = id3_ucs4_utf8duplicate(value);

		callback(callback_context, lookup->long_name, value8);

		free(value8);
	}
}

static get_hex(int val)
{
	assert(val >= 0 && val < 16);
	if (val < 10) return '0' + val;
	else return 'A' - 10 + val;
}

void uniqueid_process(frame, lookup, callback, callback_context)
	struct id3_frame * frame;
	const struct t_read_lookup * lookup;
	t_process_callback callback;
	void * callback_context;
{
	union id3_field * field;
	const id3_latin1_t * owner;
	const id3_byte_t * data;
	id3_length_t length;

	field = id3_frame_field(frame, 0);
	owner = id3_field_getlatin1(field);

	field = id3_frame_field(frame, 1);
	data = id3_field_getbinarydata(field, &length);

	if (owner && length && data)
	{
		id3_length_t i;
		size_t len_owner;
		id3_utf8_t * output;

		if (*owner)
		{
			id3_ucs4_t * owner4 = id3_latin1_ucs4duplicate(owner);
			id3_utf8_t * owner8 = id3_ucs4_utf8duplicate(owner4);
			size_t len_owner = strlen(owner8);
			output = malloc(len_owner + 2 + length * 2 + 1); /* colon, one space, null */

			memcpy(output, owner8, len_owner);
			output[len_owner++] = ':';
			output[len_owner++] = ' ';

			free(owner8);
			free(owner4);
		}
		else
		{
			len_owner = 0;
			output = malloc(length * 2 + 1);
		}

		for (i = 0; i < length; i++)
		{
			output[len_owner++] = get_hex(*data >> 4);
			output[len_owner++] = get_hex(*data & 15);
		}

		output[len_owner] = 0;

		callback(callback_context, lookup->long_name, output);

		free(output);
	}
}
