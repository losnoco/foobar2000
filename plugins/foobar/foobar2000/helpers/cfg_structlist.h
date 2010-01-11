template<typename T>
class cfg_structlist_t : public cfg_var, public mem_block_list<T>
{
public:
	bool get_raw_data(write_config_callback * out)
	{
		t_uint32 n, m = get_count();
		out->write_dword_le(m);
		for(n=0;n<m;n++)
		{
			out->write(&m_buffer[n],sizeof(T));
		}
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
					if (!r.read(&m_buffer[n],sizeof(T)))
					{
						m_buffer.set_size(0);
						break;
					}
				}

			}
		}
	}

public:
	cfg_structlist_t(const GUID & p_guid) : cfg_var(p_guid) {}
};