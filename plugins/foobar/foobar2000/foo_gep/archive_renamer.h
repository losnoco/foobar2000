#ifndef _ARCHIVE_RENAMER_H_
#define _ARCHIVE_RENAMER_H_

#include <foobar2000.h>

#define archive_renamed_factory(src_ext, impl_ext) \
	namespace src_ext##impl_ext { \
		extern char const ext_##src_ext[] = #src_ext; \
		extern char const ext_##impl_ext[] = #impl_ext; \
		static archive_factory_t<archive_renamed<ext_##src_ext, ext_##impl_ext>> g_archive_renamed_factory; \
	}

template <const char * src_ext, const char * impl_ext>
class archive_renamed : public archive_impl
{
	class archive_renamed_callback : public archive_callback
	{
		archive_callback & m_callback;
		pfc::string8       m_extension;

	public:
		archive_renamed_callback( archive_callback & p_callback, const char * p_extension ) : m_callback( p_callback ), m_extension( p_extension ) { }

		virtual bool on_entry( archive * owner, const char * url, const t_filestats & p_stats, const service_ptr_t<file> & p_reader )
		{
			pfc::string8 m_url, archive, file;
			archive_impl::g_parse_unpack_path( url, archive, file );
			archive_impl::g_make_unpack_path( m_url, pfc::string_replace_extension( archive, m_extension ), file, impl_ext );
			return m_callback.on_entry( owner, m_url, p_stats, p_reader );
		}

		bool is_aborting() const {return m_callback.is_aborting();}
		abort_callback_event get_abort_event() const {return m_callback.get_abort_event();}
	};

	static bool g_query_renamed_service( service_ptr_t<archive> & p_arch, pfc::string8 & p_path, const char * p_archive, const char * p_file )
	{
		g_make_unpack_path( p_path, p_archive, p_file, impl_ext );

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
		return src_ext;
	}

	virtual t_filestats get_stats_in_archive( const char * p_archive, const char * p_file, abort_callback & p_abort )
	{
#if 0
		pfc::string8 m_path;
		service_ptr_t< archive > m_arch;
		if (g_query_renamed_service( m_arch, m_path, p_archive, p_file ))
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
		if (g_query_renamed_service( m_arch, m_path, p_archive, p_file ))
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
		if ( stricmp_utf8( p_extension, src_ext ) )
			throw exception_io_unsupported_format();

		pfc::string8 m_path;
		service_ptr_t< archive > m_arch;
		if (g_query_renamed_service( m_arch, m_path, path, src_ext ))
		{
			service_ptr_t< file > m_file = p_reader;
			if ( m_file.is_empty() )
				filesystem::g_open( m_file, path, filesystem::open_mode_read, p_out );

			archive_renamed_callback m_out( p_out, p_extension );

			m_arch->archive_list( pfc::string_replace_extension(path, impl_ext), m_file, m_out, p_want_readers );
		}
		else throw exception_io_unsupported_format();
	}
};

#endif
