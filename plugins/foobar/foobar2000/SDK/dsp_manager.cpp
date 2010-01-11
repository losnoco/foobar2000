#include "foobar2000.h"

void dsp_manager::set_config( const dsp_chain_config & p_data )
{
	//dsp_chain_config::g_instantiate(m_dsp_list,p_data);
	m_config.copy(p_data);
	m_config_changed = true;
}

void dsp_manager::dsp_run(t_size idx,dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags,double & latency)
{
	list->remove_bad_chunks();

	TRACK_CODE("dsp::run",m_dsp_list[idx]->run(list,cur_file,flags));
	TRACK_CODE("dsp::get_latency",latency += m_dsp_list[idx]->get_latency());
}

double dsp_manager::run(dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags)
{
	TRACK_CALL_TEXT("dsp_manager::run");

	try {
		fpu_control_default l_fpu_control;

		t_size n;

		
		double latency=0;
		bool done = false;

		if (m_config_changed)
		{
			if (m_dsp_list.get_count()>0)
			{
				for(n=0;n<m_dsp_list.get_count();n++)
				{
					dsp_run(n,list,cur_file,flags|dsp::FLUSH,latency);
				}
				done = true;
			}

			m_config.instantiate(m_dsp_list);
			m_config_changed = false;
		}
		
		if (!done)
		{
			t_size num_flush = 0;
			if ((flags & dsp::END_OF_TRACK) && ! (flags & dsp::FLUSH))
			{
				for(n=0;n<m_dsp_list.get_count();n++)
				{
					if (m_dsp_list[n].is_valid())
					{
						if (m_dsp_list[n]->need_track_change_mark())
							num_flush = n;
					}
				}
			}

			for(n=0;n<m_dsp_list.get_count();n++)
			{
				unsigned flags2 = flags;
				if (n<num_flush) flags2|=dsp::FLUSH;
				dsp_run(n,list,cur_file,flags2,latency);
			}
			done = true;
		}
		
		list->remove_bad_chunks();

		return latency;
	} catch(...) {
		list->remove_all();
		throw;
	}
}

void dsp_manager::flush()
{
	t_size n, m = m_dsp_list.get_count();
	for(n=0;n<m;n++) {
		TRACK_CODE("dsp::flush",m_dsp_list[n]->flush());
	}
}


bool dsp_manager::is_active() {return m_config.get_count()>0;}