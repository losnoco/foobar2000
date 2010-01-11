namespace cue_creator
{
	struct t_index
	{
		unsigned m_index;
		double m_offset;
	};

	typedef pfc::chain_list_t<t_index> t_index_list;

	struct t_entry
	{
		file_info_impl m_infos;
		string8 m_file;
		unsigned m_track_number;

		void set_simple_index(double p_time);

		t_index_list m_index_list;
	};
	
	typedef pfc::chain_list_t<t_entry> t_entry_list;

	void create(string_formatter & p_out,const pfc::chain_list_t<t_entry> & p_list);
};