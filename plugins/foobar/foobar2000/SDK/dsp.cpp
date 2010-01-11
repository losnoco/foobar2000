#include "foobar2000.h"
#include <math.h>

unsigned dsp_chunk_list_i::get_count() const {return data.get_count();}

audio_chunk * dsp_chunk_list_i::get_item(unsigned n) const {return n>=0 && n<(unsigned)data.get_count() ? data[n] : 0;}

void dsp_chunk_list_i::remove_by_idx(unsigned idx)
{
	if (idx>=0 && idx<(unsigned)data.get_count())
		recycled.add_item(data.remove_by_idx(idx));
}

void dsp_chunk_list_i::remove_mask(const bit_array & mask)
{
	unsigned n, m = data.get_count();
	for(n=0;n<m;n++)
		if (mask[m])
			recycled.add_item(data[n]);
	data.remove_mask(mask);
}

audio_chunk * dsp_chunk_list_i::insert_item(unsigned idx,unsigned hint_size)
{
	unsigned max = get_count();
	if (idx<0) idx=0;
	else if (idx>max) idx = max;
	audio_chunk_i * ret = 0;
	if (recycled.get_count()>0)
	{
		unsigned best;
		if (hint_size>0)
		{
			best = 0;
			unsigned best_found = recycled[0]->get_data_size(), n, total = recycled.get_count();
			for(n=1;n<total;n++)
			{
				if (best_found==hint_size) break;
				unsigned size = recycled[n]->get_data_size();
				int delta_old = abs((int)best_found - (int)hint_size), delta_new = abs((int)size - (int)hint_size);
				if (delta_new < delta_old)
				{
					best_found = size;
					best = n;
				}
			}
		}
		else best = recycled.get_count()-1;

		ret = recycled.remove_by_idx(best);
		ret->set_sample_count(0);
		ret->set_channels(0);
		ret->set_srate(0);
	}
	else ret = new audio_chunk_i;
	if (idx==max) data.add_item(ret);
	else data.insert_item(ret,idx);
	return ret;
}

dsp_chunk_list_i::~dsp_chunk_list_i() {data.delete_all();recycled.delete_all();}

void dsp_chunk_list::remove_bad_chunks()
{
	bool blah = false;
	unsigned idx;
	for(idx=0;idx<get_count();)
	{
		audio_chunk * chunk = get_item(idx);
		if (!chunk->is_valid())
		{
			chunk->reset();
			remove_by_idx(idx);
			blah = true;
		}
		else idx++;
	}
	if (blah) console::info("one or more bad chunks removed from dsp chunk list");
}


bool dsp_entry::g_instantiate(service_ptr_t<dsp> & p_out,const dsp_preset & p_preset)
{
	service_ptr_t<dsp_entry> ptr;
	if (!g_get_interface(ptr,p_preset.get_owner())) return false;
	return ptr->instantiate(p_out,p_preset);
}

bool dsp_entry::g_instantiate_default(service_ptr_t<dsp> & p_out,const GUID & p_guid)
{
	service_ptr_t<dsp_entry> ptr;
	if (!g_get_interface(ptr,p_guid)) return false;
	dsp_preset_impl preset;
	if (!ptr->get_default_preset(preset)) return false;
	return ptr->instantiate(p_out,preset);
}

bool dsp_entry::g_name_from_guid(string_base & p_out,const GUID & p_guid)
{
	service_ptr_t<dsp_entry> ptr;
	if (!g_get_interface(ptr,p_guid)) return false;
	ptr->get_name(p_out);
	return true;
}

bool dsp_entry::g_dsp_exists(const GUID & p_guid)
{
	service_ptr_t<dsp_entry> blah;
	return g_get_interface(blah,p_guid);
}

bool dsp_entry::g_get_default_preset(dsp_preset & p_out,const GUID & p_guid)
{
	service_ptr_t<dsp_entry> ptr;
	if (!g_get_interface(ptr,p_guid)) return false;
	return ptr->get_default_preset(p_out);
}

t_io_result dsp_chain_config::contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const
{
	t_io_result status;
	unsigned n, count = get_count();

	status = p_stream->write_lendian_t(count,p_abort);
	if (io_result_failed(status)) return status;

	for(n=0;n<count;n++)
	{
		status = get_item(n).contents_to_stream(p_stream,p_abort);
		if (io_result_failed(status)) return status;
	}
	return io_result_success;
}

t_io_result dsp_chain_config::contents_from_stream(stream_reader * p_stream,abort_callback & p_abort)
{
	
	t_io_result status;
	unsigned long n,count;

	remove_all();

	status = p_stream->read_lendian_t(count,p_abort);
	if (io_result_failed(status)) return status;

	dsp_preset_impl temp;

	for(n=0;n<count;n++)
	{
		status = temp.contents_from_stream(p_stream,p_abort);
		if (io_result_failed(status)) return status;
		add_item(temp);
	}
	return io_result_success;
}


bool cfg_dsp_chain_config::get_data(dsp_chain_config & p_data) const
{
	p_data.copy(m_data);
	return true;
}

void cfg_dsp_chain_config::set_data(const dsp_chain_config & p_data)
{
	m_data.copy(p_data);
}

void cfg_dsp_chain_config::reset()
{
	m_data.remove_all();
}

t_io_result cfg_dsp_chain_config::get_data_raw(stream_writer * p_stream,abort_callback & p_abort)
{
	return m_data.contents_to_stream(p_stream,p_abort);
}

t_io_result cfg_dsp_chain_config::set_data_raw(stream_reader * p_stream,unsigned,abort_callback & p_abort)
{
	return m_data.contents_from_stream(p_stream,p_abort);
}

void dsp_chain_config::remove_item(unsigned p_index)
{
	remove_mask(bit_array_one(p_index));
}

void dsp_chain_config::add_item(const dsp_preset & p_data)
{
	insert_item(p_data,get_count());
}

void dsp_chain_config::remove_all()
{
	remove_mask(bit_array_true());
}

void dsp_chain_config::instantiate(service_list_t<dsp> & p_out)
{
	p_out.remove_all();
	unsigned n, m = get_count();
	for(n=0;n<m;n++)
	{
		service_ptr_t<dsp> temp;
		if (dsp_entry::g_instantiate(temp,get_item(n)))
			p_out.add_item(temp);
	}
}

unsigned dsp_chain_config_impl::get_count() const
{
	return m_data.get_count();
}

const dsp_preset & dsp_chain_config_impl::get_item(unsigned p_index) const
{
	return *m_data[p_index];
}

void dsp_chain_config_impl::replace_item(const dsp_preset & p_data,unsigned p_index)
{
	*m_data[p_index] = p_data;
}

void dsp_chain_config_impl::insert_item(const dsp_preset & p_data,unsigned p_index)
{
	m_data.insert_item(new dsp_preset_impl(p_data),p_index);
}

void dsp_chain_config_impl::remove_mask(const bit_array & p_mask)
{
	m_data.delete_mask(p_mask);
}

dsp_chain_config_impl::~dsp_chain_config_impl()
{
	m_data.delete_all();
}

t_io_result dsp_preset::contents_to_stream(stream_writer * p_stream,abort_callback & p_abort) const
{
	t_io_result status;
	unsigned long size = get_data_size();
	status = p_stream->write_lendian_t(get_owner(),p_abort);
	if (io_result_failed(status)) return status;
	status = p_stream->write_lendian_t(size,p_abort);
	if (io_result_failed(status)) return status;
	if (size > 0)
	{
		status = p_stream->write_object(get_data(),size,p_abort);
		if (io_result_failed(status)) return status;
	}
	return io_result_success;
}

t_io_result dsp_preset::contents_from_stream(stream_reader * p_stream,abort_callback & p_abort)
{
	t_io_result status;
	unsigned long size;
	GUID guid;
	status = p_stream->read_lendian_t(guid,p_abort);
	if (io_result_failed(status)) return status;
	set_owner(guid);
	status = p_stream->read_lendian_t(size,p_abort);
	if (io_result_failed(status)) return status;
	if (size > 1024*1024*10) return io_result_error_data;
	return set_data_from_stream(p_stream,size,p_abort);	
}

t_io_result dsp_preset::g_contents_from_stream_skip(stream_reader * p_stream,abort_callback & p_abort)
{
	t_io_result status;
	unsigned long size;
	GUID guid;
	status = p_stream->read_lendian_t(guid,p_abort);
	if (io_result_failed(status)) return status;
	status = p_stream->read_lendian_t(size,p_abort);
	if (io_result_failed(status)) return status;
	if (size > 1024*1024*10) return io_result_error_data;
	return p_stream->skip_object(size,p_abort);
}

t_io_result dsp_preset_impl::set_data_from_stream(stream_reader * p_stream,unsigned p_bytes,abort_callback & p_abort)
{
	m_data.set_size(p_bytes);
	if (p_bytes > 0 && m_data.get_ptr() == 0)
		return io_result_error_generic;
	else if (p_bytes == 0)
		return io_result_success;
	else
		return p_stream->read_object(m_data.get_ptr(),p_bytes,p_abort);
}

void dsp_chain_config::copy(const dsp_chain_config & p_source)
{
	remove_all();
	unsigned n, m = p_source.get_count();
	for(n=0;n<m;n++)
		add_item(p_source.get_item(n));
}

bool dsp_entry::g_have_config_popup(const GUID & p_guid)
{
	service_ptr_t<dsp_entry> entry;
	if (!g_get_interface(entry,p_guid)) return false;
	return entry->have_config_popup();
}

bool dsp_entry::g_have_config_popup(const dsp_preset & p_preset)
{
	return g_have_config_popup(p_preset.get_owner());
}

bool dsp_entry::g_show_config_popup(dsp_preset & p_preset,HWND p_parent)
{
	service_ptr_t<dsp_entry> entry;
	if (!g_get_interface(entry,p_preset.get_owner())) return false;
	return entry->show_config_popup(p_preset,p_parent);
}

bool dsp_entry::g_get_interface(service_ptr_t<dsp_entry> & p_out,const GUID & p_guid)
{
	service_ptr_t<dsp_entry> ptr;
	service_enum_t<dsp_entry> e;
	e.reset();
	while(e.next(ptr))
	{
		if (ptr->get_guid() == p_guid)
		{
			p_out = ptr;
			return true;
		}
	}
	return false;
}

bool resampler_entry::g_get_interface(service_ptr_t<resampler_entry> & p_out,unsigned p_srate_from,unsigned p_srate_to)
{
	service_ptr_t<dsp_entry> ptr_dsp;
	service_ptr_t<resampler_entry> ptr_resampler;
	service_enum_t<dsp_entry> e;
	e.reset();
	float found_priority = 0;
	service_ptr_t<resampler_entry> found;
	while(e.next(ptr_dsp))
	{
		if (ptr_dsp->service_query_t(ptr_resampler))
		{
			if (p_srate_from == 0 || ptr_resampler->is_conversion_supported(p_srate_from,p_srate_to))
			{
				float priority = ptr_resampler->get_priority();
				if (found.is_empty() || priority > found_priority)
				{
					found = ptr_resampler;
					found_priority = priority;
				}
			}
		}
	}
	if (found.is_empty()) return false;
	p_out = found;
	return true;
}

bool resampler_entry::g_create_preset(dsp_preset & p_out,unsigned p_srate_from,unsigned p_srate_to,float p_qualityscale)
{
	service_ptr_t<resampler_entry> entry;
	if (!g_get_interface(entry,p_srate_from,p_srate_to)) return false;
	return entry->create_preset(p_out,p_srate_to,p_qualityscale);
}

bool resampler_entry::g_create(service_ptr_t<dsp> & p_out,unsigned p_srate_from,unsigned p_srate_to,float p_qualityscale)
{
	service_ptr_t<resampler_entry> entry;
	if (!g_get_interface(entry,p_srate_from,p_srate_to)) return false;
	dsp_preset_impl preset;
	if (!entry->create_preset(preset,p_srate_to,p_qualityscale)) return false;
	return entry->instantiate(p_out,preset);
}


void dsp_chain_config::get_name_list(string_base & p_out) const
{
	const unsigned count = get_count();
	bool added = false;
	for(unsigned n=0;n<count;n++)
	{
		service_ptr_t<dsp_entry> ptr;
		if (dsp_entry::g_get_interface(ptr,get_item(n).get_owner()))
		{
			if (added) p_out += ", ";
			added = true;

			string8 temp;
			ptr->get_name(temp);
			p_out += temp;
		}
	}
}