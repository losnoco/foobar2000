#include "pfc.h"


static t_uint32 read_dword_le_fromptr(const void * src) {return byte_order::dword_le_to_native(*(t_uint32*)src);}

cfg_var * cfg_var::list=0;
#if 0
void cfg_var::config_reset()
{
	cfg_var * ptr;
	for(ptr = list; ptr; ptr=ptr->next) ptr->reset();
}
#endif


static int cfg_var_guid_compare(const cfg_var * p_var1,const cfg_var * p_var2)
{
	return pfc::guid_compare(p_var1->get_guid(),p_var2->get_guid());
}

static int cfg_var_guid_compare_search(const cfg_var * p_var1,const GUID & p_var2)
{
	return pfc::guid_compare(p_var1->get_guid(),p_var2);
}

void cfg_var::config_read_file(const void * p_src,unsigned p_size)
{
	read_config_helper r(p_src,p_size);
	t_uint32 size;
	GUID guid;
#if 0
	list_t<cfg_var*> vars;

	{
		unsigned count = 0;
		for(cfg_var * ptr = list; ptr; ptr = ptr->next) count++;
		vars.set_count(count);

		count = 0;

		for(cfg_var * ptr = list; ptr; ptr = ptr->next)
			vars[count++] = ptr;

		vars.sort_t(cfg_var_guid_compare);
	}

	while(r.get_remaining() >= sizeof(guid) + sizeof(size))
	{
		if (!r.read_guid_le(guid)) break;
		if (!r.read_dword_le(size)) break;
		if (size > r.get_remaining()) break;

		unsigned index;

		if (vars.bsearch_t(cfg_var_guid_compare_search,guid,index))
		{
			vars[index]->set_raw_data(r.get_ptr(),size);
		}
		r.advance(size);
	}
#else

	while(r.get_remaining() >= sizeof(guid) + sizeof(size))
	{
		if (!r.read_guid_le(guid)) break;
		if (!r.read_dword_le(size)) break;
		if (size > r.get_remaining()) break;

		cfg_var * ptr;
		for(ptr = list; ptr; ptr=ptr->next)
		{
			if (ptr->get_guid() == guid)
			{
				ptr->set_raw_data(r.get_ptr(),size);
				break;
			}
		}
		r.advance(size);
	}
#endif
}

void cfg_var::config_write_file(write_config_callback * out)
{
	cfg_var * ptr;
	write_config_callback_i temp;
	for(ptr = list; ptr; ptr=ptr->next)
	{
		temp.data.set_size(0);
		if (ptr->get_raw_data(&temp))
		{
			out->write_guid_le(ptr->get_guid());
			unsigned size = temp.data.get_size();
			out->write_dword_le(size);
			if (size>0) out->write(temp.data.get_ptr(),size);
		}
	}
}




void cfg_var::write_config_callback::write_dword_le(t_uint32 param)
{
	t_uint32 temp = byte_order::dword_native_to_le(param);
	write(&temp,sizeof(temp));
}

void cfg_var::write_config_callback::write_dword_be(t_uint32 param)
{
	t_uint32 temp = byte_order::dword_native_to_be(param);
	write(&temp,sizeof(temp));
}

void cfg_var::write_config_callback::write_string(const char * param)
{
	unsigned len = strlen(param);
	write_dword_le(len);
	write(param,len);
}

bool cfg_int::get_raw_data(write_config_callback * out)
{
	out->write_dword_le(m_val);
	return true;
}

void cfg_int::set_raw_data(const void * data,int size)
{
	if (size==sizeof(long))
	{
		m_val = (t_int32)read_dword_le_fromptr(data);
	}
}


bool cfg_var::read_config_helper::read(void * out,unsigned bytes)
{
	if (bytes>remaining) return false;
	memcpy(out,ptr,bytes);
	advance(bytes);
	return true;
}

bool cfg_var::read_config_helper::read_dword_le(t_uint32 & val)
{
	t_uint32 temp;
	if (!read(&temp,sizeof(temp))) return false;
	val = byte_order::dword_le_to_native(temp);
	return true;
}
bool cfg_var::read_config_helper::read_dword_be(t_uint32 & val)
{
	t_uint32 temp;
	if (!read(&temp,sizeof(temp))) return false;
	val = byte_order::dword_be_to_native(temp);
	return true;
}

bool cfg_var::read_config_helper::read_string(string_base & out)
{
	t_uint32 len;
	if (!read_dword_le(len)) return false;
	if (remaining<len) return false;
	out.set_string((const char*)ptr,len);
	advance(len);
	return true;
}

void cfg_var::write_config_callback::write_guid_le(const GUID& param)
{
	GUID temp = param;
	byte_order::guid_native_to_le(temp);
	write(&temp,sizeof(temp));
}

bool cfg_var::read_config_helper::read_guid_le(GUID & val)
{
	if (!read(&val,sizeof(val))) return false;
	byte_order::guid_le_to_native(val);
	return true;
}
