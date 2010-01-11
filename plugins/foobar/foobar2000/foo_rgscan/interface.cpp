#include "../sdk/foobar2000.h"
#include "../helpers/helpers.h"
#include "replaygain.h"

#define uTEXT(blah) TEXT(blah)
#define uLVM_SETITEM LVM_SETITEM
#define uLVM_INSERTITEM LVM_INSERTITEM
#define uLVM_INSERTCOLUMN LVM_INSERTCOLUMN
#define uLVM_GETITEM LVM_GETITEM

static audio_sample calc_peak(const audio_sample * data,unsigned num,audio_sample peak)
{
	for(;num;num--)
	{
		audio_sample temp = (audio_sample)fabs(*(data++));
		if (temp>peak) peak = temp;
	}
	return peak;
}

static audio_sample calc_peak_chunk(const audio_chunk * c,audio_sample peak=0)
{
	return calc_peak(c->get_data(),c->get_data_length(),peak);
}


static bool g_process_chunk(rgcalc * p_calc,mem_block_t<rg_float> &buf,const audio_chunk * c)
{
	assert(p_calc->GetSampleRate()==c->get_srate());
	return p_calc->ProcessSamples( c->get_data(), c->get_channels(), c->get_sample_count() );
}

#include "resource.h"

// {D0F8D4E0-951A-4a5a-AF3A-D9B7C1B2EBF1}
static const GUID guid_cfg_use_skip = 
{ 0xd0f8d4e0, 0x951a, 0x4a5a, { 0xaf, 0x3a, 0xd9, 0xb7, 0xc1, 0xb2, 0xeb, 0xf1 } };

// {EEA7804E-0223-4e9a-8C45-1B1ADAB4C874}
static const GUID guid_cfg_album_pattern = 
{ 0xeea7804e, 0x223, 0x4e9a, { 0x8c, 0x45, 0x1b, 0x1a, 0xda, 0xb4, 0xc8, 0x74 } };

static cfg_int cfg_use_skip(guid_cfg_use_skip,0);
static cfg_string cfg_album_pattern(guid_cfg_album_pattern,"%album artist% - %album%");

class rg_results_dialog : private dialog_helper::dialog
{
public:

	rg_results_dialog(metadb_handle_list & p_data,array_t<replaygain_info> & p_results) : m_last_sort(-1)
	{
		pfc::swap_t(p_data,m_data);
		pfc::swap_t(p_results,m_results);
		const unsigned count = m_data.get_count();
		assert(count == m_results.get_size());
		m_order.set_size(count);
		unsigned n;
		for(n=0;n<count;n++) m_order[n]=n;
		m_names.set_size(count);
		for(n=0;n<count;n++) m_names[n] = string_filename(m_data[n]->get_path());
	}

	HWND run()
	{
		return run_modeless(IDD_REPLAYGAIN_RESULTS,core_api::get_main_window());
	}


	int compare_filename(unsigned p_index1,unsigned p_index2) {return uStringCompare(m_names[m_order[p_index1]],m_names[m_order[p_index2]]);}
	
	int compare_album_gain(unsigned p_index1,unsigned p_index2) {return pfc::compare_t(m_results[m_order[p_index1]].m_album_gain,m_results[m_order[p_index2]].m_album_gain);}
	int compare_track_gain(unsigned p_index1,unsigned p_index2) {return pfc::compare_t(m_results[m_order[p_index1]].m_track_gain,m_results[m_order[p_index2]].m_track_gain);}
	int compare_album_peak(unsigned p_index1,unsigned p_index2) {return pfc::compare_t(m_results[m_order[p_index1]].m_album_peak,m_results[m_order[p_index2]].m_album_peak);}
	int compare_track_peak(unsigned p_index1,unsigned p_index2) {return pfc::compare_t(m_results[m_order[p_index1]].m_track_peak,m_results[m_order[p_index2]].m_track_peak);}

	void swap(unsigned p_index1,unsigned p_index2)
	{
		pfc::swap_t(m_order[p_index1],m_order[p_index2]);
	}

private:

	class sort_callback_filename : public pfc::sort_callback
	{
	public:
		int compare(unsigned p_index1, unsigned p_index2) const {return m_mul * m_owner->compare_filename(p_index1,p_index2);}
		void swap(unsigned p_index1, unsigned p_index2) {m_owner->swap(p_index1,p_index2);}
		inline sort_callback_filename(rg_results_dialog * p_owner,int p_mul) : m_owner(p_owner), m_mul(p_mul) {}
	private:
		rg_results_dialog * m_owner;
		int m_mul;
	};

	class sort_callback_album_gain : public pfc::sort_callback
	{
	public:
		int compare(unsigned p_index1, unsigned p_index2) const {return m_mul * m_owner->compare_album_gain(p_index1,p_index2);}
		void swap(unsigned p_index1, unsigned p_index2) {m_owner->swap(p_index1,p_index2);}
		inline sort_callback_album_gain(rg_results_dialog * p_owner,int p_mul) : m_owner(p_owner), m_mul(p_mul) {}
	private:
		rg_results_dialog * m_owner;
		int m_mul;
	};

	class sort_callback_track_gain : public pfc::sort_callback
	{
	public:
		int compare(unsigned p_index1, unsigned p_index2) const {return m_mul * m_owner->compare_track_gain(p_index1,p_index2);}
		void swap(unsigned p_index1, unsigned p_index2) {m_owner->swap(p_index1,p_index2);}
		inline sort_callback_track_gain(rg_results_dialog * p_owner,int p_mul) : m_owner(p_owner), m_mul(p_mul) {}
	private:
		rg_results_dialog * m_owner;
		int m_mul;
	};

	class sort_callback_album_peak : public pfc::sort_callback
	{
	public:
		int compare(unsigned p_index1, unsigned p_index2) const {return m_mul * m_owner->compare_album_peak(p_index1,p_index2);}
		void swap(unsigned p_index1, unsigned p_index2) {m_owner->swap(p_index1,p_index2);}
		inline sort_callback_album_peak(rg_results_dialog * p_owner,int p_mul) : m_owner(p_owner), m_mul(p_mul) {}
	private:
		rg_results_dialog * m_owner;
		int m_mul;
	};

	class sort_callback_track_peak : public pfc::sort_callback
	{
	public:
		int compare(unsigned p_index1, unsigned p_index2) const {return m_mul * m_owner->compare_track_peak(p_index1,p_index2);}
		void swap(unsigned p_index1, unsigned p_index2) {m_owner->swap(p_index1,p_index2);}
		inline sort_callback_track_peak(rg_results_dialog * p_owner,int p_mul) : m_owner(p_owner), m_mul(p_mul) {}
	private:
		rg_results_dialog * m_owner;
		int m_mul;
	};

	void readd_results()
	{
		const HWND wnd = get_wnd();
		const HWND list = uGetDlgItem(wnd,IDC_LIST);

		unsigned n; const unsigned m = m_data.get_count();

		uSendMessage(list,LVM_DELETEALLITEMS,0,0);

		for(n=0;n<m;n++)
		{
			LVITEM item;
			memset(&item,0,sizeof(item));
			mem_block os_string_temp;

			os_string_temp.set_size(uOSStringEstimateSize(m_names[m_order[n]]));
			uOSStringConvert(m_names[m_order[n]],os_string_temp);


			item.mask = LVIF_TEXT;
			item.iItem = n;
			item.pszText = reinterpret_cast<TCHAR*>(os_string_temp.get_ptr());
			uSendMessage(list,uLVM_INSERTITEM,0,(LPARAM)&item);

			const replaygain_info & rg = m_results[m_order[n]];
			char rgtemp[replaygain_info::text_buffer_size];

			rg.format_album_gain(rgtemp);
			os_string_temp.set_size(uOSStringEstimateSize(rgtemp));
			uOSStringConvert(rgtemp,os_string_temp);
			item.iSubItem = 1;
			item.pszText = reinterpret_cast<TCHAR*>(os_string_temp.get_ptr());
			uSendMessage(list,uLVM_SETITEM,0,(LPARAM)&item);

			rg.format_track_gain(rgtemp);
			os_string_temp.set_size(uOSStringEstimateSize(rgtemp));
			uOSStringConvert(rgtemp,os_string_temp);
			item.iSubItem = 2;
			item.pszText = reinterpret_cast<TCHAR*>(os_string_temp.get_ptr());
			uSendMessage(list,uLVM_SETITEM,0,(LPARAM)&item);

			rg.format_album_peak(rgtemp);
			os_string_temp.set_size(uOSStringEstimateSize(rgtemp));
			uOSStringConvert(rgtemp,os_string_temp);
			item.iSubItem = 3;
			item.pszText = reinterpret_cast<TCHAR*>(os_string_temp.get_ptr());
			uSendMessage(list,uLVM_SETITEM,0,(LPARAM)&item);

			rg.format_track_peak(rgtemp);
			os_string_temp.set_size(uOSStringEstimateSize(rgtemp));
			uOSStringConvert(rgtemp,os_string_temp);
			item.iSubItem = 4;
			item.pszText = reinterpret_cast<TCHAR*>(os_string_temp.get_ptr());
			uSendMessage(list,uLVM_SETITEM,0,(LPARAM)&item);
		}

	}

	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp)
	{
		const HWND wnd = get_wnd();
		switch(msg)
		{
		case WM_INITDIALOG:
			modeless_dialog_manager::add(wnd);
			{
				RECT rect_filename = {0,0,150,0}, rect_column = {0,0,50,0};
				MapDialogRect(wnd,&rect_filename);
				MapDialogRect(wnd,&rect_column);

				const HWND list = uGetDlgItem(wnd,IDC_LIST);
				uSendMessage(list,LVM_SETEXTENDEDLISTVIEWSTYLE,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
				LVCOLUMN data;
				memset(&data,0,sizeof(data));
				data.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT ;
				data.fmt = LVCFMT_LEFT ;
				data.cx = rect_filename.right;
				data.pszText = uTEXT("File name");
				uSendMessage(list,uLVM_INSERTCOLUMN,0,(LPARAM)&data);
				data.cx = rect_column.right;
				data.pszText = uTEXT("Album gain");
				uSendMessage(list,uLVM_INSERTCOLUMN,1,(LPARAM)&data);
				data.pszText = uTEXT("Track gain");
				uSendMessage(list,uLVM_INSERTCOLUMN,2,(LPARAM)&data);
				data.pszText = uTEXT("Album peak");
				uSendMessage(list,uLVM_INSERTCOLUMN,3,(LPARAM)&data);
				data.pszText = uTEXT("Track peak");
				uSendMessage(list,uLVM_INSERTCOLUMN,4,(LPARAM)&data);

				readd_results();

			}

			return TRUE;
		case WM_NOTIFY:
			{
				LPNMHDR hdr = (LPNMHDR)lp;
				switch(wp)
				{
				case IDC_LIST:
					switch(hdr->code)
					{
					case LVN_COLUMNCLICK:
						{
							LPNMLISTVIEW hdr2 = (LPNMLISTVIEW) hdr;
							int mul = 1;
							if (m_last_sort == hdr2->iSubItem) {mul = -1; m_last_sort = -1;}
							else m_last_sort = hdr2->iSubItem;

							switch(hdr2->iSubItem)
							{
							case 0:
								pfc::sort_stable(sort_callback_filename(this,mul),m_data.get_count());
								readd_results();
								break;
							case 1:
								pfc::sort_stable(sort_callback_album_gain(this,mul),m_data.get_count());
								readd_results();
								break;
							case 2:
								pfc::sort_stable(sort_callback_track_gain(this,mul),m_data.get_count());
								readd_results();
								break;
							case 3:
								pfc::sort_stable(sort_callback_album_peak(this,mul),m_data.get_count());
								readd_results();
								break;
							case 4:
								pfc::sort_stable(sort_callback_track_peak(this,mul),m_data.get_count());
								readd_results();
								break;
							}
						}
						return TRUE;
					default:
						return FALSE;
					}
				default:
					return FALSE;
				}
			}
			return FALSE;
		case WM_COMMAND:
			switch(wp)
			{
			case IDOK:
				{
					static_api_ptr_t<metadb_io> api;
					if (api->is_busy())
					{
						MessageBeep(0);
						api->highlight_running_process();
					}
					else
					{
						file_info_update_helper helper(m_data);
						if (helper.read_infos(wnd,true))
						{
							unsigned n; const unsigned m = helper.get_item_count();
							for(n=0;n<m;n++)
							{
								if (m_results[n] == replaygain_info_invalid)
								{
									helper.invalidate_item(n);
								}
								else
								{
									helper.get_item(n).set_replaygain(m_results[n]);
								}
							}
							if (helper.write_infos(wnd,true) == file_info_update_helper::write_ok)
								DestroyWindow(wnd);
						}
					}
				}

				return TRUE;
			case IDCANCEL:
				DestroyWindow(wnd);
				return TRUE;
			default:
				return FALSE;
			}
		case WM_CLOSE:
			DestroyWindow(wnd);
			return TRUE;
		case WM_DESTROY:
			modeless_dialog_manager::remove(wnd);
			delete this;
			return TRUE;
		default:
			return FALSE;
		
		}
	}

	metadb_handle_list m_data;
	array_t<replaygain_info> m_results;
	mem_block_t<unsigned> m_order;
	array_t<string8> m_names;
	int m_last_sort;
};

class rg_thread_param : public threaded_process_callback
{
	metadb_handle_list m_files;
	int use_album;
	HANDLE thread;
	HWND dialog;
	abort_callback_impl m_abort;
	bool is_updating,is_updating_old;
	string8 cur_file;
	unsigned cur_file_idx;
	double duration_total,duration_done,duration_current;
	int last_progress;
	int fileptr;
	string8 album_pattern;
	bool use_skip;
	array_t<replaygain_info> m_results;


	void run(threaded_process_status & p_status,abort_callback & p_abort);
	void on_done(HWND,bool);
	

public:
	rg_thread_param(const list_base_const_t<metadb_handle_ptr> & p_files,int p_album_mode)
		: use_skip(!!cfg_use_skip), album_pattern(cfg_album_pattern), use_album(p_album_mode)
	{
		m_files.add_items(p_files);
		duration_total = m_files.calc_total_duration();
		duration_current = 0;
		duration_done = 0;
		thread=0;
		dialog=0;
		is_updating=is_updating_old=false;
		cur_file_idx=0;
		last_progress=-1;

		m_results.set_size(m_files.get_count());
		m_results.fill(replaygain_info_invalid);
	}
	~rg_thread_param() {}
private:
	bool process_file(rgcalc * calc,const metadb_handle_ptr & src_entry,audio_sample & peak,abort_callback & p_abort,threaded_process_status & p_status);
	void skip_file(const metadb_handle_ptr & src_entry,threaded_process_status & p_status);
	bool process_trackgain(unsigned p_index,abort_callback & p_abort,threaded_process_status & p_status);
	bool process_albumgain(unsigned p_index_base,unsigned p_count,abort_callback & p_abort,threaded_process_status & p_status);


	void set_status_scan(threaded_process_status & p_status,unsigned p_index)
	{
		if (p_index < m_files.get_count())
		{
			p_status.set_item_path(m_files[p_index]->get_path());

			char msg[128];
			sprintf(msg,"ReplayGain scan status (%u/%u)",p_index+1,m_files.get_count());
			p_status.set_title(msg);
		}
		else
		{
			p_status.set_item("");
			p_status.set_title("ReplayGain scan status");
		}

		p_status.process_pause();
		//p_status.set_progress_float(0);
		//p_status.set_progress_secondary_float( duration_done + val) / duration_total);
	}

	void set_status_update(threaded_process_status & p_status,unsigned p_index)
	{
		p_status.set_title("ReplayGain scan status - updating files");
		if (p_index < m_files.get_count())
		{
			p_status.set_item_path(m_files[p_index]->get_path());
		}
		else
		{
			p_status.set_item("");
		}

		p_status.process_pause();
	}

	void on_progress(threaded_process_status & p_status,double val)
	{
		if (duration_current > 0)
		{
			double duration_div = duration_current>0 ? 1.0 / duration_current : 0;
			p_status.set_progress_float(duration_div * val);
			p_status.set_progress_secondary_float( (duration_done + val) / duration_total);
		}
		else
		{
			p_status.set_progress_float(0);
			p_status.set_progress_secondary_float( duration_done / duration_total);
		}

		p_status.process_pause();
	}
};

static double get_duration(const metadb_handle_ptr & item)
{
	double duration = item->get_length();
	return duration > 0 ? duration : 0;
}

void rg_thread_param::skip_file(const metadb_handle_ptr & src_entry,threaded_process_status & p_status)
{
	duration_current = get_duration(src_entry);
	on_progress(p_status,duration_current);
	duration_done += duration_current;
}

bool rg_thread_param::process_file(rgcalc * calc,const metadb_handle_ptr & src_entry,audio_sample & peak,abort_callback & p_abort,threaded_process_status & p_status)
{
	mem_block_t<rg_float> deinterlace_buf;
	audio_sample title_peak = 0;
	input_helper the_input;
	the_input.hint_no_looping(true);
	the_input.hint_no_seeking(true);
	bool error = false;

	duration_current = get_duration(src_entry);
	
	if (io_result_succeeded(the_input.open(src_entry,p_abort)))
	{
		unsigned last_srate = calc->GetSampleRate();

		service_ptr_t<dsp> p_resampler_dsp;
		dsp_chunk_list_i chunk_list;

		bool first_chunk_in = true,first_chunk_out = true;
		audio_chunk_i chunk;
		double cur_time = 0;
		//int blah = 0;
		while(!p_abort.is_aborting() && !error)
		{
			t_io_result input_ret = the_input.run(&chunk,p_abort);

			if (io_result_failed(input_ret)) {error = true;break;}
			else if (input_ret == io_result_eof) chunk.reset();

			{
				rg_profiler(calc_peak);
				title_peak = calc_peak_chunk(&chunk,title_peak);
			}

			if (first_chunk_in)
			{
				if (p_resampler_dsp.is_empty() && (last_srate>0 ? last_srate!=chunk.get_srate() : !rgcalc::is_supported_samplerate(chunk.get_srate())))
				{
					resampler_entry::g_create(p_resampler_dsp,chunk.get_srate(),last_srate>0 ? last_srate : 48000,0);
				}

				first_chunk_in = false;
			}
			else
			{
				if (p_resampler_dsp.is_empty()&& last_srate!=chunk.get_srate())
				{
					resampler_entry::g_create(p_resampler_dsp,chunk.get_srate(),last_srate,0);
				}
			}

			if (!chunk.is_empty())
				chunk_list.add_chunk(&chunk);

			if (p_resampler_dsp.is_valid())
			{
				p_resampler_dsp->run(&chunk_list,src_entry,input_ret == io_result_eof ? (dsp::END_OF_TRACK|dsp::FLUSH) : 0);
			}

			UINT n_chunk;
			for(n_chunk = 0;n_chunk<chunk_list.get_count();n_chunk++)
			{
				audio_chunk * p_chunk = chunk_list.get_item(n_chunk);

				cur_time += p_chunk->get_duration();

//					if (p_chunk->get_channels()>2) {error = true;break;}
				if (last_srate>0)
				{
					if (!first_chunk_out && p_chunk->get_srate()!=last_srate) {error=true;break;}
					if (first_chunk_out && p_chunk->get_srate()!=last_srate)
					{
						error=true;break;
					}
				}
				
				if (last_srate == 0)
				{
					last_srate = p_chunk->get_srate();
					if (!calc->InitGainAnalysis(last_srate))
					{
						error = true;
						break;
					}
				}
				if (!g_process_chunk(calc,deinterlace_buf,p_chunk))
				{
					error = true;
					break;
				}
				first_chunk_out=false;
			}

			chunk_list.remove_all();

			if (error) break;

			on_progress(p_status,cur_time);

			if (input_ret==io_result_eof) break;
		}
		the_input.close();
	}
	duration_done += duration_current;
	peak = title_peak;
	return !error;
}

static bool check_trackgain(const metadb_handle_ptr & entry,abort_callback & p_abort)
{
	bool found = false;
	entry->metadb_lock();
	const file_info * info;
	if (entry->get_info_locked(info))
	{
		replaygain_info rg = info->get_replaygain();
		found = rg.is_track_gain_present() && rg.is_track_peak_present();
	}
	entry->metadb_unlock();
	return found;
}

static bool check_albumgain(const metadb_handle_ptr & entry,abort_callback & p_abort)
{
	bool found = false;
	entry->metadb_lock();
	const file_info * info;
	if (entry->get_info_locked(info))
	{
		replaygain_info rg = info->get_replaygain();
		found = rg.is_album_gain_present() && rg.is_album_peak_present();
	}
	entry->metadb_unlock();
	return found;
}

static bool check_albumgain_multi(const metadb_handle_list & list,unsigned p_base,unsigned p_count,abort_callback & p_abort)
{
	unsigned n;
	for(n=0;n<p_count;n++)
	{
		if (!check_albumgain(list[p_base+n],p_abort)) return false;
	}
	return true;
}

bool rg_thread_param::process_trackgain(unsigned p_index,abort_callback & p_abort,threaded_process_status & p_status)
{
	set_status_scan(p_status,p_index);

	metadb_handle_ptr src_entry = m_files[p_index];
	if (use_skip && check_trackgain(src_entry,p_abort))
	{
		console::info("skipped file:");
		console::info_location(src_entry);
		skip_file(src_entry,p_status);
	}
	rgcalc * calc = rgcalc::create();
	audio_sample title_peak = 0;	
	bool error = !process_file(calc,src_entry,title_peak,p_abort,p_status);
	if (error || m_abort.is_aborting()) return false;

	

	file_info_impl info;
	if (src_entry->get_info(info))
	{
		info.info_set_replaygain_track_gain((float)calc->GetTitleGain());
		info.info_set_replaygain_track_peak((float)title_peak);
		m_results[p_index] = info.get_replaygain();
	}

#ifdef RG_BENCHMARK
	console::info(uStringPrintf("%d",(int)(100.0 * calc->GetTitleGain())));
	console::info(uStringPrintf("%d",(int)(100.0 * title_peak)));
#endif
	delete calc;
	return true;
}

bool rg_thread_param::process_albumgain(unsigned p_index_base,unsigned p_count,abort_callback & p_abort,threaded_process_status & p_status)
{
	set_status_scan(p_status,p_index_base);
	if (use_skip && check_albumgain_multi(m_files,p_index_base,p_count,p_abort))
	{
		console::info("skipped album:");
		unsigned n;
		for(n=0;n<p_count;n++)
		{
			skip_file(m_files[n + p_index_base],p_status);
			console::info_location(m_files[n + p_index_base]);
		}
		return true;
	}
	rgcalc * calc = rgcalc::create();
	audio_sample album_peak = 0;
	unsigned file_ptr;
	mem_block_t<float> track_gain(p_count);
	mem_block_t<audio_sample> track_peak(p_count);
	for(file_ptr = 0; file_ptr < p_count; file_ptr++)
	{
		set_status_scan(p_status,p_index_base + file_ptr);
		metadb_handle_ptr src_entry = m_files[file_ptr + p_index_base];
		audio_sample title_peak = 0;
		bool error = !process_file(calc,src_entry,title_peak,p_abort,p_status);
		if (error || p_abort.is_aborting()) break;
		if (album_peak < title_peak) album_peak = title_peak;
		track_gain[file_ptr] = (float)calc->GetTitleGain();
		track_peak[file_ptr] = title_peak;
	}

#ifndef RG_BENCHMARK
	{
		

		float album_gain = (float)calc->GetAlbumGain();


		for(file_ptr = 0; file_ptr<p_count && !p_abort.is_aborting();file_ptr++)
		{
			file_info_impl info;
			if (m_files[file_ptr + p_index_base]->get_info(info))
			{
				info.info_set_replaygain_track_gain((float)track_gain[file_ptr]);
				info.info_set_replaygain_track_peak((float)track_peak[file_ptr]);
				info.info_set_replaygain_album_gain((float)album_gain);
				info.info_set_replaygain_album_peak((float)album_peak);
				m_results[file_ptr + p_index_base] = info.get_replaygain();
			}
		}

	}
#endif

	delete calc;
	return true;
}

void rg_thread_param::run(threaded_process_status & p_status,abort_callback & p_abort)
{
	SetThreadPriority(GetCurrentThread(),
#ifdef RG_BENCHMARK
		THREAD_PRIORITY_TIME_CRITICAL
#else
		THREAD_PRIORITY_BELOW_NORMAL
#endif
		);

#ifdef RG_BENCHMARK
	profiler(rg_threadfunc);
	DWORD start_time = GetTickCount();
#endif
	switch(use_album)
	{
	default:
		break;
	case 0://track gain only
		{
			unsigned n,m = m_files.get_count();
			for(n=0;n<m && !p_abort.is_aborting();n++)
			{
				if (!p_status.process_pause()) break;
				if (!process_trackgain(n,p_abort,p_status)) break;
			}
		}
		break;
	case 1://single album
		//unsigned p_index_base,unsigned p_count,abort_callback & p_abort,threaded_process_status & p_status
		process_albumgain(0,m_files.get_count(),p_abort,p_status);
		break;
	case 2://multiple albums
		{
			{
				string8 sort_format;
				sort_format = album_pattern;
				sort_format += "|%path%";
				m_files.sort_by_format(sort_format,0);
			}
			
			unsigned n,m = m_files.get_count();
			string8_fastalloc cur_album,temp_album;
			unsigned done = 0;
			bool error = false;
			for(n=0;n<m && !p_abort.is_aborting() && !error;n++)
			{
				metadb_handle_ptr entry = m_files[n];
				if (n>done)
				{
					entry->format_title_legacy(temp_album,album_pattern,0);
					if (stricmp_utf8(temp_album,cur_album))
					{
						error = !process_albumgain(done,n-done,p_abort,p_status);
						done = n;
						cur_album = temp_album;
					}
				}
				else
				{
					entry->format_title_legacy(cur_album,album_pattern,0);
				}
			}
			if (!error && !p_abort.is_aborting() && done < n)
			{
				error = !process_albumgain(done,n-done,p_abort,p_status);
				done = n;
			}
		}
		break;
	};

#ifdef RG_BENCHMARK
	console::info(uStringPrintf("RG time: %u",GetTickCount() - start_time));
	console::popup();
#endif

}

void rg_thread_param::on_done(HWND p_wnd,bool p_aborted)
{
	if (!p_aborted)
	{
		rg_results_dialog * dialog = new rg_results_dialog(m_files,m_results);
		if (dialog)
		{
			HWND wnd = dialog->run();
			if (wnd == 0) delete dialog;
		}
#if 0
		file_info_update_helper helper(m_files);
		if (helper.read_infos(p_wnd,true))
		{
			unsigned n; const unsigned m = helper.get_item_count();
			for(n=0;n<m;n++)
			{
				if (m_results[n] == replaygain_info_invalid)
				{
					helper.invalidate_item(n);
				}
				else
				{
					helper.get_item(n).set_replaygain(m_results[n]);
				}
			}
			helper.write_infos(core_api::get_main_window(),true);
		}
#endif
	}
	delete this;
}
static void replaygain_process(const list_base_const_t<metadb_handle_ptr> & p_files,int use_album)
{
	rg_thread_param * param = new rg_thread_param(p_files,use_album);
	if (param != 0)
	{
		if (!threaded_process::g_run_modeless(
			*param,
			threaded_process::flag_show_abort | threaded_process::flag_show_progress_dual | threaded_process::flag_show_item | threaded_process::flag_show_pause,
			core_api::get_main_window(),
			"ReplayGain scanner"
			))
			delete param;
	}
}

static BOOL CALLBACK RgEditProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			//metadb_handle * ptr = reinterpret_cast<metadb_handle_ptr>(lp);
            list_base_t<metadb_handle_ptr> *data = reinterpret_cast<list_base_t<metadb_handle_ptr> *>(lp);
            metadb_handle_ptr ptr = data->get_item(0);
			file_info_impl info;
			if (!ptr->get_info(info))
				info.reset();
			uSetDlgItemText(wnd,IDC_FILE,file_path_display(ptr->get_path()));

			replaygain_info rg = info.get_replaygain();
			char rgtemp[replaygain_info::text_buffer_size];
			if (rg.is_album_gain_present())
			{
				rg.format_album_gain(rgtemp);
				uSetDlgItemText(wnd,IDC_ALBUM_GAIN,rgtemp);
			}
			if (rg.is_album_peak_present())
			{
				rg.format_album_peak(rgtemp);
				uSetDlgItemText(wnd,IDC_ALBUM_PEAK,rgtemp);
			}

			if (data->get_count() == 1)
			{
				if (rg.is_track_gain_present())
				{
					rg.format_track_gain(rgtemp);
					uSetDlgItemText(wnd,IDC_TRACK_GAIN,rgtemp);
				}
				if (rg.is_track_peak_present())
				{
					rg.format_track_peak(rgtemp);
					uSetDlgItemText(wnd,IDC_TRACK_PEAK,rgtemp);
				}
			}
			else
			{
                EnableWindow(GetDlgItem(wnd, IDC_TRACK_GAIN), 0);
                EnableWindow(GetDlgItem(wnd, IDC_TRACK_PEAK), 0);
            }
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				//metadb_handle * ptr = reinterpret_cast<metadb_handle_ptr>(uGetWindowLong(wnd,DWL_USER));

				file_info_update_helper helper(* reinterpret_cast<list_base_t<metadb_handle_ptr> *>(uGetWindowLong(wnd,DWL_USER)));
				if (helper.read_infos(wnd,true))
				{
					unsigned n; const unsigned m = helper.get_item_count();
					for(n=0;n<m;n++)
					{
						file_info & info = helper.get_item(n);

						replaygain_info rg = info.get_replaygain();
						if (m == 1) {
							string8_fastalloc temp;
							uGetDlgItemText(wnd,IDC_ALBUM_GAIN,temp);
							rg.set_album_gain_text(temp);
							uGetDlgItemText(wnd,IDC_ALBUM_PEAK,temp);
							rg.set_album_peak_text(temp);
							uGetDlgItemText(wnd,IDC_TRACK_GAIN,temp);
							rg.set_track_gain_text(temp);
							uGetDlgItemText(wnd,IDC_TRACK_PEAK,temp);
							rg.set_track_peak_text(temp);

						} else {
							string8_fastalloc temp;
							uGetDlgItemText(wnd,IDC_ALBUM_GAIN,temp);
							rg.set_album_gain_text(temp);
							uGetDlgItemText(wnd,IDC_ALBUM_PEAK,temp);
							rg.set_album_peak_text(temp);
						}
						
						info.set_replaygain(rg);
					}
					helper.write_infos(wnd,true);
				}
			}
			EndDialog(wnd,1);
			break;
		case IDCANCEL:
			EndDialog(wnd,0);
			break;
		}
		break;
	}
	return 0;
}

class contextmenu_rg : public menu_item_legacy_context
{
public:
	virtual GUID get_item_guid(unsigned n)
	{
		// {575A5437-8EDA-4c9b-8857-FE469BC4BA2D}
		static const GUID guid_edit = 
		{ 0x575a5437, 0x8eda, 0x4c9b, { 0x88, 0x57, 0xfe, 0x46, 0x9b, 0xc4, 0xba, 0x2d } };

		static const GUID* guids[] = {
			&standard_commands::guid_context_rg_scan_track,
			&standard_commands::guid_context_rg_scan_album,
			&standard_commands::guid_context_rg_scan_album_multi,
			&standard_commands::guid_context_rg_remove,
			&guid_edit
		};
		return *guids[n];
	}

	virtual unsigned get_num_items() {return 5;}
	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & data,string_base & out,unsigned& flags,const GUID& caller)
	{
		bool rv = false;
		unsigned count = data.get_count();
		switch(n)
		{
			case 0:	rv = count > 0;break;
			case 1: rv = count > 0;break;
			case 2: rv = count > 1;break;
			case 3: rv = count > 0;break;
			//case 4: rv = count == 1;break;
            case 4: rv = count > 0;break;
			default: rv = false;
		}
		return rv ? menu_item_legacy_context::context_get_display(n,data,out,flags,caller) : false;
	}
	virtual void get_item_name(unsigned n,string_base & out)
	{
		static const char * strings[] = {
			"Scan per-file track gain",
			"Scan selection as album",
			"Scan selection as multiple albums",
			"Remove replaygain info from files",
			"Edit replaygain info (advanced)",
		};
		out = strings[n];
	}

	virtual void get_item_default_path(unsigned n,string_base & out)
	{
		out = "ReplayGain";
	}

	bool get_item_description(unsigned n,string_base & out)
	{
		static const char * strings[] = {
			"ReplayGain-scans selected items as independant tracks.",
			"ReplayGain-scans selected items as an album.",
			"ReplayGain-scans selected items as multiple albums, dividing them according to specified field.",
			"Removes ReplayGain info from selected items.",
			"Edits ReplayGain info on selected items. For advanced users only.",
		};
		out = strings[n];
		return true;
	}

	
	virtual void context_command(unsigned cmd,const list_base_const_t<metadb_handle_ptr> & data,const GUID&)
	{
		if (data.get_count()>0)
		{
			if (static_api_ptr_t<metadb_io>()->is_busy()) MessageBeep(0);
			else
			{
				if (cmd==0) replaygain_process(data,0);
				else if (cmd==1) replaygain_process(data,1);
				else if (cmd==2) replaygain_process(data,2);
				else if (cmd==3)
				{
					file_info_update_helper helper(data);
					if (helper.read_infos(core_api::get_main_window(),true))
					{
						unsigned n; const unsigned m = helper.get_item_count();
						for(n=0;n<m;n++) helper.get_item(n).reset_replaygain();

						helper.write_infos(core_api::get_main_window(),true);
					}
				}
				else if (cmd==4)
				{
					uDialogBox(IDD_REPLAYGAIN_EDIT,core_api::get_main_window(),RgEditProc,(long)&data);
				}
			}
		}
	}	
};

static menu_item_factory_t<contextmenu_rg> foo;


class preferences_page_rgscan : public preferences_page
{
	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			uSendDlgItemMessage(wnd,IDC_SKIP,BM_SETCHECK,cfg_use_skip,0);
			uSetDlgItemText(wnd,IDC_ALBUM_PATTERN,cfg_album_pattern);			
			return 1;
		case WM_COMMAND:
			switch(wp)
			{
			case IDC_SKIP:
				cfg_use_skip = uSendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_ALBUM_PATTERN | (EN_CHANGE<<16):
				cfg_album_pattern = string_utf8_from_window((HWND)lp);
				break;
			}
			break;
		}
		return 0;
	}
public:
	HWND create(HWND parent)
	{
		return uCreateDialog(IDD_REPLAYGAIN_CONFIG,parent,DialogProc);
	}
	const char * get_name() {return "ReplayGain scanner";}
	GUID get_guid()
	{
		// {6851CCA5-2A22-4c17-A88E-3E28C230D802}
		static const GUID guid = 
		{ 0x6851cca5, 0x2a22, 0x4c17, { 0xa8, 0x8e, 0x3e, 0x28, 0xc2, 0x30, 0xd8, 0x2 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_components;}

	bool reset_query() {return true;}
	
	void reset() 
	{
		cfg_use_skip = 0;
		cfg_album_pattern = "%album artist% - %album%";
	}

};

static preferences_page_factory_t<preferences_page_rgscan> g_preferences_page_rgscan_factory;

DECLARE_COMPONENT_VERSION("ReplayGain Scanner","1.3",0)