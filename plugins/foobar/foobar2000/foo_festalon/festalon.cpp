#define MYVERSION "0.4.0"
#define VSTRING "0.5.5"

/*
	changelog

2005-01-01 01:19 UTC - kode54
- Updated to Festalon 0.4.0, apparently the final release :[
- Version is now 0.4.0

2004-04-26 07:58 UTC - kode54
- Changed finite rendering to match the wanted length instead of outputting a multiple of 576 samples
- Version is now 0.3.0.2

2004-02-02 22:36 UTC - kode54
- Fixed context_get_display function declaration in context_fest
- Version is now 0.3.0.1

2004-02-01 10:23 UTC - kode54
- Updated to Festalon 0.3.0
- NOTE: For f'n ICL, NTSC_CPU/PAL_CPU*65536 blah in filter.c had to be hardcoded
  to int64 constants, because the damn compiler clipped the float stuff to INT_MAX
- Added configuration for quality level
- Version is now 0.3.0

2004-01-23 10:58 UTC - kode54
- Fixed bug in field_set macro
- Version is now 0.2.4.3

2003-10-19 03:12 UTC - kode54
- Changed length editor to use query_info_locked to retrieve the existing length from a single track

2003-10-11 07:09 UTC - kode54
- Made playlist editor a little safer
- Updated length editor locking behavior

2003-09-30 14:42 UTC - kode54
- Added helpful console info to set_info()

2003-09-30 13:58 UTC - kode54
- Changed to use reader only when loading or updating the file, allows for...
- Auto-renaming when converting to or from NSFE
- Fixed stupid bug with input (forgot to initialize a variable)
- Version is now 0.2.4.2

2003-09-30 13:07 UTC - kode54
- Oops, forgot to support .nsfe extension

2003-09-30 09:01 UTC - kode54
- Added NSFE playlist editor, using modified CPlaylistDlg and DFC
- Version is now 0.2.4.1

2003-09-30 04:39 UTC - kode54
- Added seeking - INSANELY SLOW!

2003-09-30 04:01 UTC - kode54
- Added extra chip and speed info

2003-09-30 03:21 UTC - kode54
- Added option to ignore NSFE playlists

2003-09-28 15:43 UTC - kode54
- Initial NSFE support
- Length/fade editing
- Updated Festalon to 0.2.4
- Version is now 0.2.4

*/

#include <foobar2000.h>
#include "../helpers/window_placement_helper.h"

#include "resource.h"

extern "C" {
	#define private fest_private
	#define MSVC 1
	#include "driver.h"
	#include "nes/nsf.h"
	#undef private
}

#include <DFC.h>
#include "PlaylistDlg.h"

// Rates supported by Festalon
const int srate_tab[]={44100,48000,96000};

// {A6A4F2CC-FF17-493b-BA00-3B6E99A0FAC5}
static const GUID guid_cfg_srate = 
{ 0xa6a4f2cc, 0xff17, 0x493b, { 0xba, 0x0, 0x3b, 0x6e, 0x99, 0xa0, 0xfa, 0xc5 } };
// {27ED3347-2E24-4fe3-AA06-9C6A92FC3429}
static const GUID guid_cfg_quality = 
{ 0x27ed3347, 0x2e24, 0x4fe3, { 0xaa, 0x6, 0x9c, 0x6a, 0x92, 0xfc, 0x34, 0x29 } };
// {64D3AB91-F954-46f7-85BD-1FAEBB680BE0}
static const GUID guid_cfg_infinite = 
{ 0x64d3ab91, 0xf954, 0x46f7, { 0x85, 0xbd, 0x1f, 0xae, 0xbb, 0x68, 0xb, 0xe0 } };
// {5D4E991B-D394-4298-9782-3D82DBF538E6}
static const GUID guid_cfg_default_length = 
{ 0x5d4e991b, 0xd394, 0x4298, { 0x97, 0x82, 0x3d, 0x82, 0xdb, 0xf5, 0x38, 0xe6 } };
// {765BE0B7-2044-4e1a-AF31-E246EDF6086C}
static const GUID guid_cfg_default_fade = 
{ 0x765be0b7, 0x2044, 0x4e1a, { 0xaf, 0x31, 0xe2, 0x46, 0xed, 0xf6, 0x8, 0x6c } };
// {C552D230-7048-4af0-9B41-381B9E8DB3FD}
static const GUID guid_cfg_placement = 
{ 0xc552d230, 0x7048, 0x4af0, { 0x9b, 0x41, 0x38, 0x1b, 0x9e, 0x8d, 0xb3, 0xfd } };
// {BCBC1B79-E55B-468a-928C-5BBAEE2D76F6}
static const GUID guid_cfg_write = 
{ 0xbcbc1b79, 0xe55b, 0x468a, { 0x92, 0x8c, 0x5b, 0xba, 0xee, 0x2d, 0x76, 0xf6 } };
// {5340C20B-0501-45fd-8D0A-D76F45890354}
static const GUID guid_cfg_write_nsfe = 
{ 0x5340c20b, 0x501, 0x45fd, { 0x8d, 0xa, 0xd7, 0x6f, 0x45, 0x89, 0x3, 0x54 } };
// {C41E2EA0-EEEF-48b1-8871-C52AFF0B9CD5}
static const GUID guid_cfg_nsfe_ignore_playlists = 
{ 0xc41e2ea0, 0xeeef, 0x48b1, { 0x88, 0x71, 0xc5, 0x2a, 0xff, 0xb, 0x9c, 0xd5 } };


// Index to rate in the above table
cfg_int cfg_srate(guid_cfg_srate, 0);

// Quality level labels
static const char * quality_tab[]={"high","higher"};

// Quality level, controls which FIR filter sets Festalon uses for downsampling
cfg_int cfg_quality(guid_cfg_quality, 0);

// Info recycled by the tag writer and manipulated by the editor
static const char field_length[]="nsf_length";
static const char field_fade[]="nsf_fade";

// For show only
static const char field_speed[]="nsf_speed";
static const char field_chips[]="nsf_extra_chips";

// Global fields, supported by both NESM and NSFE
static const char field_artist[]="ARTIST";
static const char field_game[]="ALBUM";
static const char field_copyright[]="COPYRIGHT";

// Supported only by NSFE
static const char field_ripper[]="RIPPER";
static const char field_track[]="TITLE";

// Special field, used only for playlist writing
static const char field_playlist[]="nsf_playlist";

// Do we play forever?
cfg_int cfg_infinite(guid_cfg_infinite,0);

// User defaults for files without lengths, or for negative values in NSFE table
cfg_int cfg_default_length(guid_cfg_default_length, 170000);
cfg_int cfg_default_fade(guid_cfg_default_fade, 10000);

// This remembers the editor window's position if user enables "Remember window position" in player UI settings
static cfg_window_placement cfg_placement(guid_cfg_placement);

// Will input_fest::set_info() function, and will it convert to NSFE (or downgrade to NESM) as necessary?
static cfg_int cfg_write(guid_cfg_write, 0), cfg_write_nsfe(guid_cfg_write_nsfe, 0);

// Ignore NSFE playlists on load? See track_indexer_fest
static cfg_int cfg_nsfe_ignore_playlists(guid_cfg_nsfe_ignore_playlists, 0);

// nsf<->nsfe conversion requires renaming
static void rename_file(const char * src, const char * ext, string_base & out, abort_callback & p_abort)
{
	t_io_result status;
	string8 dst(src);
	const char * start = dst.get_ptr() + dst.scan_filename();
	const char * end = start + strlen(start);
	const char * ptr = end - 1;
	while (ptr > start && *ptr != '.')
	{
		if (*ptr=='?') end = ptr;
		ptr--;
	}

	if (ptr >= start && *ptr == '.')
	{
		string8 temp(ptr + 4);
		dst.truncate(ptr - dst.get_ptr() + 1);
		dst += ext;
		dst += temp;
	}

	for(;;)
	{
		status = filesystem::g_move( src, dst, p_abort );
		if ( io_result_succeeded( status ) )
		{
			list_t< const char * > from, to;
			from.add_item( src );
			to.add_item( dst );
			file_operation_callback::g_on_files_moved( from, to );
			out = dst;
			break;
		}
		else
		{
			string8 msg;
			msg = "Error renaming file: \n";
			msg += file_path_display( src );
			msg += "\nto:\n";
			msg += file_path_display( dst );
			int rv = uMessageBox( core_api::get_main_window(), msg, 0, MB_ICONERROR|MB_ABORTRETRYIGNORE );
			if ( rv == IDABORT )
			{
				throw status;
			}
			else if ( rv == IDRETRY )
			{
				continue;
			}
			else if ( rv == IDIGNORE )
			{
				out = src;
				break;
			}
		}
	}
}

// Simplifies tag writing
#define field_set( f, p ) \
{ \
	ptr = p_info.meta_get( field_##f, 0 ); \
	if ( ptr ) \
	{ \
		string_ansi_from_utf8 foo( ptr ); \
		if ( ! p || strcmp( foo, p ) ) \
		{ \
			int len = foo.length() + 1; \
			if ( p ) free( p ); \
			p = ( char * ) malloc( sizeof( char ) * len ); \
			memcpy( p, foo.get_ptr(), sizeof( char ) * len ); \
		} \
	} \
	else \
	{ \
		free( p ); \
		p = 0; \
	} \
}

// Ugly and disorganized, yes!
class input_fest
{
protected:
	bool no_infinite;

	float * sample_buffer;

	FESTALON * hfest;

	int rate, rendered;

	int song_len,fade_len;
	int tag_song_ms, tag_fade_ms;
	int data_written, discard;

	bool ignoring_playlist;

	string_simple m_path;

	t_filestats m_stats;

public:
	input_fest()
	{
		hfest = NULL;
	}

	~input_fest()
	{
		if (hfest) FESTAI_Close(hfest);
	}

	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write && !cfg_write )
		{
			console::info( "Writing is disabled, see configuration." );
			return io_result_error_data;
		}

		t_io_result status;

		service_ptr_t<file> p_file;

		if ( p_filehint.is_empty() )
		{
			status = filesystem::g_open( p_file, p_path, ( p_reason == input_open_info_write ) ? filesystem::open_mode_write_existing : filesystem::open_mode_read, p_abort );
			if ( io_result_failed( status ) ) return status;
		}
		else p_file = p_filehint;

		status = p_file->get_stats( m_stats, p_abort );
		if ( io_result_failed( status ) ) return status;

		m_path = p_path;

		rate = srate_tab[ cfg_srate ];
		int quality = cfg_quality;

		{
			mem_block_t<BYTE> nsfbuffer;
			unsigned size = (unsigned) p_file->get_size_e(p_abort);
			if ( ! nsfbuffer.set_size( size ) )
				return io_result_error_out_of_memory;
			BYTE * ptr = nsfbuffer.get_ptr();

			try
			{
				p_file->seek_e( 0, p_abort );
				p_file->read_object_e( ptr, size, p_abort );
			}
			catch( t_io_result code )
			{
				return code;
			}

			hfest = FESTAI_Load( ptr, size );
			if (!hfest) return io_result_error_data;
			FESTAI_SetSound( hfest, rate, quality );
			FESTAI_SetVolume( hfest, 100 );
			FESTAI_Disable( hfest, 0 );
		}

		return io_result_success;
	}

	unsigned get_subsong_count()
	{
		ignoring_playlist = ( !!cfg_nsfe_ignore_playlists ) || ( ! hfest->Playlist );
		if ( ! ignoring_playlist )
			return hfest->PlaylistSize;
		else
			return hfest->TotalSongs;
	}

	t_uint32 get_subsong( unsigned p_index )
	{
		if ( ! ignoring_playlist )
		{
			assert( p_index < hfest->PlaylistSize );
			return hfest->Playlist[ p_index ];
		}
		else
		{
			assert( p_index < hfest->TotalSongs );
			return p_index;
		}
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		int tag_song_ms = -1;
		int tag_fade_ms = -1;

		if ( hfest->SongLengths ) tag_song_ms = hfest->SongLengths[ p_subsong ];
		if ( hfest->SongFades ) tag_fade_ms = hfest->SongFades[ p_subsong ];

		if (tag_song_ms < 0) tag_song_ms = cfg_default_length;
		else p_info.info_set_int(field_length, tag_song_ms);
		if (tag_fade_ms < 0) tag_fade_ms = cfg_default_fade;
		else p_info.info_set_int(field_fade, tag_fade_ms);

#define HEADER_STRING(i,n,f) if ((f) && (f)[0]) (i).meta_add((n), string_utf8_from_ansi((f)))

		HEADER_STRING( p_info, field_artist, hfest->Artist );
		HEADER_STRING( p_info, field_game, hfest->GameName );
		HEADER_STRING( p_info, field_copyright, hfest->Copyright );
		HEADER_STRING( p_info, field_ripper, hfest->Ripper );

#undef HEADER_STRING

		if (hfest->SongNames && p_subsong < hfest->TotalSongs &&
			hfest->SongNames[ p_subsong ] && hfest->SongNames[ p_subsong ][ 0 ] )
			p_info.meta_add( field_track, string_utf8_from_ansi( hfest->SongNames[ p_subsong ] ) );

		p_info.set_length( double( tag_song_ms + tag_fade_ms ) * .001 );

		p_info.info_set( field_speed, ( ( hfest->VideoSystem & 3 ) == 1 ) ? "PAL" : "NTSC");

		int soundchip = ((FESTALON_NSF*)(hfest->nsf))->SoundChip;
		if ( soundchip )
		{
			string8 chips;
			static const char * chip[] = { "VRC6", "VRC7", "FDS", "MMC5", "N106", "FME7" };
			for ( int i = 0, mask = 1; i < tabsize( chip ); ++i, mask <<= 1 )
			{
				if ( soundchip & mask )
				{
					if ( chips.length() ) chips.add_byte( '+' );
					chips += chip[ i ];
				}
			}
			if ( chips.length() ) p_info.info_set( field_chips, chips );
		}

		p_info.info_set_int( "samplerate", rate );
		p_info.info_set_int( "channels", hfest->OutChannels );

		return io_result_success;
	}

	t_io_result get_file_stats( t_filestats & p_stats,abort_callback & p_abort )
	{
		p_stats = m_stats;
		return io_result_success;
	}

	t_io_result decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		tag_song_ms = -1;
		tag_fade_ms = -1;

		if ( hfest->SongLengths ) tag_song_ms = hfest->SongLengths[ p_subsong ];
		if ( hfest->SongFades ) tag_fade_ms = hfest->SongFades[ p_subsong ];

		if ( tag_song_ms < 0 ) tag_song_ms = cfg_default_length;
		if ( tag_fade_ms < 0 ) tag_fade_ms = cfg_default_fade;

		FESTAI_SongControl( hfest, p_subsong, 1 );

		data_written=0;
		discard=0;
		rendered=0;

		song_len=MulDiv(tag_song_ms,rate,1000);
		fade_len=MulDiv(tag_fade_ms,rate,1000);

		no_infinite = ( p_flags & input_flag_no_looping ) || !cfg_infinite;

		return io_result_success;
	}

	t_io_result decode_run( audio_chunk & p_chunk,abort_callback & p_abort )
	{
		if ( no_infinite && data_written >= song_len + fade_len ) return io_result_eof;

		t_io_result status = emulate( p_abort );
		if ( io_result_failed( status ) ) return status;

		int d_start, d_end;
		d_start = data_written;
		data_written += rendered;
		d_end = data_written;

		if ( no_infinite && tag_song_ms && d_end > song_len )
		{
			float * foo = sample_buffer;
			for( int n = d_start, i, j = hfest->OutChannels; n < d_end; ++n )
			{
				if ( n > song_len )
				{
					if ( n <= song_len + fade_len )
					{
						float scale = float( song_len + fade_len - n ) / float( fade_len );
						for ( i = 0; i < j; ++i )
						{
							foo[ i ] = foo[ i ] * scale;
						}
					}
				}
				foo += j;
			}
		}

		int32 size;
		if ( no_infinite && d_end >= song_len + fade_len ) size = ( song_len + fade_len - d_start );
		else size = rendered;
		p_chunk.set_data_32( sample_buffer, size, hfest->OutChannels, rate );

		rendered = 0;

		return io_result_success;
	}

	t_io_result decode_seek( double p_seconds, abort_callback & p_abort )
	{
		int pos = int( p_seconds * double( rate ) );
		if ( pos < data_written )
		{
			FESTAI_SongControl( hfest, 0, 0 );
			data_written = 0;
		}
		data_written = discard = pos - data_written;
		t_io_result status = emulate( p_abort );
		if ( io_result_failed( status ) ) return status;
		else return io_result_success;
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta,bool & p_track_change)
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	t_io_result retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		const char * ptr;

		field_set( artist, hfest->Artist );
		field_set( game, hfest->GameName );
		field_set( copyright, hfest->Copyright );

		field_set( ripper, hfest->Ripper );

		ptr = p_info.meta_get( field_track, 0 );

		if (ptr)
		{
			// fun!
			string_ansi_from_utf8 foo(ptr);
			int len = foo.length() + 1;

			if (len > 1)
			{
				if ( ! hfest->SongNames )
				{
					hfest->SongNames = ( char ** ) malloc( sizeof( char * ) * hfest->TotalSongs );
					memset( hfest->SongNames, 0, sizeof( char * ) * hfest->TotalSongs );
				}

				if ( hfest->SongNames[ p_subsong ] ) free( hfest->SongNames[ p_subsong ] );
				hfest->SongNames[ p_subsong ] = ( char * ) malloc( sizeof( char ) * len );
				memcpy( hfest->SongNames[ p_subsong ], foo.get_ptr(), sizeof( char ) * len );
			}
			else ptr = 0;
		}

		if ( ! ptr )
		{
			if ( hfest->SongNames )
			{
				if ( hfest->SongNames[ p_subsong ] )
				{
					free( hfest->SongNames[ p_subsong ] );
					hfest->SongNames[ p_subsong ] = 0;
				}
			}
		}

		int tag_song_ms, tag_fade_ms;

		ptr = p_info.info_get( field_length );
		if ( ptr ) tag_song_ms = atoi( ptr );
		else tag_song_ms = -1;

		ptr = p_info.info_get( field_fade );
		if ( ptr ) tag_fade_ms = atoi( ptr );
		else tag_fade_ms = -1;

		if ( !hfest->SongLengths && tag_song_ms >= 0)
		{
			hfest->SongLengths = ( int32 * ) malloc( sizeof( int32 ) * hfest->TotalSongs );
			if ( ! hfest->SongLengths ) return io_result_error_out_of_memory;
			memset( hfest->SongLengths, -1, sizeof( int32 ) * hfest->TotalSongs );
		}
		if ( hfest->SongLengths )
		{
			hfest->SongLengths[ p_subsong ] = tag_song_ms;
		}

		if ( ! hfest->SongFades && tag_fade_ms >= 0 )
		{
			hfest->SongFades = ( int32 * ) malloc( sizeof( int32 ) * hfest->TotalSongs );
			if ( ! hfest->SongFades ) return io_result_error_out_of_memory;
			memset( hfest->SongFades, -1, sizeof( int32 ) * hfest->TotalSongs );
		}
		if ( hfest->SongFades )
		{
			hfest->SongFades[ p_subsong ] = tag_fade_ms;
		}

		ptr = p_info.info_get(field_playlist);
		if (ptr)
		{
			// buoy, playlist writer!
			if ( hfest->Playlist )
			{
				free( hfest->Playlist );
				hfest->Playlist = 0;
			}
			unsigned count = atoi( ptr );
			if ( count )
			{
				hfest->Playlist = ( uint8 * ) malloc( sizeof( uint8 ) * count );
				for ( unsigned i = 0; i < count; ++i )
				{
					ptr = strchr( ptr, ',' ) + 1;
					hfest->Playlist[ i ] = atoi( ptr );
				}
			}
			hfest->PlaylistSize = count;
		}

		return io_result_success;
	}

	t_io_result retag_commit( abort_callback & p_abort )
	{
		service_ptr_t<file> p_file;
		t_io_result status = filesystem::g_open( p_file, m_path, filesystem::open_mode_write_existing, p_abort );
		if ( io_result_failed( status ) ) return status;

		uint8 sig_test[ 4 ];
		status = p_file->read_object( sig_test, 4, p_abort );
		if ( io_result_failed( status ) ) return status;

		bool bIsExtended = ! memcmp( sig_test, "NSFE", 4 );

		if ( bIsExtended || cfg_write_nsfe )
		{
			if ( ! bIsExtended && hfest->Ripper && hfest->Ripper[ 0 ] ) bIsExtended = true;

			if ( ! bIsExtended && hfest->SongLengths )
			{
				int i;
				for ( i = hfest->TotalSongs; i--; )
				{
					if ( hfest->SongLengths[ i ] >= 0 ) break;
				}
				if ( i >= 0 ) bIsExtended = true;
			}

			if ( ! bIsExtended && hfest->SongFades )
			{
				int i;
				for ( i = hfest->TotalSongs; i--; )
				{
					if ( hfest->SongFades[ i ] >= 0 ) break;
				}
				if ( i >= 0 ) bIsExtended = true;
			}

			if ( ! bIsExtended && hfest->SongNames )
			{
				int i;
				for ( i = hfest->TotalSongs; i--; )
				{
					if ( hfest->SongNames[ i ] && hfest->SongNames[ i ][ 0 ] ) break;
				}
				if ( i >= 0 ) bIsExtended = true;
			}
		}

		try
		{
			string8_fastalloc path;
			path = m_path;
			const char * ext = bIsExtended ? "nsfe" : "nsf";
			if ( stricmp( string_extension_8( m_path ), ext ) )
			{
				string8_fastalloc newname;
				p_file.release();
				rename_file( m_path, ext, newname, p_abort );
				status = filesystem::g_open( p_file, newname, filesystem::open_mode_write_existing, p_abort );
				if ( io_result_failed( status ) ) throw status;
				m_path = newname;
			}

			uint8 * ptr;
			uint32 size;

			if ( bIsExtended ) ptr = FESTAI_CreateNSFE( hfest, & size );
			else               ptr = FESTAI_CreateNSF( hfest, & size );

			if ( ! ptr || ! size ) throw io_result_error_out_of_memory;

			p_file->seek_e(0, p_abort);
			p_file->set_eof_e(p_abort);
			p_file->write_object_e( ptr, size, p_abort );

			free( ptr );

			m_stats = p_file->get_stats_e( p_abort );

			// and now for the core info reload crap
			if ( ! bIsExtended )
			{
				if ( hfest->Ripper )
				{
					free( hfest->Ripper );
					hfest->Ripper = 0;
				}

				if ( hfest->SongLengths )
				{
					free( hfest->SongLengths );
					hfest->SongLengths = 0;
				}

				if ( hfest->SongFades )
				{
					free( hfest->SongFades );
					hfest->SongFades = 0;
				}

				if ( hfest->SongNames )
				{
					for ( int i = hfest->TotalSongs; i--; )
					{
						if ( hfest->SongNames[ i ] ) free ( hfest->SongNames[ i ] );
					}
					free( hfest->SongNames );
					hfest->SongNames = 0;
				}
			}
		}
		catch ( t_io_result code )
		{
			return code;
		}

		return io_result_success;
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return ! strcmp( p_content_type, "audio/x-nsf" );
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "nsf" ) || ! stricmp( p_extension, "nsfe" );
	}

protected:
	t_io_result emulate( abort_callback & p_abort )
	{
		while ( ! rendered && ! p_abort.is_aborting() )
		{
			int count;
			float * buffer = FESTAI_Emulate( hfest, & count );
			if ( discard )
			{
				int meh = count;
				if ( meh > discard ) meh = discard;
				discard -= meh;
				count -= meh;
				if ( ! count ) continue;
				buffer += meh;
			}
			sample_buffer = buffer;
			rendered = count;
		}

		return p_abort.is_aborting() ? io_result_aborted : io_result_success;
	}
};

class input_fest_hes : public input_fest
{
public:
	t_io_result open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) return io_result_error_data;

		return input_fest::open( p_filehint, p_path, p_reason, p_abort );
	}

	t_io_result get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		int tag_song_ms = cfg_default_length;
		int tag_fade_ms = cfg_default_fade;

		p_info.set_length( double( tag_song_ms + tag_fade_ms ) * .001 );

		p_info.info_set_int( "samplerate", rate );
		p_info.info_set_int( "channels", hfest->OutChannels );

		return io_result_success;
	}

	t_io_result retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	t_io_result retag_commit( abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "hes" );
	}
};

#define BORK_TIME 0xC0CAC01A

static unsigned long parse_time_crap(const char *input)
{
	if (!input) return BORK_TIME;
	int len = strlen(input);
	if (!len) return BORK_TIME;
	int value = 0;
	{
		int i;
		for (i = len - 1; i >= 0; i--)
		{
			if ((input[i] < '0' || input[i] > '9') && input[i] != ':' && input[i] != ',' && input[i] != '.')
			{
				return BORK_TIME;
			}
		}
	}
	string8 foo = input;
	char *bar = (char *)foo.get_ptr();
	char *strs = bar + foo.length() - 1;
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	if (*strs == '.' || *strs == ',')
	{
		// fraction of a second
		strs++;
		if (strlen(strs) > 3) strs[3] = 0;
		value = atoi(strs);
		switch (strlen(strs))
		{
		case 1:
			value *= 100;
			break;
		case 2:
			value *= 10;
			break;
		}
		strs--;
		*strs = 0;
		strs--;
	}
	while (strs > bar && (*strs >= '0' && *strs <= '9'))
	{
		strs--;
	}
	// seconds
	if (*strs < '0' || *strs > '9') strs++;
	value += atoi(strs) * 1000;
	if (strs > bar)
	{
		strs--;
		*strs = 0;
		strs--;
		while (strs > bar && (*strs >= '0' && *strs <= '9'))
		{
			strs--;
		}
		if (*strs < '0' || *strs > '9') strs++;
		value += atoi(strs) * 60000;
		if (strs > bar)
		{
			strs--;
			*strs = 0;
			strs--;
			while (strs > bar && (*strs >= '0' && *strs <= '9'))
			{
				strs--;
			}
			value += atoi(strs) * 3600000;
		}
	}
	return value;
}

static void print_time_crap(int ms, char *out)
{
	char frac[8];
	int i,h,m,s;
	if (ms % 1000)
	{
		sprintf(frac, ".%03d", ms % 1000);
		for (i = 3; i > 0; i--)
			if (frac[i] == '0') frac[i] = 0;
		if (!frac[1]) frac[0] = 0;
	}
	else
		frac[0] = 0;
	h = ms / (60*60*1000);
	m = (ms % (60*60*1000)) / (60*1000);
	s = (ms % (60*1000)) / 1000;
	if (h) sprintf(out, "%d:%02d:%02d%s",h,m,s,frac);
	else if (m) sprintf(out, "%d:%02d%s",m,s,frac);
	else sprintf(out, "%d%s",s,frac);
}

class preferences_page_fest : public preferences_page
{
	static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				HWND w;
				int n;
				char temp[16];
				w = GetDlgItem(wnd, IDC_SAMPLERATE);
				for(n=0;n<tabsize(srate_tab);n++)
				{
					wsprintfA(temp,"%u",srate_tab[n]);
					SendMessageA(w,CB_ADDSTRING,0,(long)temp);
				}			
				SendMessageA(w,CB_SETCURSEL,cfg_srate,0);

				w = GetDlgItem(wnd, IDC_QUALITY);
				for(n=0;n<tabsize(quality_tab);n++)
				{
					uSendMessageText(w,CB_ADDSTRING,0,quality_tab[n]);
				}
				SendMessageA(w,CB_SETCURSEL,cfg_quality,0);

				uSendDlgItemMessage(wnd, IDC_INFINITE, BM_SETCHECK, cfg_infinite, 0);
				uSendDlgItemMessage(wnd, IDC_WRITE, BM_SETCHECK, cfg_write, 0);
				uSendDlgItemMessage(wnd, IDC_WNSFE, BM_SETCHECK, cfg_write_nsfe, 0);
				uSendDlgItemMessage(wnd, IDC_NSFEPL, BM_SETCHECK, cfg_nsfe_ignore_playlists, 0);
				print_time_crap(cfg_default_length, (char *)&temp);
				uSetDlgItemText(wnd, IDC_DLENGTH, (char *)&temp);
				print_time_crap(cfg_default_fade, (char *)&temp);
				uSetDlgItemText(wnd, IDC_DFADE, (char *)&temp);
			}
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDC_INFINITE:
				cfg_infinite = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_WRITE:
				cfg_write = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_WNSFE:
				cfg_write_nsfe = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_NSFEPL:
				cfg_nsfe_ignore_playlists = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case (EN_CHANGE<<16)|IDC_DLENGTH:
				{
					int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
					if (meh != BORK_TIME) cfg_default_length = meh;
				}
				break;
			case (EN_KILLFOCUS<<16)|IDC_DLENGTH:
				{
					char temp[16];
					print_time_crap(cfg_default_length, (char *)&temp);
					uSetWindowText((HWND)lp, temp);
				}
				break;
			case (EN_CHANGE<<16)|IDC_DFADE:
				{
					int meh = parse_time_crap(string_utf8_from_window((HWND)lp));
					if (meh != BORK_TIME) cfg_default_fade = meh;
				}
				break;
			case (EN_KILLFOCUS<<16)|IDC_DFADE:
				{
					char temp[16];
					print_time_crap(cfg_default_fade, (char *)&temp);
					uSetWindowText((HWND)lp, temp);
				}
			case (CBN_SELCHANGE<<16)|IDC_SAMPLERATE:
				cfg_srate = SendMessage((HWND)lp,CB_GETCURSEL,0,0);
				break;
			case (CBN_SELCHANGE<<16)|IDC_QUALITY:
				cfg_quality = SendMessage((HWND)lp,CB_GETCURSEL,0,0);
				break;
			}
			break;
		}
		return 0;
	}

public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG,parent,ConfigProc);
	}
	GUID get_guid()
	{
		// {AEE04A0D-071A-4432-B5AD-0EF37141D2B4}
		static const GUID guid = 
		{ 0xaee04a0d, 0x71a, 0x4432, { 0xb5, 0xad, 0xe, 0xf3, 0x71, 0x41, 0xd2, 0xb4 } };
		return guid;
	}
	virtual const char * get_name() {return "Festalon";}
	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_srate = 0;

		cfg_quality = 0;

		cfg_infinite = 0;

		cfg_default_length = 170000;
		cfg_default_fade = 10000;

		cfg_write = 0;
		cfg_write_nsfe = 0;

		cfg_nsfe_ignore_playlists = 0;
	}
};

typedef struct
{
	int song, fade;
} INFOSTRUCT;

static BOOL CALLBACK TimeProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			INFOSTRUCT * i=(INFOSTRUCT*)lp;
			char temp[16];
			if (i->song < 0 && i->fade < 0) uSetWindowText(wnd, "Set length");
			else uSetWindowText(wnd, "Edit length");
			if (i->song >= 0)
			{
				print_time_crap(i->song, (char*)&temp);
				uSetDlgItemText(wnd, IDC_LENGTH, (char*)&temp);
			}
			if (i->fade >= 0)
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
				else i->song = -1;
				foo = parse_time_crap(string_utf8_from_window(wnd, IDC_FADE));
				if (foo != BORK_TIME) i->fade = foo;
				else i->fade = -1;
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

static bool context_time_dialog(int *song_ms, int *fade_ms)
{
	bool ret;
	INFOSTRUCT *i=new INFOSTRUCT;
	if (!i) return 0;
	i->song = *song_ms;
	i->fade = *fade_ms;
	HWND hwnd = core_api::get_main_window();
	ret = uDialogBox(IDD_TIME, hwnd, TimeProc, (long)i) > 0;
	if (ret)
	{
		*song_ms = i->song;
		*fade_ms = i->fade;
	}
	delete i;
	return ret;
}

static bool context_playlist_dialog( FESTALON * pFile, LPCSTR pszTitle )
{
	CPlaylistDlg m_playlist;
	m_playlist.pFile = pFile;
	m_playlist.pszTitle = pszTitle;
	return !!m_playlist.DoModal( core_api::get_my_instance(), core_api::get_main_window(), IDD_PLAYLISTINFO );
}

class context_fest : public menu_item_legacy_context
{
public:
	virtual unsigned get_num_items() { return 2; }

	virtual void get_item_name(unsigned n, string_base & out)
	{
		if (!n) out = "Edit length";
		else out = "Edit playlist";
	}

	virtual void get_item_default_path(unsigned n, string_base & out)
	{
		out = "NSFE";
	}

	virtual bool get_item_description(unsigned n, string_base & out)
	{
		if (!n) out = "Edits the length for the selected track";
		else out = "Edits the playlist(s) for the selected NSFE file(s)";
		return true;
	}

	virtual GUID get_item_guid(unsigned n)
	{
		static const GUID guids[] = {
			{ 0x82059528, 0xcb8b, 0x4521, { 0xab, 0x45, 0x61, 0xc3, 0xf9, 0xc3, 0xb, 0x41 } },
			{ 0x7c454678, 0xc87a, 0x47ae, { 0x8b, 0xd4, 0xe3, 0xbd, 0x78, 0xd2, 0xb5, 0xf0 } }
		};
		assert(n < tabsize(guids));
		return guids[n];
	}

	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned & displayflags,const GUID &)
	{
		if (!cfg_write || !cfg_write_nsfe) return false; // No writing? File doesn't check database for lengths.
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			string_extension_8 ext(foo.get_path());
			if (stricmp(ext, "nsf") && stricmp(ext, "nsfe")) return false;
		}
		if (n) out = "Edit playlist";
		else
		{
			if (i == 1) out = "Edit length";
			else out = "Set length";
		}
		return true;
	}

	virtual void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		unsigned i = data.get_count();
		abort_callback_impl m_abort;
		if (!n)
		{
			int tag_song_ms = -1, tag_fade_ms = -1;
			file_info_impl info;
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
			static_api_ptr_t<metadb_io> p_imgr;
			for (unsigned j = 0; j < i; j++)
			{
				metadb_handle_ptr foo = data.get_item(j);
				foo->metadb_lock();
				foo->get_info(info);
				if (tag_song_ms >= 0) info.info_set_int(field_length, tag_song_ms);
				else info.info_remove(field_length);
				if (tag_fade_ms >= 0) info.info_set_int(field_fade, tag_fade_ms);
				else info.info_remove(field_fade);
				{
					if (tag_song_ms < 0) tag_song_ms = cfg_default_length;
					if (tag_fade_ms < 0) tag_fade_ms = cfg_default_fade;
					double length = (double)(tag_song_ms + tag_fade_ms) * .001;
					info.set_length(length);
				}
				foo->metadb_unlock();
				t_io_result status = p_imgr->update_info(foo, info, core_api::get_main_window(), true);
				if (io_result_failed(status)) break;
			}
		}
		else
		{
			metadb_handle_list files;
			for (unsigned j = 0; j < i; j++)
			{
				metadb_handle_ptr item = data.get_item(j);
				if (files.get_count())
				{
					const char * path = item->get_location().get_path();
					unsigned k = files.get_count();
					unsigned l;
					for (l = 0; l < k; l++)
					{
						if (!stricmp(files.get_item(l)->get_location().get_path(), path)) break;
					}
					if (l < k) continue;
				}
				files.add_item(item);
			}
			i = files.get_count();
			file_info_impl info;
			string8_fastalloc list;
			static_api_ptr_t<metadb_io> p_imgr;

			mem_block_t< t_uint8 > buffer;

			for (unsigned j = 0; j < i; j++)
			{
				unsigned size;
				t_filesize size64;
				metadb_handle_ptr item = files.get_item(j);
				const char * path = item->get_location().get_path();
				service_ptr_t<file> m_file;
				if ( io_result_failed( filesystem::g_open( m_file, path, filesystem::open_mode_read, m_abort ) ) ||
					 io_result_failed( m_file->get_size( size64, m_abort ) ) ||
					 size64 < 1 || size64 > UINT_MAX ) break;

				size = unsigned ( size64 );

				FESTALON * hfest;

				if ( ! buffer.check_size( size ) )
					return;

				if ( io_result_failed( m_file->read_object( buffer.get_ptr(), size, m_abort ) ) ||
					 ! ( hfest = FESTAI_Load( buffer.get_ptr(), size ) ) ) break;

				m_file.release();
				if ( ! context_playlist_dialog( hfest, uStringPrintf( "%s - NSFE playlist editor", (const char *) string_filename( item->get_location().get_path() ) ) ) )
				{
					FESTAI_Close( hfest );
					continue;
				}
				list.reset();
				list.add_int( hfest->PlaylistSize );
				for ( unsigned k = 0; k < hfest->PlaylistSize; ++k )
				{
					list.add_byte(',');
					list.add_int( hfest->Playlist[ k ] );
				}
				FESTAI_Close( hfest );

				item->get_info( info );
				info.info_set( field_playlist, list );
				p_imgr->update_info(item, info, core_api::get_main_window(), true);
				info.info_remove(field_playlist);
				p_imgr->update_info(item, info, core_api::get_main_window(), true);
			}
		}
	}
};

DECLARE_FILE_TYPE("NSF files", "*.NSF;*.NSFE");

static input_factory_t           <input_fest>                   g_input_fest_factory;
static input_factory_t           <input_fest_hes>               g_input_fest_hes_factory;
static preferences_page_factory_t<preferences_page_fest>        g_config_fest_factory;
static menu_item_factory_t       <context_fest>                 g_menu_item_context_fest_factory;

DECLARE_COMPONENT_VERSION("Festalon", MYVERSION, "Plays NSF files.\n\nBased on Festalon " VSTRING "\n\nThis plug-in is licensed under the GNU General\nPublic License. See COPYING.TXT.");
