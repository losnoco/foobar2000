#include "stdafx.h"
#include "input_mp3.h"
#include "resource.h"
#include <math.h>



// {38FEC2F2-A3DF-4632-9F62-AF0A572C3D9B}
static const GUID guid_cfg_mp3fix_rebuild = 
{ 0x38fec2f2, 0xa3df, 0x4632, { 0x9f, 0x62, 0xaf, 0xa, 0x57, 0x2c, 0x3d, 0x9b } };

static cfg_int cfg_mp3fix_rebuild(guid_cfg_mp3fix_rebuild,0);


static const unsigned cd_sector_size = 588;

static bool is_mp3_file(const metadb_handle_ptr & ptr)
{
	return !stricmp_utf8(string_extension(ptr->get_path()),"MP3");
}

struct headerfix_dialogparam
{
	unsigned delay;
	unsigned total_samples;
	bool rebuild;
};

static BOOL CALLBACK mp3fixproc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			headerfix_dialogparam * param = (headerfix_dialogparam*)lp;
			uSetDlgItemInt(wnd,IDC_DELAY,param->delay,0);
			uSetDlgItemInt(wnd,IDC_TOTAL,param->total_samples,0);
			uButton_SetCheck(wnd,IDC_REBUILD,param->rebuild);
		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDC_DOWN:
			{
				unsigned val = uGetDlgItemInt(wnd,IDC_TOTAL,0,0);
				unsigned mod = val % cd_sector_size;
				if (mod == 0) val -= cd_sector_size;
				else val -= mod;
				uSetDlgItemInt(wnd,IDC_TOTAL,val,0);
			}
			return TRUE;
		case IDC_UP:
			{
				unsigned val = uGetDlgItemInt(wnd,IDC_TOTAL,0,0);
				unsigned mod = val % cd_sector_size;
				if (mod == 0) val += cd_sector_size;
				else val += cd_sector_size - mod;
				uSetDlgItemInt(wnd,IDC_TOTAL,val,0);
			}
			return TRUE;
		case IDOK:
			{
				headerfix_dialogparam * param = (headerfix_dialogparam*)uGetWindowLong(wnd,DWL_USER);
				param->delay = uGetDlgItemInt(wnd,IDC_DELAY,0,0);
				param->total_samples = uGetDlgItemInt(wnd,IDC_TOTAL,0,0);
				param->rebuild = uButton_GetCheck(wnd,IDC_REBUILD);
			}
			EndDialog(wnd,1);
			return TRUE;
		case IDCANCEL:
			EndDialog(wnd,0);
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}


static void process_headerfix(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	if (p_data.get_count()==1 && is_mp3_file(p_data[0]))
	{
		abort_callback_impl p_abort;
		metadb_handle_ptr handle = p_data[0];
		headerfix_dialogparam param = {576,0,!!cfg_mp3fix_rebuild};
		handle->metadb_lock();
		{
			const file_info * p_info;
			if (handle->get_info_locked(p_info))
			{
				param.delay = (unsigned)p_info->info_get_int("enc_delay");
				if (p_info->info_get("mp3_accurate_length")) param.total_samples = (unsigned)p_info->info_get_length_samples();
				if (param.delay==0 && !p_info->info_get("enc_delay")) 
				{
					param.delay = 576;
					if (param.total_samples>576) param.total_samples-=576;
					else param.total_samples = 0;
				}
			}
		}
		handle->metadb_unlock();
		if (uDialogBox(IDD_MP3FIX,core_api::get_main_window(),mp3fixproc,(long)&param) > 0)
		{
			t_io_result io_status;
			cfg_mp3fix_rebuild = !!param.rebuild;
			bool success = false;
			file_info_impl info;
			service_ptr_t<file> l_reader;
			if (io_result_failed(filesystem::g_open(l_reader,handle->get_path(),filesystem::open_mode_write_existing,p_abort)))
				console::error("mp3 header fix: unable to open file");
			else if (!l_reader->can_seek()) console::error("mp3 header fix: file isn't seekable");
			else
			{
				service_impl_single_t<input_mp3> in;
				
				if (io_result_failed(io_status = in.open(l_reader,handle->get_location(),info,p_abort,0)))
				{
					popup_message::g_show_file_error("Error opening file",io_status,handle->get_path(),"MP3 header fix");
				}
				else
				{
					input_mp3::g_purge_seektable(handle->get_path());
					service_ptr_t<file> p_temp;
					

					try {
						if (l_reader->get_size_e(p_abort) > 16*1024*1024)
							filesystem::g_open_temp_e(p_temp,p_abort);
						else
							filesystem::g_open_tempmem_e(p_temp,p_abort);

						in.process_lamefix_e(p_temp,param.delay,param.total_samples,param.rebuild,p_abort);
						l_reader->seek_e(0,p_abort);
						l_reader->set_eof_e(p_abort);
						p_temp->seek_e(0,p_abort);
						file::g_transfer_object_e(p_temp.get_ptr(),l_reader.get_ptr(),p_temp->get_size_e(p_abort),p_abort);
					} catch(t_io_result code)
					{
						popup_message::g_show_file_error("Error processing file",code,handle->get_path(),"MP3 header fix");
					}
				}
			}
			l_reader.release();
			if (success)
			{
				static_api_ptr_t<metadb_io>()->load_info(handle,metadb_io::load_info_force,core_api::get_main_window(),false);
			}
		}
	}
}

struct tagging_dialogparam
{
	unsigned m_apev2,m_id3v1,m_id3v2;
	unsigned m_count;
};

static void process_tagstate(bool p_found,unsigned & p_state)
{
	if (p_state == (unsigned)(-1))
	{
		p_state = p_found ? BST_CHECKED : BST_UNCHECKED;
	}
	else if (p_state == BST_INDETERMINATE)
	{
	}
	else if (p_state == BST_CHECKED)
	{
		if (!p_found) p_state = BST_INDETERMINATE;
	}
	else if (p_state == BST_UNCHECKED)
	{
		if (p_found) p_state = BST_INDETERMINATE;
	}
}

static bool process_tagstate_v2(bool p_found,unsigned p_state, bool & p_changed)
{
	switch(p_state)
	{
	case BST_CHECKED:
		{
			if (!p_found) p_changed = true;
			return true;
		}
	case BST_UNCHECKED:
		{
			if (p_found) p_changed = true;
			return false;
		}
	case BST_INDETERMINATE: return p_found;
	default: return p_found;
	}
}

static void TogglewindowStyle(HWND wnd,unsigned p_flag)
{
	uSetWindowLong(wnd,GWL_STYLE,uGetWindowLong(wnd,GWL_STYLE) ^ p_flag);
}


static BOOL CALLBACK mp3tagproc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		uSetWindowLong(wnd,DWL_USER,lp);
		{
			tagging_dialogparam * param = reinterpret_cast<tagging_dialogparam*>(lp);
			if (param->m_id3v1 != BST_INDETERMINATE) TogglewindowStyle(GetDlgItem(wnd,IDC_ID3v1),BS_3STATE);
			uSendDlgItemMessage(wnd,IDC_ID3v1,BM_SETCHECK,param->m_id3v1,0);
			if (param->m_id3v2 != BST_INDETERMINATE) TogglewindowStyle(GetDlgItem(wnd,IDC_ID3v2),BS_3STATE);
			uSendDlgItemMessage(wnd,IDC_ID3v2,BM_SETCHECK,param->m_id3v2,0);
			if (param->m_apev2 != BST_INDETERMINATE) TogglewindowStyle(GetDlgItem(wnd,IDC_APEv2),BS_3STATE);
			uSendDlgItemMessage(wnd,IDC_APEv2,BM_SETCHECK,param->m_apev2,0);

			uSetWindowText(wnd,uStringPrintf("MP3 tag types (%u files)",param->m_count));
		}
		return TRUE;
	case WM_COMMAND:
		switch(wp)
		{
		case IDOK:
			{
				tagging_dialogparam * param = reinterpret_cast<tagging_dialogparam*>(uGetWindowLong(wnd,DWL_USER));
				param->m_id3v1 = uSendDlgItemMessage(wnd,IDC_ID3v1,BM_GETCHECK,0,0);
				param->m_id3v2 = uSendDlgItemMessage(wnd,IDC_ID3v2,BM_GETCHECK,0,0);
				param->m_apev2 = uSendDlgItemMessage(wnd,IDC_APEv2,BM_GETCHECK,0,0);
			}
			EndDialog(wnd,1);
			return TRUE;
		case IDCANCEL:
			EndDialog(wnd,0);
			return TRUE;
		default:
			return FALSE;
		}
	default:
		return FALSE;
	}
}



static void process_tagging(const list_base_const_t<metadb_handle_ptr> & p_data)
{
	tagging_dialogparam param = {-1,-1,-1, p_data.get_count()};
	unsigned counted = 0;
	
	
	{
		in_metadb_sync l_sync;

		unsigned n; const unsigned m = p_data.get_count();
		for(n=0;n<m;n++)
		{
			const file_info * infoptr;
			if (p_data[n]->get_info_locked(infoptr))
			{
				bool have_id3v1 = false,have_id3v2 = false,have_apev2 = false;
				counted++;
				input_mp3::g_parse_tagtype(*infoptr,have_id3v1,have_id3v2,have_apev2);
				process_tagstate(have_id3v1,param.m_id3v1);
				process_tagstate(have_id3v2,param.m_id3v2);
				process_tagstate(have_apev2,param.m_apev2);
			}
		}
	}

	if (counted > 0)
	{
		if (uDialogBox(IDD_MP3TAG,core_api::get_main_window(),mp3tagproc,(long)&param) > 0)
		{
			file_info_update_helper helper(p_data);
			if (helper.read_infos(core_api::get_main_window(),true))
			{
				unsigned item_ptr; const unsigned item_max = helper.get_item_count();
				for(item_ptr=0;item_ptr<item_max;item_ptr++)
				{
					file_info & info = helper.get_item(item_ptr);
					bool have_id3v1 = false,have_id3v2 = false,have_apev2 = false;
					input_mp3::g_parse_tagtype(info,have_id3v1,have_id3v2,have_apev2);
					string8 tagtype_new;
					bool changed = false;
					if (process_tagstate_v2(have_id3v2,param.m_id3v2,changed)) tagtype_new += "id3v2";
					if (process_tagstate_v2(have_apev2,param.m_apev2,changed))
					{
						if (!tagtype_new.is_empty()) tagtype_new += "|";
						tagtype_new += "apev2";
					}
					if (process_tagstate_v2(have_id3v1,param.m_id3v1,changed))
					{
						if (!tagtype_new.is_empty()) tagtype_new += "|";
						tagtype_new += "id3v1";
					}

					if (!changed) helper.invalidate_item(item_ptr);
					else info.info_set("tagtype_override",tagtype_new);
				}
				helper.write_infos(core_api::get_main_window(),true);
			}
		}
	}
}

class menu_item_headerfix : public menu_item_legacy_context
{

	unsigned get_num_items() {return 2;}
	
	void get_item_name(unsigned n,string_base & out)
	{
		switch(n)
		{
		case 0:
			out = "Fix MP3 header...";
			break;
		case 1:
			out = "MP3 tag types...";
			break;
		}
	}
	
	void get_item_default_path(unsigned n,string_base & out)
	{
		switch(n)
		{
		case 0:
			out = "Utils";
			break;
		case 1:
			out = "Tagging";
			break;
		}
		
		
	}
	
	bool get_item_description(unsigned n,string_base & out)
	{
		switch(n)
		{
		case 0:
			out = "Edits header of MP3 file (advanced).";
			return true;
		case 1:
			out = "Changes type of tags in MP3 file (addvanced).";
			return true;
		default:
			return false;
		}
	}
	
	void context_command(unsigned n,const list_base_const_t<metadb_handle_ptr> & p_data,const GUID&)
	{
		switch(n)
		{
		case 0:
			process_headerfix(p_data);
			break;
		case 1:
			process_tagging(p_data);
			break;
		}
	}

	virtual bool context_get_display(unsigned n,const list_base_const_t<metadb_handle_ptr> & p_data,string_base & out,unsigned&,const GUID&)
	{
		bool rv = false;
		switch(n)
		{
		case 0:
			if (p_data.get_count()==1 && is_mp3_file(p_data[0]))
			{
				rv = true;
				out = "Fix MP3 header...";
			}
			break;
		case 1:
			if (p_data.get_count() > 0)
			{
				unsigned n; const unsigned m = p_data.get_count();
				bool abort = false;
				for(n=0;n<m;n++)
				{
					if (!is_mp3_file(p_data[n])) {abort = true; break; }
				}
				if (!abort)
				{
					rv = true;
					out = "MP3 tag types...";
				}
			}

			break;
		}
		return rv;
	}

	GUID get_item_guid(unsigned n)
	{
		static const GUID guids[] = {
			{ 0x79f79224, 0x50e7, 0x4d68, { 0x8b, 0x48, 0x3, 0xee, 0xdd, 0x9b, 0x41, 0xef } },
			{ 0xc62fa43d, 0x6823, 0x4703, { 0x92, 0x1e, 0x67, 0x5c, 0x2e, 0x34, 0x28, 0x91 } }
		};
		return guids[n];

	}
};

static menu_item_factory_t<menu_item_headerfix> g_menu_item_headerfix;

namespace blah {
DECLARE_FILE_TYPE("MPEG layer 3","*.MP3");
}

namespace bleh {
DECLARE_FILE_TYPE("MPEG layer 2","*.MP2");
}