#include "../SDK/foobar2000.h"
#include "resource.h"

DECLARE_COMPONENT_VERSION ( "APL support", "0.2.4", "Monkey's Audio Image Link file reader/writer" );

static cfg_string cfg_format ( "format", "$num(%tracknumber%,2) - %title%" );

class input_apl : public input {
private:
    string8 file;
    __int64 start_block, finish_block;
    double dur, time_done;
    double start, end;
    input_helper in;

private:
    void parse_string ( const char *src, string8 &out )
    {
        const char *start = src;
        while ( *src && *src != 10 && *src != 13 ) src++;
        out.set_string_n ( start, src-start );
    }

    int apl_parse ( reader *r )
    {
        mem_block_t<char> mem;
        char *ptr;

        {
            int size = (int)r->get_length();
            if ( size < 70 || size > 65536 ) return 0; // 68 bytes in static text fields, don't bother with 64KB+ files
            ptr = mem.set_size ( size+1 );
            if ( !ptr ) return 0;
            if ( r->read (ptr, size) != (unsigned)size ) return 0;
            ptr[size] = 0;
        }

        if ( memcmp (ptr, "[Monkey's Audio Image Link File]", 32) ) return 0;
        ptr += 32;

        file.reset();
        start_block = -1, finish_block = -1;

        while ( *ptr ) {
            while ( *ptr == ' ' || *ptr == 10 || *ptr == 13 ) ptr++;
            char *start = ptr;
            while ( *ptr && *ptr != 10 && *ptr != 13) ptr++;
            char *end = ptr;
            while ( end >= start && (*end == 10 || *end == 13 || *end == ' ') ) end--;

            if ( start == ptr ) continue;

            string8 command, value;
            command.set_string_n ( start, end-start+1 );

            if ( !strncmp (command, "-----", 5) ) break; //----- APE TAG (DO NOT TOUCH!!!) -----

            if ( !strncmp (command, "Image File=", 11) ) {
                parse_string ( command+11, file );
            }
            else if ( !strncmp (command, "Start Block=", 12) ) {
                parse_string ( command+12, value );
                start_block = _atoi64 ( value );
            }
            else if ( !strncmp (command, "Finish Block=", 13) ) {
                parse_string ( command+13, value );
                finish_block = _atoi64 ( value );
            }
        }

        if ( !file.is_empty() && start_block >= 0 ) return 1;

        return 0;
    }

public:
    input_apl() {}
    ~input_apl() {}

    virtual bool test_filename ( const char *fn, const char *ext )
    {
        return !stricmp ( ext, "APL" );
    }

    virtual set_info_t set_info ( reader *r, const file_info *info )
    {
        tag_remover::g_run ( r );
        return tag_writer::g_run (r, info, "ape" ) ? SET_INFO_SUCCESS : SET_INFO_FAILURE;
    }

    virtual bool open ( reader *r, file_info *info, unsigned flags )
    {
        if ( !apl_parse (r) ) return 0;

        string8 filename = info->get_file_path();
        filename.truncate ( filename.scan_filename() );
        string8 filename_ansi = filename;
        filename += file;
        filename_ansi += string_ansi_from_utf8 ( file );

        if ( !file::g_exists (filename) ) {
            filename = file_path_canonical ( file );
            if ( !file::g_exists (filename) ) {
                filename = filename_ansi;
                if ( !file::g_exists (filename) ) {
                    filename = file_path_canonical ( string_ansi_from_utf8 (file) );
                    if ( !file::g_exists (filename) ) return 0;
                }
            }
        }

        int srate = 0;
        file_info_i_full info2 ( (const playlist_entry *)make_playlist_entry (filename, 0) );

        if ( (flags & OPEN_FLAG_DECODE) ) {
            if ( !in.open (&info2) ) return 0;
            const char *s = info2.info_get ( "samplerate" );
            if ( s ) srate = atoi ( s );
            if ( srate <= 0 ) return 0;
            start = (double)start_block / (double)srate;
            end = (double)finish_block / (double)srate;
            if ( end < start ) end = info2.get_length();
            if ( !in.seek (start) ) return 0;
        } else {
            if ( !input::g_get_info (&info2) ) return 0;
            const char *s = info2.info_get ( "samplerate" );
            if ( s ) srate = atoi ( s );
            if ( srate <= 0 ) return 0;
            start = (double)start_block / (double)srate;
            end = (double)finish_block / (double)srate;
            if ( end < start ) end = info2.get_length();
        }

        dur = end - start;

        if ( dur <= 0 ) return 0;

        int i;
        for ( i = 0; i < info2.info_get_count(); i++ ) {
            info->info_set ( info2.info_enum_name(i), info2.info_enum_value(i) );
        }

        for ( i = 0; i < info2.meta_get_count(); i++ ) {
            info->meta_set ( info2.meta_enum_name(i), info2.meta_enum_value(i) );
        }

        tag_reader::g_run ( r, info, "ape" );

		info->info_set ( "referenced_file", file );
        info->set_length ( dur );

        time_done = 0;

        return 1;
    }

    virtual int run ( audio_chunk *chunk )
    {
        if ( time_done >= dur ) return 0;

        int rv = in.run ( chunk );

        if ( rv > 0 ) {
            double delta = chunk->get_duration();

            if ( time_done + delta > dur ) {
                delta = dur - time_done;
                unsigned int samples = (unsigned int)(delta * chunk->get_srate() + 0.5);
                chunk->set_sample_count ( samples );
                time_done = dur;
                if ( samples == 0 ) rv = 0;
            } else time_done += delta;
        } else if ( rv == 0 && time_done == 0 ) rv = -1;

        return rv;
    }

    virtual bool seek ( double s )
    {
        time_done = s;
        return in.seek ( time_done + start );
    }

    virtual void abort()
    {
        in.abort();
    }
};

// -------------------------------------

class contextmenu_apl : public menu_item_context {
private:
    int first_num;
    string8 path;

private:
    int get_path ( const char *str, string8 &out )
    {
        if ( !str ) return 0;
        const char *p = strrchr ( str, '\\' );
        if ( !p ) return 0;
        out.set_string_n ( str, p-str+1 );
        return 1;
    }

    int get_name ( const char *str, string8 &out )
    {
        if ( !str ) return 0;
        const char *p = strrchr ( str, '\\' );
        if ( !p ) p = str; else p++;
        out.set_string ( p );
        return 1;
    }

    int get_path_and_ref ( const file_info *info, string8 &o_path, string8 &o_ref )
    {
        const char *p = info->get_file_path();
        if ( !p ) return 0;

        if ( !get_path (p, o_path) ) return 0;

        const char *ref = info->info_get ( "referenced_file" );
        if ( ref ) {
            o_ref = ref;
        } else {
            get_name ( p, o_ref );
        }

        return 1;
    }

    void get_apl_file ( const char *name, string8 &out )
    {
        out = path;
        out.add_string ( name );
        out.add_string ( ".apl" );
    }

    int write_apl ( const char *ref_file, const char *apl_file, const file_info *info, __int64 sample_offset, __int64 samples )
    {
        reader *r = file::g_open ( apl_file, reader::MODE_WRITE_NEW );
        if ( !r ) return 0;

        string8 data;
        data.add_string ( "[Monkey's Audio Image Link File]" );
        data.add_byte ( 13 ); data.add_byte ( 10 );

        data.add_string ( "Image File=" );
        data.add_string ( ref_file );
        data.add_byte ( 13 ); data.add_byte ( 10 );

        data.add_string ( "Start Block=" );
        data.add_int ( sample_offset );
        data.add_byte ( 13 ); data.add_byte ( 10 );

        data.add_string ( "Finish Block=" );
        data.add_int ( sample_offset + samples );
        data.add_byte ( 13 ); data.add_byte ( 10 );

        data.add_byte ( 13 ); data.add_byte ( 10 );
        data.add_string ( "----- APE TAG (DO NOT TOUCH!!!) -----" );
        data.add_byte ( 13 ); data.add_byte ( 10 );

        int ret = 0;
        if ( (int)r->write (data, data.length()) == (int)data.length() ) {
            if ( tag_writer::g_run (r, info, "ape") ) ret = 1;
        }

        r->reader_release();

        return ret;
    }

public:
    virtual unsigned get_num_items() { return 1; }

    virtual void enum_item ( unsigned n , string_base & out )
    {
        if ( n == 0 ) out = "Write APL link file(s)";
    }

    virtual bool context_get_display ( unsigned n, const ptr_list_base<metadb_handle> &data, string_base &out, unsigned & displayflags, const GUID & caller)
    {
        int sel_count = data.get_count();
        if ( sel_count <= 1 || sel_count > 99 ) return false;

        int count = 0;

        {
            first_num = (1<<30);

            for ( int i = 0; i < sel_count; i++ ) {
                metadb_handle *ptr = data.get_item ( i );
                int subsong = ptr->handle_get_location()->get_number();
                if ( first_num == subsong || subsong < 0 ) return false;
                if ( subsong < first_num ) first_num = subsong;
            }
        }

        if ( first_num < 0 || first_num > 1 ) return false;

        while ( count < sel_count ) {
            bool found = false;

            for ( int i = 0; i < sel_count; i++ ) {
                metadb_handle *ptr = data.get_item ( i );
                int subsong = ptr->handle_get_location()->get_number();

                if ( subsong == count + first_num ) {
                    found = true;
                    break;
                }
            }

            if ( !found ) return false;

            count++;
        }

        out.set_string ( "Write APL link file" );
        if ( count > 1 ) out.add_byte ( 's' );

        return true;
    }

    virtual void context_command ( unsigned n, const ptr_list_base<metadb_handle> &data, const GUID & caller )
    {
        const int count = data.get_count();
        __int64 sample_offset = 0;
        string8 format = cfg_format;

        path.reset();

        for ( int j = 0; j < count; j++ ) {
            for ( int i = 0; i < count; i++ ) {
                metadb_handle *ptr = data.get_item ( i );
                if ( ptr->handle_get_location()->get_number() != j + first_num ) continue;

                int srate = 44100;
                int index = 0;
                double dur = 0;
                string8 name, ref_file, apl;

                bool error = false;
                ptr->handle_lock();

                const file_info *info = ptr->handle_query_locked();
                if ( info ) {
                    if ( !get_path_and_ref (info, path, ref_file) ) error = true;
                    ptr->handle_format_title ( name, format, 0 );
                    name.fix_filename_chars();
                    const char *p = info->info_get ( "samplerate" );
                    if ( p ) srate = atoi ( p );
                    p = info->info_get ( "index" );
                    if ( p ) index = atoi ( p );
                    dur = info->get_length();
                } else error = true;

                if ( error ) {
                    ptr->handle_unlock();
                    console::error ( "Failed to get file infos" );
                    return;
                }

                if ( j == 0 && index > 0 ) sample_offset += index; // lead-in

                __int64 samples = (__int64)(dur * srate + 0.5);

                get_apl_file ( name, apl );

                if ( !write_apl (ref_file, apl, info, sample_offset, samples) ) {
                    ptr->handle_unlock();
                    console::error ( string_printf ("Failed to write '%s'", (const char *)apl) );
                    return;
                } else {
                    console::info ( string_printf ("APL file '%s' created", (const char *)apl) );
                }

                ptr->handle_unlock();

                sample_offset += samples;
            }
        }
    }
};

// -------------------------------------

class config_apl : public config {
    static BOOL CALLBACK ConfigProc ( HWND wnd, UINT msg, WPARAM wp, LPARAM lp )
    {
        string8 temp;

        switch ( msg ) {
        case WM_INITDIALOG:
            uSetDlgItemText ( wnd, IDC_FORMAT, cfg_format );
            return TRUE;

        case WM_COMMAND:
            switch ( wp ) {
            case IDC_FORMAT | (EN_KILLFOCUS<<16):
				uGetWindowText ( (HWND)lp, temp );
				cfg_format = temp;
                break;
            }
            break;
        }

        return FALSE;
    }

    virtual HWND create ( HWND parent )
    {
        return uCreateDialog ( service_factory_base::get_my_instance(), (UINT)MAKEINTRESOURCE(IDD_CONFIG), parent, ConfigProc );
    }

    virtual const char *get_name() { return "APL support"; }

    virtual const char *get_parent_name() { return "Components"; }
};

static service_factory_t<input, input_apl> foo_apl;
static service_factory_single_t<menu_item, contextmenu_apl> foo_apl_context;
static service_factory_single_t<config, config_apl> foo_apl_cfg;
