#include "foobar2000.h"

void dsp_manager::set_config( const dsp_chain_config & p_data )
{
	//dsp_chain_config::g_instantiate(m_dsp_list,p_data);
	m_config.copy(p_data);
	m_config_changed = true;
}

void dsp_manager::dsp_run(unsigned idx,dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags,double & latency)
{
	list->remove_bad_chunks();

	TRACK_CALL_TEXT("dsp::run");	
	m_dsp_list[idx]->run(list,cur_file,flags);
	latency += m_dsp_list[idx]->get_latency();
}

double dsp_manager::run(dsp_chunk_list * list,const metadb_handle_ptr & cur_file,unsigned flags)
{
	fpu_control_default l_fpu_control;

	unsigned n;

	
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
		unsigned num_flush = 0;
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
}

void dsp_manager::flush()
{
	TRACK_CALL_TEXT("dsp_manager::flush");
	unsigned n, m = m_dsp_list.get_count();
	for(n=0;n<m;n++)
	{
		m_dsp_list[n]->flush();
	}
}


bool dsp_manager::is_active() {return m_config.get_count()>0;}