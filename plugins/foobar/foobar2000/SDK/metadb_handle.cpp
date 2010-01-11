#include "foobar2000.h"


double metadb_handle::get_length()
{
	double rv = 0;
	metadb_lock();
	const file_info * info;
	if (get_info_locked(info))
		rv = info->get_length();
	metadb_unlock();
	return rv;
}


int metadb_handle::format_title_legacy(string_base & out,const char * spec,const char * extra_items)
{
	if (extra_items) return format_title(&titleformat_hook_impl_legacy_extrainfos(extra_items),out,spec,0);
	else return format_title(0,out,spec,0);
}

t_filetimestamp metadb_handle::get_filetimestamp()
{
	return get_filestats().m_timestamp;
}

t_filesize metadb_handle::get_filesize()
{
	return get_filestats().m_size;
}

int metadb_handle::format_title(titleformat_hook * p_hook,string_base & p_out,const char * p_spec,titleformat_text_filter * p_filter)
{
	service_ptr_t<titleformat_object> script;
	if (static_api_ptr_t<titleformat>()->compile(script,p_spec))
		return format_title(p_hook,p_out,script,p_filter);
	else
	{
		p_out.reset();
		return 0;
	}
}



bool metadb_handle::g_should_reload(const t_filestats & p_old_stats,const t_filestats & p_new_stats,bool p_fresh)
{
	if (p_new_stats.m_timestamp == filetimestamp_invalid) return p_fresh;
	else if (p_fresh) return p_old_stats!= p_new_stats;
	else return p_old_stats.m_timestamp < p_new_stats.m_timestamp;
}

bool metadb_handle::should_reload(const t_filestats & p_new_stats, bool p_fresh) const
{
	if (!is_info_loaded_async()) return true;
	else return g_should_reload(get_filestats(),p_new_stats,p_fresh);
}