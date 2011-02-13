#ifndef _MIDI_CONTAINER_H_
#define _MIDI_CONTAINER_H_

#include "../../pfc/pfc.h"

struct midi_event
{
	enum
	{
		max_static_data_count = 16
	};

	enum event_type
	{
		note_off = 0,
		note_on,
		polyphonic_aftertouch,
		control_change,
		program_change,
		channel_aftertouch,
		pitch_wheel,
		extended
	};

	unsigned m_timestamp;

	event_type m_type;
	unsigned m_channel;
	unsigned m_data_count;
	t_uint8 m_data[max_static_data_count];
	pfc::array_t<t_uint8> m_ext_data;

	midi_event() : m_timestamp(0), m_type(note_off), m_channel(0), m_data_count(0) { }
	midi_event( const midi_event & p_in );
	midi_event( unsigned p_timestamp, event_type p_type, unsigned p_channel, const t_uint8 * p_data, t_size p_data_count );

	unsigned get_data_count() const;
	void copy_data( t_uint8 * p_out, unsigned p_offset, unsigned p_count ) const;
};

class midi_track
{
	pfc::array_t<midi_event> m_events;

public:
	midi_track() { }
	midi_track(const midi_track & p_in);

	void add_event( const midi_event & p_event );
	t_size get_count() const;
	const midi_event & operator [] ( t_size p_index ) const;
};

struct tempo_entry
{
	unsigned m_timestamp;
	unsigned m_tempo;

	tempo_entry() : m_timestamp(0), m_tempo(0) { }
	tempo_entry(unsigned p_timestamp, unsigned p_tempo);
};

class tempo_map
{
	pfc::array_t<tempo_entry> m_entries;

public:
	void add_tempo( unsigned p_tempo, unsigned p_timestamp );
	const unsigned timestamp_to_ms( unsigned p_timestamp, unsigned p_dtx ) const;
};

struct system_exclusive_entry
{
	t_size m_offset;
	t_size m_length;
	system_exclusive_entry() : m_offset(0), m_length(0) { }
	system_exclusive_entry(const system_exclusive_entry & p_in);
	system_exclusive_entry(t_size p_offset, t_size p_length);
};

class system_exclusive_table
{
	pfc::array_t<t_uint8> m_data;
	pfc::array_t<system_exclusive_entry> m_entries;

public:
	unsigned add_entry( const t_uint8 * p_data, t_size p_size );
	void get_entry( unsigned p_index, const t_uint8 * & p_data, t_size & p_size );
};

struct midi_stream_event
{
	unsigned m_timestamp;
	unsigned m_event;

	midi_stream_event() : m_timestamp(0), m_event(0) { }
	midi_stream_event(unsigned p_timestamp, unsigned p_event);
};

struct midi_meta_data_item
{
	unsigned m_timestamp;
	pfc::string8 m_name;
	pfc::string8 m_value;

	midi_meta_data_item() : m_timestamp(0) { }
	midi_meta_data_item(const midi_meta_data_item & p_in);
	midi_meta_data_item(unsigned p_timestamp, const char * p_name, const char * p_value);
};

class midi_meta_data
{
	pfc::array_t<midi_meta_data_item> m_data;

public:
	midi_meta_data() { }

	void add_item( const midi_meta_data_item & p_item );

	void append( const midi_meta_data & p_data );
	
	bool get_item( const char * p_name, midi_meta_data_item & p_out ) const;

	t_size get_count() const;

	const midi_meta_data_item & operator [] ( t_size p_index ) const;
};

class midi_container
{
	unsigned m_form;
	unsigned m_dtx;
	unsigned m_channel_mask;
	tempo_map m_tempo_map;
	pfc::array_t<midi_track> m_tracks;

	midi_meta_data m_extra_meta_data;

	unsigned m_timestamp_end;

	unsigned m_timestamp_loop_start;
	unsigned m_timestamp_loop_end;

	static void encode_delta( pfc::array_t<t_uint8> & p_out, unsigned delta );

public:
	midi_container() : m_channel_mask(0), m_timestamp_end(0), m_timestamp_loop_start(~0), m_timestamp_loop_end(~0) { }

	void initialize( unsigned p_form, unsigned p_dtx );

	void add_track( const midi_track & p_track );

	void add_track_event( t_size p_track_index, const midi_event & p_event );

	void set_extra_meta_data( const midi_meta_data & p_data );

	void serialize_as_stream( pfc::array_t<midi_stream_event> & p_stream, system_exclusive_table & p_system_exclusive, bool clean_emidi ) const;

	void serialize_as_standard_midi_file( pfc::array_t<t_uint8> & p_midi_file ) const;

	const unsigned get_timestamp_end(bool ms = false) const;

	const unsigned get_format() const;
	const unsigned get_track_count() const;
	const unsigned get_channel_count() const;

	const unsigned get_timestamp_loop_start(bool ms = false) const;
	const unsigned get_timestamp_loop_end(bool ms = false) const;

	void get_meta_data( midi_meta_data & p_out );

	void scan_for_loops( bool p_xmi_loops, bool p_marker_loops );
};

#endif
