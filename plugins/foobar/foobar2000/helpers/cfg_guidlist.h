class cfg_guidlist : public cfg_var, public mem_block_list<GUID>
{
public:
	bool get_raw_data(write_config_callback * out)
	{
		t_uint32 n, m = get_count();
		out->write_dword_le(m);
		for(n=0;n<m;n++)
			out->write_guid_le(get_item(n));
		return true;
	}

	void set_raw_data(const void * data,int size)
	{
		read_config_helper r(data,size);
		t_uint32 n,count;
		if (r.read_dword_le(count))
		{
			if (m_buffer.set_size(count))
			{
				for(n=0;n<count;n++)
				{
					if (!r.read_guid_le(m_buffer[n]))
					{
						m_buffer.set_size(0);
						break;
					}
				}

			}
		}
	}

	void sort()
	{
		sort_t(pfc::guid_compare);
	}


	bool have_item_bsearch(const GUID & p_item)
	{
		unsigned dummy;
		return bsearch_t(pfc::guid_compare,p_item,dummy);
	}


public:
	cfg_guidlist(const GUID & p_guid) : cfg_var(p_guid) {}
};
