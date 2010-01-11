#include "base.h"
#include "reader.h"
#include "config.h"

#include "NSF_File.h"
#include <DFC.h>
#include "PlaylistDlg.h"

#include "../helpers/window_placement_helper.h"

#include "resource.h"

#include <gme/Nsf_Emu.h>

// Borrowed from NSF_File.cpp
#define SAFE_DELETE(p) { if(p){ delete[] p; p = NULL; } }
#define SAFE_NEW(p,t,s) p = new t[s]; ZeroMemory(p,sizeof(t) * s)

// Info recycled by the tag writer and manipulated by the editor
static const char field_length[]="nsf_length";
static const char field_fade[]="nsf_fade";

// For show only
static const char field_speed[]="nsf_speed";
static const char field_chips[]="nsf_extra_chips";

// Global fields, supported by both NESM and NSFE
static const char field_artist[]="artist";
static const char field_game[]="album";
static const char field_copyright[]="copyright";

// Supported only by NSFE
static const char field_ripper[]="ripper";
static const char field_track[]="title";

// Special field, used only for playlist writing
static const char field_playlist[]="nsf_playlist";

// {6EE94DA8-6A42-46cc-8948-5B1A2859CA4F}
static const GUID guid_cfg_placement = 
{ 0x6ee94da8, 0x6a42, 0x46cc, { 0x89, 0x48, 0x5b, 0x1a, 0x28, 0x59, 0xca, 0x4f } };

// This remembers the editor window's position if user enables "Remember window position" in player UI settings
static cfg_window_placement cfg_placement(guid_cfg_placement);

#undef HEADER_STRING
#define HEADER_STRING(i,n,f) if ((f) && (f)[0]) (i).meta_add((n), pfc::stringcvt::string_utf8_from_codepage(pfc::stringcvt::codepage_system, (f)))

static void rename_file(const char * src, const char * ext, pfc::string_base & out, abort_callback & p_abort)
{
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
		try
		{
			filesystem::g_move(src, dst, p_abort);

			list_t<const char *> from, to;
			from.add_item(src);
			to.add_item(dst);
			file_operation_callback::g_on_files_moved(from, to);
			out = dst;
			break;
		}
		catch ( const exception_io & )
		{
			string8 msg;
			msg = "Error renaming file: \n";
			msg += file_path_display(src);
			msg += "\nto:\n";
			msg += file_path_display(dst);
			int rv = uMessageBox(core_api::get_main_window(),msg,0,MB_ICONERROR|MB_ABORTRETRYIGNORE);
			if (rv==IDABORT) throw;
			else if (rv==IDRETRY) continue;
			else if (rv==IDIGNORE)
			{
				out = src;
				break;
			}
		}
	}
}

#define field_set(f,p) \
{ \
	ptr = p_info.meta_get(field_##f, 0); \
	if (ptr) \
	{ \
		pfc::stringcvt::string_codepage_from_utf8 foo(pfc::stringcvt::codepage_system, ptr); \
		if (!p || strcmp(foo, p)) \
		{ \
			int len = foo.length() + 1; \
			SAFE_DELETE(p); \
			SAFE_NEW(p,char,len); \
			memcpy(p,foo.get_ptr(),len); \
		} \
	} \
	else \
	{ \
		SAFE_DELETE(p); \
	} \
}

class input_nsf : public input_gep
{
	CNSFFile nsf;

public:
	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "nsf" ) || ! stricmp( p_extension, "nsfe" );
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write && ! cfg_write )
		{
			console::print("Writing is disabled, see configuration.");
			throw exception_io_data();
		}

		input_gep::open( p_filehint, p_path, p_reason, p_abort );

		nsf.LoadFile(m_file, true, p_abort);

		if ( p_reason != input_open_info_write ) m_file.release();
	}

	unsigned get_subsong_count()
	{
		return nsf.nTrackCount;
	}

	void get_info( t_uint32 p_subsong, file_info & p_info, abort_callback & p_abort )
	{
		HEADER_STRING(p_info, field_artist, nsf.szArtist);
		HEADER_STRING(p_info, field_game, nsf.szGameTitle);
		HEADER_STRING(p_info, field_copyright, nsf.szCopyright);
		HEADER_STRING(p_info, field_ripper, nsf.szRipper);

		if (nsf.szTrackLabels && p_subsong < nsf.nTrackCount && nsf.szTrackLabels[p_subsong] &&
			nsf.szTrackLabels[p_subsong][0]) p_info.meta_add(field_track, pfc::stringcvt::string_utf8_from_codepage(pfc::stringcvt::codepage_system, nsf.szTrackLabels[p_subsong]));

		tag_song_ms = -1;
		tag_fade_ms = -1;

		if (nsf.pTrackTime) tag_song_ms = nsf.pTrackTime[p_subsong];
		if (nsf.pTrackFade) tag_fade_ms = nsf.pTrackFade[p_subsong];

		if (tag_song_ms < 0) tag_song_ms = cfg_default_length;
		else p_info.info_set_int(field_length, tag_song_ms);
		if (tag_fade_ms < 0) tag_fade_ms = cfg_default_fade;
		else p_info.info_set_int(field_fade, tag_fade_ms);

		p_info.set_length((double)(tag_song_ms + tag_fade_ms) * .001);

		p_info.info_set(field_speed, nsf.nIsPal ? "PAL" : "NTSC");

		{
			string8 chips;
			static const char * chip[] = {"VRC6", "VRC7", "FDS", "MMC5", "N106", "FME7"};
			for (int i = 0, mask = 1; i < tabsize(chip); i++, mask <<= 1)
			{
				if (nsf.nChipExtensions & mask)
				{
					if (chips.length()) chips.add_byte('+');
					chips += chip[i];
				}
			}
			if (chips.length()) p_info.info_set(field_chips, chips);
		}

		p_info.info_set_int("samplerate", sample_rate);
		p_info.info_set_int("channels", 1);
		p_info.info_set_int("bitspersample", 16);
	}

	void decode_initialize( t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort )
	{
		Nsf_Emu * emu = ( Nsf_Emu * ) this->emu;
		if ( ! emu )
		{
			filesystem::g_open_tempmem(m_file, p_abort);

			bool extended = nsf.bIsExtended;
			nsf.bIsExtended = false;
			nsf.SaveFile(m_file, p_abort);
			nsf.bIsExtended = extended;

			try
			{
				m_file->seek(0, p_abort);

				foobar_File_Reader rdr(m_file, p_abort);
				Nsf_Emu::header_t header;

				ERRCHK( rdr.read( &header, sizeof(header) ) );

				this->emu = emu = new Nsf_Emu;

				ERRCHK( emu->set_sample_rate( sample_rate ) );
				ERRCHK( emu->load( header, rdr ) );
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
		}

		tag_song_ms = -1;
		tag_fade_ms = -1;
		if (nsf.pTrackTime) tag_song_ms = nsf.pTrackTime[p_subsong];
		if (nsf.pTrackFade) tag_fade_ms = nsf.pTrackFade[p_subsong];
		if (tag_song_ms < 0) tag_song_ms = cfg_default_length;
		if (tag_fade_ms < 0) tag_fade_ms = cfg_default_fade;

		input_gep::decode_initialize( p_subsong, p_flags, p_abort );
	}

	void retag_set_info( t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort )
	{
		const char * ptr;

		{
			field_set( artist, nsf.szArtist );
			field_set( game, nsf.szGameTitle );
			field_set( copyright, nsf.szCopyright );

			field_set( ripper, nsf.szRipper );

			ptr = p_info.meta_get( field_track, 0 );

			if ( ptr )
			{
				// fun!
				pfc::stringcvt::string_ansi_from_utf8 foo( ptr );
				int len = foo.length() + 1;

				if ( len > 1 )
				{
					if ( ! nsf.szTrackLabels )
					{
						SAFE_NEW( nsf.szTrackLabels, char*, nsf.nTrackCount );
						for ( unsigned i = 0; i < nsf.nTrackCount; ++i )
						{
							if ( i == p_subsong ) continue;
							SAFE_NEW( nsf.szTrackLabels[i], char, 1 );
						}
					}

					SAFE_DELETE( nsf.szTrackLabels[ p_subsong ] );
					SAFE_NEW( nsf.szTrackLabels[ p_subsong ], char, len );
					memcpy( nsf.szTrackLabels[ p_subsong ], foo.get_ptr(), len );
					nsf.bIsExtended = true;
				}
			}
			else
			{
				if (nsf.szTrackLabels)
				{
					SAFE_DELETE(nsf.szTrackLabels[p_subsong]);
					unsigned i;
					for (i = 0; i < nsf.nTrackCount; i++)
					{
						if (!nsf.szTrackLabels[i]) continue;
						if (strlen(nsf.szTrackLabels[i])) break;
					}
					if (i < nsf.nTrackCount) nsf.bIsExtended = true;
				}
			}

			int tag_song_ms, tag_fade_ms;
			ptr = p_info.info_get(field_length);
			if (ptr) tag_song_ms = atoi(ptr);
			else tag_song_ms = -1;

			ptr = p_info.info_get(field_fade);
			if (ptr) tag_fade_ms = atoi(ptr);
			else tag_fade_ms = -1;

			if (!nsf.pTrackTime && tag_song_ms >= 0)
			{
				nsf.pTrackTime = new int[nsf.nTrackCount];
				memset(nsf.pTrackTime, -1, nsf.nTrackCount * 4);
			}
			if (nsf.pTrackTime)
			{
				nsf.pTrackTime[p_subsong] = tag_song_ms;
				unsigned i;
				for (i = 0; i < nsf.nTrackCount; i++)
				{
					if (nsf.pTrackTime[i] >= 0) break;
				}
				if (i < nsf.nTrackCount) nsf.bIsExtended = true;
				else SAFE_DELETE(nsf.pTrackTime);
			}

			if (!nsf.pTrackFade && tag_fade_ms >= 0)
			{
				nsf.pTrackFade = new int[nsf.nTrackCount];
				memset(nsf.pTrackFade, -1, nsf.nTrackCount * 4);
			}
			if (nsf.pTrackFade)
			{
				nsf.pTrackFade[p_subsong] = tag_fade_ms;
				unsigned i;
				for (i = 0; i < nsf.nTrackCount; i++)
				{
					if (nsf.pTrackFade[i] >= 0) break;
				}
				if (i < nsf.nTrackCount) nsf.bIsExtended = true;
				else SAFE_DELETE(nsf.pTrackFade);
			}

			ptr = p_info.info_get(field_playlist);
			if (ptr)
			{
				// buoy, playlist writer!
				SAFE_DELETE(nsf.pPlaylist);
				unsigned count = atoi(ptr);
				if (count)
				{
					ptr = strchr(ptr, ',') + 1;
					SAFE_NEW(nsf.pPlaylist,BYTE,count);
					for (unsigned i = 0; i < count; i++)
					{
						nsf.pPlaylist[i] = atoi(ptr);
						ptr = strchr(ptr, ',') + 1;
					}
				}
				nsf.nPlaylistSize = count;
			}
		}
	}

	void retag_commit( abort_callback & p_abort )
	{
		if ( !nsf.bIsExtended && cfg_write_nsfe )
		{
			if ( ! nsf.bIsExtended && nsf.szRipper && nsf.szRipper[0] ) nsf.bIsExtended = true;

			if ( ! nsf.bIsExtended && nsf.pTrackTime )
			{
				int i;
				for ( i = nsf.nTrackCount; i--; )
				{
					if ( nsf.pTrackTime[i] >= 0 ) break;
				}
				if ( i >= 0 ) nsf.bIsExtended = true;
			}

			if ( ! nsf.bIsExtended && nsf.pTrackFade )
			{
				int i;
				for ( i = nsf.nTrackCount; i--; )
				{
					if ( nsf.pTrackFade[i] >= 0 ) break;
				}
				if ( i >= 0 ) nsf.bIsExtended = true;
			}

			if ( ! nsf.bIsExtended && nsf.szTrackLabels )
			{
				int i;
				for ( i = nsf.nTrackCount; i--; )
				{
					if ( nsf.szTrackLabels[ i ] && nsf.szTrackLabels[ i ][ 0 ] ) break;
				}
				if ( i >= 0 ) nsf.bIsExtended = true;
			}

			if ( ! nsf.bIsExtended && nsf.pPlaylist && nsf.nPlaylistSize ) nsf.bIsExtended = true;
		}

		//try
		{
			m_file->seek( 0, p_abort );
			m_file->set_eof( p_abort );

			nsf.SaveFile( m_file, p_abort );

			m_stats = m_file->get_stats( p_abort );

			m_file.release();

			string8_fastalloc path;
			path = m_path;
			const char * ext = nsf.bIsExtended ? "nsfe" : "nsf";
			if ( stricmp( string_extension( m_path ), ext ) )
			{
				string8_fastalloc newname;
				rename_file( m_path, ext, newname, p_abort );
				m_path = newname;
			}

			if ( ! nsf.bIsExtended )
			{
				SAFE_DELETE( nsf.szRipper );
				SAFE_DELETE( nsf.pTrackTime );
				SAFE_DELETE( nsf.pTrackFade );

				if ( nsf.szTrackLabels )
				{
					for ( int i = nsf.nTrackCount; i--; )
					{
						SAFE_DELETE( nsf.szTrackLabels[ i ] );
					}
					SAFE_DELETE( nsf.szTrackLabels );
				}
			}
		}
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

static bool context_playlist_dialog(CNSFFile * pFile, LPCSTR pszTitle)
{
	CPlaylistDlg m_playlist;
	m_playlist.pFile = pFile;
	m_playlist.pszTitle = pszTitle;
	return !!m_playlist.DoModal(core_api::get_my_instance(),core_api::get_main_window(),IDD_PLAYLISTINFO);
}

class context_nsf : public menu_item_legacy_context
{
public:
	virtual unsigned get_num_items() { return 2; }

	virtual void get_item_name( unsigned n, pfc::string_base & out )
	{
		if (!n) out = "Edit length";
		else out = "Edit playlist";
	}

	virtual void get_item_default_path( unsigned n, pfc::string_base & out )
	{
		out = "NSFE";
	}

	virtual bool get_item_description( unsigned n, pfc::string_base & out )
	{
		if (!n) out = "Edits the length for the selected track";
		else out = "Edits the playlist(s) for the selected NSFE file(s)";
		return true;
	}

	virtual GUID get_item_guid( unsigned n )
	{
		static const GUID guids[] = {
			{ 0x82059528, 0xcb8b, 0x4521, { 0xab, 0x45, 0x61, 0xc3, 0xf9, 0xc3, 0xb, 0x41 } },
			{ 0x7c454678, 0xc87a, 0x47ae, { 0x8b, 0xd4, 0xe3, 0xbd, 0x78, 0xd2, 0xb5, 0xf0 } }
		};
		assert(n < tabsize(guids));
		return guids[n];
	}

	virtual bool context_get_display( unsigned n, const list_base_const_t<metadb_handle_ptr> & data, pfc::string_base & out,unsigned & displayflags, const GUID & )
	{
		if (!cfg_write || !cfg_write_nsfe) return false; // No writing? File doesn't check database for lengths.
		unsigned i, j;
		i = data.get_count();
		for (j = 0; j < i; j++)
		{
			const playable_location & foo = data.get_item(j)->get_location();
			string_extension ext(foo.get_path());
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

	virtual void context_command( unsigned n, const list_base_const_t<metadb_handle_ptr> & data, const GUID & )
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
				//foo->metadb_lock();
				if (foo->get_info(info))
				{
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
					if ( metadb_io::update_info_success != p_imgr->update_info( foo, info, core_api::get_main_window(), true ) ) j = i;
				}
				else j = i;
				//foo->metadb_unlock();
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
			for (unsigned j = 0; j < i; j++)
			{
				CNSFFile nsf;
				metadb_handle_ptr item = files.get_item(j);
				const char * path = item->get_location().get_path();
				try
				{
					service_ptr_t<file> m_file;
					filesystem::g_open(m_file, path, filesystem::open_mode_read, m_abort);
					nsf.LoadFile(m_file, false, m_abort);
				}
				catch ( ... )
				{
					break;
				}
				if ( ! context_playlist_dialog( &nsf, string8() << string_filename( item->get_location().get_path() ) << " - NSFE playlist editor" ) ) continue;
				list.reset();
				list << nsf.nPlaylistSize;
				for (unsigned k = 0; k < nsf.nPlaylistSize; k++)
				{
					list.add_byte(',');
					list << format_int( nsf.pPlaylist[k] );
				}
				item->metadb_lock();
				if (item->get_info(info))
				{
					info.info_set(field_playlist, list);
					// aww fuck it
					if ( metadb_io::update_info_success == p_imgr->update_info( item, info, core_api::get_main_window(), true ) )
					{
						info.info_remove( field_playlist );
						if ( metadb_io::update_info_success != p_imgr->update_info( item, info, core_api::get_main_window(), true ) ) j = i;
					}
					else j = i;
				}
				item->metadb_unlock();
			}
		}
	}
};

DECLARE_FILE_TYPE("NSF files", "*.NSF;*.NSFE");

static input_factory_t        <input_nsf>   g_input_nsf_factory;
static menu_item_factory_t    <context_nsf> g_menu_item_context_nsf_factory;
