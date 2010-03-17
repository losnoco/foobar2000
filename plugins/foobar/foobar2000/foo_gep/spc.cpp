#include "base.h"
#include "reader.h"
#include "config.h"

#include <gme/blargg_endian.h>
#include <gme/Spc_Emu.h>
//#include <gme/SPC_Filter.h>

#include "../helpers/window_placement_helper.h"

#include "resource.h"

//#undef HEADER_STRING
//#define HEADER_STRING(i,n,f) if ((f)[0]) (i).meta_set((n), pfc::stringcvt::string_utf8_from_ansi((f), sizeof((f))))
//#define HEADER_STRING(i,n,f) meta_add((i), (n), (f), sizeof(f))

static const char field_length[]="spc_length";
static const char field_fade[]="spc_fade";

static const char spc_vis_field_registers[]="spc_dsp_registers";
static const char spc_vis_field_env_modes[]="spc_dsp_env_modes";

// {65753F0B-DADE-4341-BDE7-89564C7B5596}
static const GUID guid_cfg_placement = 
{ 0x65753f0b, 0xdade, 0x4341, { 0xbd, 0xe7, 0x89, 0x56, 0x4c, 0x7b, 0x55, 0x96 } };

// This remembers the editor window's position if user enables "Remember window position" in player UI settings
static cfg_window_placement cfg_placement(guid_cfg_placement);

//based on Uematsu.h from SuperJukebox


//Sub-chunk ID's
#define	XID_SONG    0x01
#define	XID_GAME    0x02
#define	XID_ARTIST  0x03
#define	XID_DUMPER  0x04
#define	XID_DATE    0x05
#define	XID_EMU     0x06
#define	XID_CMNTS   0x07
#define	XID_INTRO   0x30
#define	XID_LOOP    0x31
#define	XID_END     0x32
#define	XID_FADE    0x33
#define	XID_MUTE    0x34
#define XID_LOOPX   0x35
#define XID_AMP     0x36
#define	XID_OST     0x10
#define	XID_DISC    0x11
#define	XID_TRACK   0x12
#define	XID_PUB     0x13
#define	XID_COPY    0x14

//Data types
#define	XTYPE_DATA	0x00
#define	XTYPE_STR	0x01
#define	XTYPE_INT	0x04

typedef struct _ID666TAG
{
	char     szTitle[256];     //Title of song
	char     szGame[256];      //Name of game
	char     szArtist[256];    //Name of artist
	char     szPublisher[256]; //Game publisher
	char     szDumper[256];    //Name of dumper
	char     szDate[256];      //Date dumped
	char     szComment[256];   //Optional comment
	char     szOST[256];       //Original soundtrack title
	t_uint32 uSong_ms;         //Length of song
	t_uint32 uLoop_ms;         //Length of loop
	t_uint32 uLoopCount;       //Loop count
	t_uint32 uEnd_ms;          //Length of end
	t_uint32 uFade_ms;         //Length of fadeout
	t_uint8  bDisc;            //OST disc number
	t_uint16 wTrack;           //OST track number
	t_uint16 wCopyright;       //Game copyright date
	t_uint8  bMute;            //Bitmask for channel states	
	t_uint8  bEmulator;        //Emulator used to dump
	t_uint32 uAmplification;   //Amplification level
}ID666TAG,*PID666TAG,FAR *LPID666TAG;

static void SetDate(LPSTR lpszDate,int year,int month,int day)
{
	if(year<1900)
	{
		year+=1900;
//		if(year<1997)year+=100;
	}
//	if(year<1997)year=0;
	if(month<1||month>12)year=0;
	if(day<1||day>31)year=0;

	if(year)
	{
/*	What the bloody fuck is this?
		if(month>2)
		{
			if(!(year&3)&&year%100)day++;
			else if(!(year%400))day++;
		}
*/
		pfc::string8 temp;
		temp << month;
		temp.add_char('/');
		temp << day;
		temp.add_char('/');
		temp << year;
		strcpy(lpszDate, temp);
	}
	else lpszDate[0]='\0';
}

static void parse_id666( service_ptr_t< file > & p_file, LPID666TAG lpTag, bool aligned, abort_callback & p_abort )
{
	while ( ! p_file->is_eof( p_abort ) )
	{
		t_uint8 id, type;
		t_uint16    data;

		p_file->read_object_t( id, p_abort );
		p_file->read_object_t( type, p_abort );
		p_file->read_lendian_t( data, p_abort );

		switch ( type )
		{
		case XTYPE_STR:
			{
				if ( data < 1 || data > 256 ) throw exception_io_data();

				switch ( id )
				{
				case XID_SONG:
					p_file->read_object( lpTag->szTitle, data, p_abort );
					break;

				case XID_GAME:
					p_file->read_object( lpTag->szGame, data, p_abort );
					break;

				case XID_ARTIST:
					p_file->read_object( lpTag->szArtist, data, p_abort );
					break;

				case XID_PUB:
					p_file->read_object( lpTag->szPublisher, data, p_abort );
					break;

				case XID_OST:
					p_file->read_object( lpTag->szOST, data, p_abort );
					break;

				case XID_DUMPER:
					p_file->read_object( lpTag->szDumper, data, p_abort );
					break;

				case XID_CMNTS:
					p_file->read_object( lpTag->szComment, data, p_abort );
					break;

				default:
					p_file->skip( data, p_abort );
					break;
				}

				if ( aligned && ( data & 3 ) ) p_file->skip( 4 - ( data & 3 ), p_abort );
			}
			break;

		case XTYPE_INT:
			{
				if ( data != 4 ) throw exception_io_data();

				t_uint32 value;
				p_file->read_lendian_t( value, p_abort );

				switch ( id )
				{
				case XID_DATE:
					SetDate( lpTag->szDate, ( value >> 16 ) & 255, ( value >> 8 ) & 255, value & 255 );
					break;

				case XID_INTRO:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uSong_ms = value / 64;
					break;

				case XID_LOOP:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uLoop_ms = value / 64;
					break;

				case XID_END:
					if ( value > 383999999 ) value = 383999999;
					lpTag->uEnd_ms = value / 64;
					break;

				case XID_FADE:
					if ( value > 3839999 ) value = 3839999;
					lpTag->uFade_ms = value / 64;
					break;

				case XID_AMP:
					if ( value < 32768 ) value = 32768;
					else if ( value > 524288 ) value = 524288;
					lpTag->uAmplification = value;
					break;
				}
			}
			break;

		case XTYPE_DATA:
			{
				switch ( id )
				{
				case XID_EMU:
					lpTag->bEmulator = t_uint8( data );
					break;

				case XID_DISC:
					lpTag->bDisc = t_uint8( data );
					if ( lpTag->bDisc > 9 ) lpTag->bDisc = 9;
					break;

				case XID_TRACK:
					if ( data > ( ( 100 << 8 ) - 1 ) ) data = 0;
					lpTag->wTrack = data;
					break;

				case XID_COPY:
					lpTag->wCopyright = data;
					break;

				case XID_MUTE:
					lpTag->bMute = t_uint8( data );
					break;

				case XID_LOOPX:
					if ( data < 1 ) data = 1;
					else if ( data > 9 ) data = 9;
					lpTag->uLoopCount = data;
					break;

				case XID_AMP:
					lpTag->uAmplification = data;
					lpTag->uAmplification <<= 12;
					if ( lpTag->uAmplification < 32768 ) lpTag->uAmplification = 32768;
					else if ( lpTag->uAmplification > 524288 ) lpTag->uAmplification = 524288;
					break;
				}
			}
			break;
		}
	}
}

static const t_uint8 xid6_signature[] = {'x', 'i', 'd', '6'};

static void load_id666(service_ptr_t<file> & p_file, LPID666TAG lpTag, abort_callback & p_abort)//must be seeked to correct spot before calling
{
	t_uint8 szBuf[4];
	p_file->read_object( &szBuf, 4, p_abort );

	if( ! memcmp( szBuf, xid6_signature, 4 ) )
	{
		t_uint32 tag_size;
		p_file->read_lendian_t( tag_size, p_abort );

		t_filesize offset = p_file->get_position( p_abort );

		service_ptr_t< reader_limited > m_file = new service_impl_t< reader_limited >;
		m_file->init( p_file, offset, offset + tag_size, p_abort );

		service_ptr_t<file> p_file = m_file.get_ptr();

		try
		{
			parse_id666( p_file, lpTag, false, p_abort );
		}
		catch ( const exception_io_data & )
		{
			p_file->seek( 0, p_abort );

			memset( lpTag, 0, sizeof( *lpTag ) );

			parse_id666( p_file, lpTag, true, p_abort );
		}
	}
	else throw exception_tag_not_found();
}

static void write_id666( service_ptr_t<file> & p_file, const file_info & p_info, abort_callback & p_abort )
{
	char buffer[32];
	const char * value;
	pfc::stringcvt::string_ansi_from_utf8 converter;

	p_file->seek( 0x23, p_abort );
	buffer [0] = 26;
	p_file->write_object( buffer, 1, p_abort );

	p_file->seek( offsetof( Spc_Emu::header_t, song ), p_abort );

	memset( buffer, 0, sizeof( buffer ) );
	value = p_info.meta_get( "title", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 32 );
	}
	p_file->write_object( buffer, 32, p_abort );

	memset( buffer, 0, sizeof( buffer ) );
	value = p_info.meta_get( "album", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 32 );
	}
	p_file->write_object( buffer, 32, p_abort );

	memset( buffer, 0, 16 );
	value = p_info.meta_get( "dumper", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 16 );
	}
	p_file->write_object( buffer, 16, p_abort );

	memset( buffer, 0, sizeof( buffer ) );
	value = p_info.meta_get( "comment", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 32 );
	}
	p_file->write_object( buffer, 32, p_abort );

	memset( buffer, 0, 11 );
	value = p_info.meta_get( "date", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 11 );
	}
	p_file->write_object( buffer, 11, p_abort );

	memset( buffer, 0, 3 );
	value = p_info.info_get( field_length );
	if ( value )
	{
		size_t length = strlen( value );
		if ( length > 3 )
		{
			length -= 3;
			strncpy( buffer, value, min( 3, length ) );
		}
	}
	p_file->write_object( buffer, 3, p_abort );

	memset( buffer, 0, 5 );
	value = p_info.info_get( field_fade );
	if ( value ) strncpy( buffer, value, 5 );
	p_file->write_object( buffer, 5, p_abort );

	memset( buffer, 0, sizeof( buffer ) );
	value = p_info.meta_get( "artist", 0 );
	if ( value )
	{
		converter.convert( value );
		strncpy( buffer, converter, 32 );
	}
	p_file->write_object( buffer, 32, p_abort );
}

class write_xid6
{
	service_ptr_t<file> & m_file;
	abort_callback      & m_abort;

	void write_header( t_uint8 id, t_uint8 type, t_uint16 data )
	{
		m_file->write_object_t( id, m_abort );
		m_file->write_object_t( type, m_abort );
		m_file->write_lendian_t( data, m_abort );
	}

	void write_data( t_uint8 id, t_uint16 data )
	{
		write_header( id, XTYPE_DATA, data );
	}

	void write_int( t_uint8 id, t_uint32 value )
	{
		write_header( id, XTYPE_INT, 4 );
		m_file->write_lendian_t( value, m_abort );
	}

	void write_string( t_uint8 id, const char * value )
	{
		unsigned char temp[4] = {0};

		size_t length = strlen( value );
		if ( length > 255 ) length = 255;

		write_header( id, XTYPE_STR, length + 1 );
		m_file->write_object( value, length, m_abort );
		m_file->write_object( temp, 4 - ( length & 3 ), m_abort );
	}

public:
	write_xid6( service_ptr_t<file> & p_file, const file_info & p_info, abort_callback & p_abort )
		: m_file( p_file ), m_abort( p_abort )
	{
		pfc::stringcvt::string_ansi_from_utf8 converter;
		t_filesize offset_tag_start;
		const char * value;
		t_uint32 int32 = 0;

		p_file->seek_ex( 0, file::seek_from_eof, p_abort );

		p_file->write_object( xid6_signature, 4, p_abort );
		p_file->write_object_t( int32, p_abort );

		offset_tag_start = p_file->get_position( p_abort );

		value = p_info.meta_get( "title", 0 );
		if ( value )
		{
			converter.convert( value );
			if ( strlen( converter ) > 32 ) write_string( XID_SONG, converter );
		}

		value = p_info.meta_get( "album", 0 );
		if ( value )
		{
			converter.convert( value );
			if ( strlen( converter ) > 32 ) write_string( XID_GAME, converter );
		}

		value = p_info.meta_get( "artist", 0 );
		if ( value )
		{
			converter.convert( value );
			if ( strlen( converter ) > 32 ) write_string( XID_ARTIST, converter );
		}

		value = p_info.meta_get( "dumper", 0 );
		if ( value )
		{
			converter.convert( value );
			if ( strlen( converter ) > 16 ) write_string( XID_DUMPER, converter );
		}

		value = p_info.meta_get( "comment", 0 );
		if ( value )
		{
			converter.convert( value );
			if ( strlen( converter ) > 32 ) write_string( XID_CMNTS, converter );
		}

		value = p_info.meta_get( "OST", 0 );
		if ( value )
		{
			converter.convert( value );
			write_string( XID_OST, converter );
		}

		value = p_info.meta_get( "discnumber", 0 );
		if ( value )
		{
			char * end;
			unsigned disc = strtoul( value, &end, 10 );
			if ( !*end && disc > 0 && disc <= 9 )
				write_data( XID_DISC, disc );
		}

		value = p_info.meta_get( "tracknumber", 0 );
		if ( value )
		{
			char * end;
			unsigned track = strtoul( value, &end, 10 );
			if ( track > 0 && track < 100 )
				write_data( XID_TRACK, track * 0x100 + *end );
		}

		value = p_info.meta_get( "copyright", 0 );
		if ( value )
		{
			char * end;
			unsigned copyright_year = strtoul( value, &end, 10 );
			if ( copyright_year > 0 && copyright_year < 65536 )
				write_data( XID_COPY, copyright_year );

			while ( *end && *end == ' ' ) end++;
			if ( *end )
			{
				converter.convert( end );
				write_string( XID_PUB, converter );
			}
		}

		value = p_info.info_get( field_length );
		if ( value )
		{
			char * end;
			unsigned length = strtoul( value, &end, 10 );
			if ( !*end && length > 0 && ( length % 1000 || length > 999000 ) )
				write_int( XID_INTRO, length * 64 );
		}

		value = p_info.info_get( field_fade );
		if ( value )
		{
			char * end;
			unsigned fade = strtoul( value, &end, 10 );
			if ( !*end && fade > 99999 )
				write_int( XID_FADE, fade * 64 );
		}

		t_filesize offset = p_file->get_position( p_abort );
		offset -= offset_tag_start;
		if ( offset > ( 1 << 30 ) ) throw exception_io_data();

		if ( offset )
		{
			int32 = t_uint32( offset );
			p_file->seek( offset_tag_start - 4, p_abort );
			p_file->write_lendian_t( int32, p_abort );
		}
		else
		{
			p_file->seek( offset_tag_start - 8, p_abort );
			p_file->set_eof( p_abort );
		}
	}
};

/*class Spc_Emu_Filtered : public Spc_Emu
{
	SPC_Filter filter;

protected:
	blargg_err_t start_track_( int track )
	{
		filter.clear();
		return Spc_Emu::start_track_( track );
	}

	blargg_err_t play_( long count, sample_t* out )
	{
		blargg_err_t ret = Spc_Emu::play_( count, out );
		if ( ! ret ) filter.run( out, count );
		return ret;
	}
};*/

static char get_hex_nibble( unsigned char val )
{
	if ( val < 10 ) return '0' + val;
	else return 'A' + val - 10;
}

static void add_hex( pfc::string_base & p_out, unsigned char val )
{
	p_out.add_byte( get_hex_nibble( val >> 4 ) );
	p_out.add_byte( get_hex_nibble( val & 15 ) );
}

class input_spc : public input_gep
{
	file_info_impl    m_info;
	bool              retagging;

	bool vis_info;
	unsigned char old_regs[Spc_Dsp::register_count];
	Spc_Dsp::env_mode_t old_env_modes[Spc_Dsp::voice_count];

public:
	input_spc()
	{
		sample_rate = Spc_Emu::native_sample_rate;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		if ( ! ( cfg_format_enable & 2 ) ) return false;
		return ! stricmp( p_extension, "spc" );
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		assert(sample_rate == Spc_Emu::native_sample_rate);

		input_gep::open( p_filehint, p_path, p_reason, p_abort );

		{
			char signature[ 35 ];
			m_file->read_object_t( signature, p_abort );

			if ( strncmp( signature, "SNES-SPC700 Sound File Data", 27 ) != 0 )
			{
				console::info("Not an SPC file");
				throw exception_io_data();
			}

			bool valid_tag = false;

			try
			{
				tag_processor::read_trailing( m_file, m_info, p_abort );

				const char * p;
				p = m_info.meta_get( field_length, 0 );
				if (p)
				{
					m_info.info_set( field_length, p );
					tag_song_ms = atoi( p );
					m_info.meta_remove_field( field_length );
				}
				else
				{
					tag_song_ms = 0;
				}
				p = m_info.meta_get( field_fade, 0 );
				if ( p )
				{
					m_info.info_set( field_fade,p );
					tag_fade_ms = atoi( p );
					m_info.meta_remove_field( field_fade );
				}
				else
				{
					tag_fade_ms = 0;
				}

				valid_tag = true;
			}
			catch ( const exception_io_data & ) {}

			if ( ! valid_tag )
			{
				m_file->seek( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );

				delete emu;
				if ( p_reason != input_open_decode )
					emu = gme_spc_type->new_info();
				else
				{
					// XXX needs to be in sync with crap in decode_initialize, or something
					emu = new Spc_Emu; //_Filtered;
				}
				if ( !emu ) throw std::bad_alloc();

				ERRCHK( emu->set_sample_rate( Spc_Emu::native_sample_rate ) );
				ERRCHK( emu->load( rdr ) );
				handle_warning();

				if ( p_reason == input_open_decode )
				{
					static_cast<Spc_Emu *> (this->emu)->disable_surround( !! ( cfg_spc_anti_surround ) );
					static_cast<Spc_Emu *> (this->emu)->interpolation_level( cfg_spc_interpolation );
				}

				track_info_t i;
				ERRCHK( emu->track_info( &i, 0 ) );

				HEADER_STRING( m_info, "album", i.game );
				HEADER_STRING( m_info, "title", i.song );
				HEADER_STRING( m_info, "artist", i.author );
				HEADER_STRING( m_info, "copyright", i.copyright );
				HEADER_STRING( m_info, "comment", i.comment );
				HEADER_STRING( m_info, "dumper", i.dumper );
				HEADER_STRING( m_info, "OST", i.ost );
				HEADER_STRING( m_info, "discnumber", i.disc );
				HEADER_STRING( m_info, "tracknumber", i.track );

				if ( i.length > 0 ) tag_song_ms = i.length;
				if ( i.fade_length > 0 ) tag_fade_ms = i.fade_length;
#if 0
				HEADER_STRING(m_info, "title", m_header.song);
				HEADER_STRING(m_info, "album", m_header.game);
				HEADER_STRING(m_info, "dumper", m_header.dumper);
				HEADER_STRING(m_info, "comment", m_header.comment);
				//HEADER_STRING(m_info, "date", m_header.date);
				if ((m_header.date)[0]) m_info.meta_set("date", pfc::stringcvt::string_utf8_from_ansi(((const char *)&m_header.date), sizeof((m_header.date))));
				HEADER_STRING(m_info, "artist", m_header.author);

				tag_song_ms = atoi(pfc::string_simple((const char *)&m_header.len_secs, sizeof(m_header.len_secs))) * 1000;
				tag_fade_ms = atoi(pfc::string_simple((const char *)&m_header.fade_msec, sizeof(m_header.fade_msec)));
				voice_mask = m_header.mute_mask;

				try
				{
					ID666TAG tag;

					memset(&tag, 0, sizeof(tag));

					m_file->seek( 66048, p_abort );
					load_id666( m_file, & tag, p_abort );

					HEADER_STRING(m_info, "title", tag.szTitle);
					HEADER_STRING(m_info, "album", tag.szGame);
					HEADER_STRING(m_info, "artist", tag.szArtist);
					HEADER_STRING(m_info, "dumper", tag.szDumper);
					HEADER_STRING(m_info, "date", tag.szDate);
					HEADER_STRING(m_info, "comment", tag.szComment);

					HEADER_STRING(m_info, "OST", tag.szOST);
					HEADER_STRING(m_info, "publisher", tag.szPublisher);

					if ( tag.wTrack > 255 )
					{
						pfc::string8 temp;
						temp = pfc::format_int( tag.wTrack >> 8 );
						if ( tag.wTrack & 255 )
						{
							char foo[ 2 ] = { tag.wTrack & 255, 0 };
							temp << pfc::stringcvt::string_utf8_from_ansi( foo );
						}
						m_info.meta_set("tracknumber", temp);
					}

					if ( tag.bDisc > 0 )
						m_info.meta_set("disc", pfc::format_int( tag.bDisc ) );

					if (tag.uSong_ms) tag_song_ms = tag.uSong_ms;
					if (tag.uFade_ms) tag_fade_ms = tag.uFade_ms;
					voice_mask = tag.bMute;
				}
				catch ( const exception_io_data & ) {}
#endif

				if (tag_song_ms > 0) m_info.info_set_int(field_length, tag_song_ms);
				if (tag_fade_ms > 0) m_info.info_set_int(field_fade, tag_fade_ms);
			}

			if (!tag_song_ms)
			{
				tag_song_ms = cfg_default_length;
				tag_fade_ms = cfg_default_fade;
			}
		}

		retagging = p_reason == input_open_info_write;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( m_info );

		p_info.info_set( "codec", "SPC" );
		p_info.info_set( "encoding", "synthesized" );

		p_info.info_set_int( "samplerate", Spc_Emu::native_sample_rate );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set_int( "bitspersample", 16 );

		p_info.set_length( double( tag_song_ms + tag_fade_ms ) * .001 );
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Spc_Emu * emu = ( Spc_Emu * ) this->emu;
		if ( ! emu )
		{
			this->emu = emu = new Spc_Emu;//_Filtered;

			try
			{
				m_file->seek( 0, p_abort );
				foobar_File_Reader rdr( m_file, p_abort );

				ERRCHK( emu->set_sample_rate( Spc_Emu::native_sample_rate ) );
				ERRCHK( emu->load( rdr ) );
				handle_warning();
			}
			catch(...)
			{
				if ( emu )
				{
					delete emu;
					this->emu = emu = NULL;
				}
				throw;
			}

			if ( ! retagging ) m_file.release();

			//emu->mute_voices( voice_mask );

			emu->disable_surround( !! ( cfg_spc_anti_surround ) );
			emu->interpolation_level( cfg_spc_interpolation );
		}

		input_gep::decode_initialize( 0, p_flags, p_abort );

		first_block = false;

		vis_info = !! ( p_flags & input_flag_playback );

		if ( vis_info )
		{
			memset( old_regs, 0, sizeof( old_regs ) );
			memset( old_env_modes, 0xFF, sizeof( old_env_modes) );
		}
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		if ( emu && vis_info )
		{
			bool ret = false;

			unsigned char regs[Spc_Dsp::register_count];
			Spc_Dsp::env_mode_t env_modes[Spc_Dsp::voice_count];

			const Spc_Dsp * dsp = (( Spc_Emu * )emu)->get_apu()->get_dsp();

			for ( unsigned i = 0; i < Spc_Dsp::register_count; i++ ) regs[ i ] = dsp->read( i );
			for ( unsigned i = 0; i < Spc_Dsp::voice_count; i++ ) env_modes[ i ] = dsp->get_voice( i )->env_mode;

			pfc::string8 temp;

			if ( memcmp( old_regs, regs, sizeof( old_regs ) ) )
			{
				temp.reset();

				memcpy( old_regs, regs, sizeof( old_regs ) );

				for ( unsigned i = 0; i < Spc_Dsp::register_count; i++ )
				{
					add_hex( temp, regs[ i ] );
				}

				p_out.info_set( spc_vis_field_registers, temp );

				ret = true;
			}

			if ( memcmp( old_env_modes, env_modes, sizeof( old_env_modes ) ) )
			{
				temp.reset();

				memcpy( old_env_modes, env_modes, sizeof( old_env_modes ) );

				for ( unsigned i = 0; i < Spc_Dsp::voice_count; i++ )
				{
					temp.add_byte( '0' + (int) env_modes[ i ] );
				}

				p_out.info_set( spc_vis_field_env_modes, temp );

				ret = true;
			}

			if ( ret ) p_timestamp_delta = 512 / Spc_Emu::native_sample_rate;

			return ret;
		}

		return false;
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		m_file->seek( 66048, p_abort );
		m_file->set_eof( p_abort );

		m_info.copy( p_info );

		write_id666( m_file, p_info, p_abort );
		write_xid6( m_file, p_info, p_abort );

		file_info_impl l_info;
		l_info.copy( p_info );

		{
			const char * p;
			p = l_info.info_get( field_length );
			if (p)
			{
				tag_song_ms = atoi(p);
				l_info.meta_set( field_length, p );
			}
			p = l_info.info_get( field_fade );
			if (p)
			{
				tag_fade_ms = atoi(p);
				l_info.meta_set( field_fade, p );
			}
		}

		tag_processor::write_apev2( m_file, l_info, p_abort );

		m_stats = m_file->get_stats( p_abort );
	}

	void retag_commit( abort_callback & p_abort )
	{
	}
};

class archive_rsn : public archive_impl
{
	class archive_rsn_callback : public archive_callback
	{
		archive_callback & m_callback;
		pfc::string8       m_extension;

	public:
		archive_rsn_callback( archive_callback & p_callback, const char * p_extension ) : m_callback( p_callback ), m_extension( p_extension ) { }

		virtual bool on_entry( archive * owner, const char * url, const t_filestats & p_stats, const service_ptr_t<file> & p_reader )
		{
			pfc::string8 m_url, archive, file;
			archive_impl::g_parse_unpack_path( url, archive, file );
			archive_impl::g_make_unpack_path( m_url, pfc::string_replace_extension( archive, m_extension ), file, "rar" );
			return m_callback.on_entry( owner, m_url, p_stats, p_reader );
		}

		bool is_aborting() const {return m_callback.is_aborting();}
		abort_callback_event get_abort_event() const {return m_callback.get_abort_event();}
	};

	static bool g_query_rar_service( service_ptr_t<archive> & p_arch, pfc::string8 & p_path, const char * p_archive, const char * p_file )
	{
		g_make_unpack_path( p_path, p_archive, p_file, "rar" );

		service_enum_t<filesystem> e;
		service_ptr_t<filesystem> f;
		while( e.next( f ) )
		{
			service_ptr_t<archive> arch;
			if (f->service_query_t(arch))
			{
				if (arch->is_our_path(p_path))
				{
					p_arch = arch;
					return true;
				}
			}
		}

		return false;
	}

public:
	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char * get_archive_type()
	{
		return "rsn";
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
#if 0
		pfc::string8 m_path;
		service_ptr_t< archive > m_arch;
		if (g_query_rar_service( m_arch, m_path, p_archive, p_file ))
		{
			bool dummy;
			t_filestats m_stats;
			m_arch->get_stats( m_path, m_stats, dummy, p_abort );
			return m_stats;
		}
		else
#endif
			throw exception_io_unsupported_format();
	}

	virtual void open_archive( service_ptr_t< file > & p_out, const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
#if 0
		pfc::string8 m_path;
		service_ptr_t< archive > m_arch;
		if (g_query_rar_service( m_arch, m_path, p_archive, p_file ))
		{
			m_arch->open( p_out, m_path, open_mode_read, p_abort );
		}
		else
#endif
			throw exception_io_unsupported_format();
	}

	virtual void archive_list( const char * path, const service_ptr_t< file > & p_reader, archive_callback & p_out, bool p_want_readers )
	{
		pfc::string_extension p_extension( path );
		if ( stricmp_utf8( p_extension, "rsn" ) )
			throw exception_io_unsupported_format();

		pfc::string8 m_path;
		service_ptr_t< archive > m_arch;
		if (g_query_rar_service( m_arch, m_path, path, "rsn" ))
		{
			service_ptr_t< file > m_file = p_reader;
			if ( m_file.is_empty() )
				filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

			archive_rsn_callback m_out( p_out, p_extension );

			m_arch->archive_list( pfc::string_replace_extension(path, "rar"), m_file, m_out, p_want_readers );
		}
		else throw exception_io_unsupported_format();
	}
};

typedef struct
{
	unsigned song, fade;
} INFOSTRUCT;

static BOOL CALLBACK TimeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			INFOSTRUCT * i = (INFOSTRUCT*) lp;
			char temp[16];
			if (!i->song && !i->fade) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			if (i->song != ~0)
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if (i->fade != ~0)
			{
				print_time_crap(i->fade, (char*)&temp);
				uSetDlgItemText(wnd, IDC_FADE, (char*)&temp);
			}
		}
		cfg_placement.on_window_creation(wnd);
		return 1;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				INFOSTRUCT * i=(INFOSTRUCT*)uGetWindowLong(wnd,DWL_USER);
				int foo;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_LENGTH));
				if (foo != BORK_TIME) i->song = foo;
				else i->song = ~0;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = ~0;
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
		break;
	case WM_DESTROY:
		cfg_placement.on_window_destruction(wnd);
		break;
	}
	return 0;
}

static bool context_time_dialog(unsigned *song_ms, unsigned *fade_ms)
{
	bool ret;
	INFOSTRUCT * i = new INFOSTRUCT;
	if (!i) return 0;
	i->song = *song_ms;
	i->fade = *fade_ms;
	HWND hwnd = core_api::get_main_window();
	ret = DialogBoxParam(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_TIME), hwnd, TimeProc, (LPARAM)i) > 0;
	if (ret)
	{
		*song_ms = i->song;
		*fade_ms = i->fade;
	}
	delete i;
	return ret;
}

class length_info_filter : public file_info_filter
{
	bool set_length, set_fade;
	unsigned m_length, m_fade;

	const char * m_tag_length;
	const char * m_tag_fade;

	metadb_handle_list m_handles;

public:
	length_info_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list, const char * p_tag_length, const char * p_tag_fade )
	{
		set_length = false;
		set_fade = false;

		m_tag_length = p_tag_length;
		m_tag_fade = p_tag_fade;

		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}

	}

	void length( unsigned p_length )
	{
		set_length = true;
		m_length = p_length;
	}

	void fade( unsigned p_fade )
	{
		set_fade = true;
		m_fade = p_fade;
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			if ( set_length ) p_info.info_set_int( m_tag_length, m_length );
			if ( set_fade ) p_info.info_set_int( m_tag_fade, m_fade );
			return set_length | set_fade;
		}
		else
		{
			return false;
		}
	}
};

class context_spc : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 1; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edit length";
	}

	/*virtual void get_item_default_path(unsigned n, pfc::string_base & out)
	{
		out.reset();
	}*/
	GUID get_parent() {return contextmenu_groups::tagging;}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		if (n) uBugCheck();
		out = "Edits the length of the selected SPC file, or sets the length of all selected SPC files.";
		return true;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		if (n) uBugCheck();
		// {1B41F297-E794-42ac-AC56-0111D99A238E}
		static const GUID guid = 
		{ 0x1b41f297, 0xe794, 0x42ac, { 0xac, 0x56, 0x1, 0x11, 0xd9, 0x9a, 0x23, 0x8e } };
		return guid;
	}

	virtual bool context_get_display( unsigned n, const pfc::list_base_const_t< metadb_handle_ptr > & data, pfc::string_base & out, unsigned & displayflags, const GUID & )
	{
		if (n) uBugCheck();
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			if ( stricmp( pfc::string_extension( foo.get_path() ), "spc" ) ) return false;
		}
		if (i == 1) out = "Edit length";
		else out = "Set length";
		return true;
	}

	virtual void context_command( unsigned n, const pfc::list_base_const_t< metadb_handle_ptr > & data, const GUID& )
	{
		if (n) uBugCheck();
		unsigned tag_song_ms = ~0, tag_fade_ms = ~0;
		unsigned i = data.get_count();
		file_info_impl info;
		abort_callback_impl m_abort;
		if (i == 1)
		{
			// fetch info from single file
			metadb_handle_ptr handle = data.get_item(0);
			handle->metadb_lock();
			const file_info * p_info;
			if (handle->get_info_locked(p_info) && p_info)
			{
				const char *t = p_info->info_get(field_length);
				if (t) tag_song_ms = atoi(t);
				t = p_info->info_get(field_fade);
				if (t) tag_fade_ms = atoi(t);
			}
			handle->metadb_unlock();
		}
		if (!context_time_dialog(&tag_song_ms, &tag_fade_ms)) return;
		static_api_ptr_t<metadb_io_v2> p_imgr;

		service_ptr_t<length_info_filter> p_filter = new service_impl_t< length_info_filter >( data, field_length, field_fade );
		if ( tag_song_ms != ~0 ) p_filter->length( tag_song_ms );
		if ( tag_fade_ms != ~0 ) p_filter->fade( tag_fade_ms );

		p_imgr->update_info_async( data, p_filter, core_api::get_main_window(), 0, 0 );
	}
};

namespace a { DECLARE_FILE_TYPE("SPC files", "*.SPC"); }
namespace b { DECLARE_FILE_TYPE("RSN files", "*.RSN"); }

static input_factory_t           <input_spc>   g_input_spc_factory;
static archive_factory_t         <archive_rsn> g_archive_rsn_factory;
static contextmenu_item_factory_t<context_spc> g_contextmenu_item_spc_factory;
