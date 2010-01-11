#include "../SDK/contextmenu.h"
#include "../SDK/metadb_handle.h"
#include "../SDK/reader.h"
#include "../SDK/input_helpers.h"
#include "../SDK/resampler.h"
#include "../SDK/console.h"
#include "../SDK/cvt_float_to_linear.h"
#include "../SDK/file_info_helper.h"
#include "../SDK/config.h"
#include "../SDK/componentversion.h"
#include "../SDK/replaygain.h"
#include "../SDK/dsp_manager.h"
#include "../helpers/unicode_helper.h"
#include <commctrl.h>
#include "resource.h"

#include <NeroApi.h>
#include <NeroApiGlue.h>

static BOOL NERO_CALLBACK_ATTR EOFCallback (void *pUserData)
{
	reader * ptr = reinterpret_cast<reader*>(pUserData);
	return ptr->get_position() == ptr->get_length();
}

static BOOL NERO_CALLBACK_ATTR ErrorCallback (void *pUserData)
{
	return 0;
}

static DWORD NERO_CALLBACK_ATTR ReadIOCallback (void *pUserData, BYTE *pBuffer, DWORD dwLen)
{
	return reinterpret_cast<reader*>(pUserData)->read(pBuffer,dwLen);
}


void NERO_CALLBACK_ATTR add_log_line_callback(void *pUserData, NERO_TEXT_TYPE type, const char *text)
{
}

void NERO_CALLBACK_ATTR set_phase_callback(void *pUserData, const char *text)
{
}

void NERO_CALLBACK_ATTR disable_abort_callback(void *pUserData,BOOL abortEnabled)
{
}

/* Let the application knows in which part of the burn process NeroAPI is */
void NERO_CALLBACK_ATTR set_major_phase_callback(void *pUserData,NERO_MAJOR_PHASE phase,void *reserved)
{
}

BOOL  NERO_CALLBACK_ATTR subtask_progress_callback(void *pUserData, DWORD dwProgressInPercent)
{
	return FALSE;
}


static cfg_int cfg_dither("use_dither",1),cfg_use_rg("use_rg",0),cfg_use_dsp("use_dsp",0),cfg_use_temp_files("use_temp_files",1);
static cfg_string cfg_device("device",""),cfg_image_path("image_path","c:\\image.nrg");

static int g_instances;

// Burninate on teh fly!!@#
class decoding_reader : public reader
{
private:
	input_helper in;
	audio_chunk chunk;
	dsp_chunk_list_i chunk_list;
	cvt_float_to_linear * cvt;
	dsp * p_resampler_dsp;
	dsp_manager_old * p_dsp_manager;
	metadb_handle * handle;
	file_info_i_full info;

	int remainder, offset;
	mem_block_t<char> buffer;

	__int64 pos, len;
	bool error;

	bool use_dither;
	bool use_rg;
	bool use_dsp;

	double rg_scale;

public:
	decoding_reader()
	{
		cvt = 0;
		error = 0;
		p_resampler_dsp = 0;
		p_dsp_manager = 0;
	}

	~decoding_reader()
	{
		if (cvt) cvt->service_release();
		if (p_resampler_dsp) p_resampler_dsp->service_release();
		if (p_dsp_manager) p_dsp_manager->service_release();
	}

	int open(const char * path, int mode)
	{
		if (mode != MODE_READ) return 0;

		if (in.is_open()) return 0;

		handle = (metadb_handle *) path;

		handle->handle_query(&info);

		double l = info.get_length();

		if (!l)
		{
			string8 msg = "file has no length: ";
			msg += handle->handle_get_path();
			console::error(msg);
			error = true;
			return 0;
		}

		len = (__int64)(l * 44100.);
		len *= 4;

		use_dither = !!cfg_dither;
		use_rg = !!cfg_use_rg;
		use_dsp = !!cfg_use_dsp;

		rg_scale = 1.0;
			
		if (use_rg) rg_scale = replaygain::g_query_scale(&info);

		offset = remainder = 0;

		pos = -1;

		return 1;
	}

	int read(void *buffer, int length)
	{
		if (error) return 0;

		if (!in.is_open() && pos == -1) seek(0);

		int to_read = (int)(len - pos);

		if (to_read <= 0)
		{
			memset(buffer, 0, length);
			return length;
		}

		int read;

		if (length > to_read)
		{
			read = render(buffer, to_read);
			in.close();
			buffer = (void*) ((char*)buffer + read);
			memset(buffer, 0, length - read);
			pos += length;
			return length;
		}

		read = render(buffer, length);
		if (read < length)
		{
			in.close();
			buffer = (void*) ((char*)buffer + read);
			memset(buffer, 0, length - read);
		}

		pos += length;

		return length;
	}

	__int64 get_length()
	{
		if (error) return -1;
		if (len % 2352) return len + (2352 - (len % 2352));
		else return len;
	}

	__int64 get_position()
	{
		if (error) return -1;
		return pos;
	}

	int seek(__int64 position)
	{
		if (error || position) return 0;

		if (in.is_open())
		{
			in.close();
			if (p_resampler_dsp)
			{
				p_resampler_dsp->service_release();
				p_resampler_dsp = 0;
			}
			if (p_dsp_manager)
			{
				p_dsp_manager->service_release();
				p_dsp_manager = 0;
			}
		}

		if (!in.open(&info))
		{
			string8 msg = "error opening file : ";
			msg += info.get_file_path();
			console::error(msg);
			error = true;
			return 0;
		}

		if (!cvt) cvt = cvt_float_to_linear::get();

		in.hint(input::HINT_NO_LOOPING);
		in.hint(input::HINT_NO_SEEKING);

		if (!p_resampler_dsp) p_resampler_dsp = resampler::create(44100,resampler::FLAG_SLOW);
		if (use_dsp) p_dsp_manager = dsp_manager_old::create();

		pos = 0;

		return 1;
	}

private:
	int render(void * pbuf, int bytes)
	{
		int done = 0;
		if (remainder)
		{
			int todo = remainder;
			if (todo > bytes) todo = bytes;
			memcpy(pbuf, buffer.get_ptr() + offset, todo);
			remainder -= todo;
			bytes -= todo;
			if (remainder)
			{
				offset += todo;
				return todo;
			}
			offset = 0;
			pbuf = (void*) ((char*)pbuf + todo);
			done = todo;
		}

		while (bytes)
		{
			if (in.run(&chunk) <= 0)
			{
				if (p_resampler_dsp) p_resampler_dsp->run(&chunk_list,handle,dsp::END_OF_TRACK|dsp::FLUSH);
				if (p_dsp_manager) p_dsp_manager->run(&chunk_list,handle,dsp::END_OF_TRACK|dsp::FLUSH);
				done += dump((short*)pbuf, bytes);
				return done;
			}

			if (chunk.nch > 2)
			{
				console::error("multichannel input not supported");
				error = true;
				return done;
			}

			if (rg_scale!=1.0) dsp_util::scale_chunk(&chunk,rg_scale);

			chunk_list.add_chunk(chunk);
			if (p_dsp_manager) p_dsp_manager->run(&chunk_list,handle,0);
			if (p_resampler_dsp) p_resampler_dsp->run(&chunk_list,handle,0);
			int ret = dump((short*)pbuf, bytes);
			if (!ret)
			{
				error = true;
				break;
			}
			pbuf = (void*) ((char*)pbuf + ret);
			done += ret;
			bytes -= ret;
		}
		return done;			
	}

	int dump(short * pbuf, int bytes)
	{
		int n,m=chunk_list.get_count();
		int done = 0;
		for(n=0;n<m;n++)
		{
			audio_chunk chunk = chunk_list.get_chunk(n);
			if (chunk.srate != 44100) {console::error("unsupported sample rate"); done = 0; break;}
			else if (chunk.nch > 2) {console::error("multichannel input not supported"); done = 0; break;}
			output_chunk out_chunk;
			cvt->run(&chunk,&out_chunk,16,!!use_dither,16);
			if (out_chunk.nch==2)
			{
				const char * ptr = (const char*)out_chunk.data;
				int delta = out_chunk.size&~3;
				if (bytes)
				{
					int todo = delta;
					if (todo > bytes) todo = bytes;
					memcpy(pbuf, ptr, todo);
					pbuf = (short*) ((char*)pbuf + todo);
					ptr += todo;
					bytes -= todo;
					done += todo;
					delta -= todo;
				}
				if (delta)
				{
					char * meh = buffer.check_size(offset + remainder + delta) + offset + remainder;
					memcpy(meh, ptr, delta);
					remainder += delta;
				}
			}
			else if (out_chunk.nch==1)
			{
				const short * ptr = (const short*)out_chunk.data;
				int num = out_chunk.size / sizeof(short);
				if (bytes)
				{
					while(bytes -= 4 && num--)
					{
						*pbuf++ = *ptr;
						*pbuf++ = *ptr++;
						done += 4;
					}
				}
				
				if (num)
				{
					short * meh = (short*) (buffer.check_size(offset + remainder + num * 2 * sizeof(short)) + offset + remainder);
					
					while(num--)
					{
						*meh++ = *ptr;
						*meh++ = *ptr++;
						remainder += 4;
					}
				}
			}
		}
		chunk_list.remove_all();
		return done;
	}
};

class burner
{
	enum {MSG_THREAD_DONE = WM_APP,MSG_PROGRESS,MSG_BURN_DONE,MSG_BURN_PROGRESS};
	enum {PROGRESS_MAX = 100};
	metadb_handle_list sources;
	ptr_list_t<reader> readers;
	HWND wnd;
	HANDLE h_thread;
	bool kill,burning;
	string8 m_device;
	string8 image_path;

	void kill_thread()
	{
		if (h_thread)
		{
			kill = true;
			WaitForSingleObject(h_thread,INFINITE);
			CloseHandle(h_thread);
			h_thread = 0;
			kill = false;
		}
	}

	void on_thread();
	void on_thread_burn();
	bool burninate();

	static DWORD WINAPI ThreadProc(void* param)
	{
		reinterpret_cast<burner*>(param)->on_thread();
		return 0;
	}

	static DWORD WINAPI BurnThreadProc(void* param)
	{
		reinterpret_cast<burner*>(param)->on_thread_burn();
		return 0;
	}

	static NeroUserDlgInOut NERO_CALLBACK_ATTR NeroUserDialog (void* pUserData, NeroUserDlgInOut type, void *data)
	{
		if (pUserData)
			return reinterpret_cast<burner*>(pUserData)->on_user_dialog(type,data);
		else return DLG_RETURN_FALSE;
	}

	NeroUserDlgInOut on_user_dialog(NeroUserDlgInOut type, void *data)
	{

		switch(type)
		{
		case DLG_FILESEL_IMAGE:
			strcpy((char*)data,string_ansi_from_utf8(image_path));
			return DLG_RETURN_TRUE;
		default:
			return DLG_RETURN_TRUE;
		}
		
	}




	BOOL on_message(UINT msg,WPARAM wp,LPARAM lp);

	void burn_startup()
	{
		image_path = cfg_image_path;
		m_device = cfg_device;
		SetDlgItemText(wnd,IDC_STATIC_FILE,TEXT("Writing CD..."));
		SendDlgItemMessage(wnd,IDC_PROGRESS,PBM_SETPOS,0,0);
		burning = true;
		kill = false;
		DWORD id;
		h_thread = CreateThread(0,0,BurnThreadProc,reinterpret_cast<void*>(this),0,&id);
	}

	static BOOL NERO_CALLBACK_ATTR aborted_callback(void *pUserData)
	{
		burner * p_this = reinterpret_cast<burner*>(pUserData);
		return p_this->kill;
	}

	static BOOL NERO_CALLBACK_ATTR progress_callback(void *pUserData, DWORD dwProgressInPercent)
	{
		burner * p_this = reinterpret_cast<burner*>(pUserData);
		PostMessage(p_this->wnd,MSG_BURN_PROGRESS,dwProgressInPercent,0);
		return p_this->kill;
	}


	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		burner * ptr;
		BOOL rv = 0;
		if (msg==WM_INITDIALOG)
		{
			SetWindowLong(wnd,DWL_USER,lp);
			ptr = reinterpret_cast<burner*>(lp);
			ptr->wnd = wnd;
		}
		else
		{
			ptr = reinterpret_cast<burner*>(GetWindowLong(wnd,DWL_USER));
		}

		if (ptr) rv = ptr->on_message(msg,wp,lp);
			
		return rv;
	}
public:
	burner(const ptr_list_interface<metadb_handle> * data)
	{
		g_instances++;
		burning = false;
		wnd = 0;
		int n,m=data->get_count();
		for(n=0;n<m;n++)
		{
			sources.add_item(data->get_item(n)->handle_duplicate());
			readers.add_item((reader*)0);
		}

		CreateDialogParam(service_factory_base::get_my_instance(),MAKEINTRESOURCE(IDD_DIALOG),service_factory_base::get_main_window(),DialogProc,reinterpret_cast<long>(this));
	}
	~burner()
	{
		kill_thread();

		sources.delete_all();
		int n;
		for(n=0;n<readers.get_count();n++)
		{
			if (readers[n]) readers[n]->reader_release();
		}
		readers.remove_all();
		g_instances--;
	}

};

BOOL burner::on_message(UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case MSG_BURN_PROGRESS:
		SendDlgItemMessage(wnd,IDC_PROGRESS,PBM_SETPOS,wp,0);
		break;
	case MSG_PROGRESS:
		{
			TCHAR file_msg[256];
			wsprintf(file_msg,"Preparing files: %u of %u",wp+1,sources.get_count());
			SetDlgItemText(wnd,IDC_STATIC_FILE,file_msg);
			SendDlgItemMessage(wnd,IDC_PROGRESS,PBM_SETPOS,lp,0);
		}
		break;
	case MSG_THREAD_DONE:
		kill_thread();
		if (MessageBox(wnd,wp ? TEXT("Preparing files complete. Continue with writing ?") : TEXT("There were errors during file conversion. Continue ?"),"Message",MB_YESNO)==IDYES)
		{
			burn_startup();
		}
		else
		{
			DestroyWindow(wnd);
		}
		break;
	case MSG_BURN_DONE:
		kill_thread();
		MessageBox(wnd,wp ? TEXT("CD writing completed successfully.") : TEXT("CD writing failed."),TEXT("Message"),0);
		DestroyWindow(wnd);
		break;
	case WM_INITDIALOG:
		{
			SendDlgItemMessage(wnd,IDC_PROGRESS,PBM_SETRANGE,0,MAKELPARAM(0,PROGRESS_MAX));
			DWORD id;
			kill = false;
			h_thread = CreateThread(0,0,ThreadProc,reinterpret_cast<void*>(this),0,&id);
		}
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case IDCANCEL:
			DestroyWindow(wnd);
			break;
		}
		break;
	case WM_DESTROY:
		delete this;
		break;
	}
	return 0;
}

bool burner::burninate()
{
	if (!NeroAPIGlueConnect(0)) {console::error("Nero libraries not found. Please install Nero first.");return 0;}

	NERO_PROGRESS g_progress = 
	{
		progress_callback,
		aborted_callback,
		add_log_line_callback,
		set_phase_callback,
		reinterpret_cast<void*>(this),
		disable_abort_callback,		/* Will be called only if the NBF_DISABLE_ABORT flags is given to the NeroBurn function */
		set_major_phase_callback,	
		subtask_progress_callback,		/* provide the write buffer fill level */
	};
	
	NEROAPI_BURN_ERROR err = NEROAPI_BURN_FAILED;

	NERO_SETTINGS settings = 
	{
		NULL,
		"ahead", "Nero - Burning Rom",
		"Nero.txt",
		{0, 0},
		{NeroUserDialog, reinterpret_cast<void*>(this)}
	};
	
	NeroInit (&settings, NULL);


	NERO_DEVICEHANDLE m_NeroDeviceHandle = 0;
	NERO_SCSI_DEVICE_INFOS * m_NeroDeviceInfos = NeroGetAvailableDrivesEx(MEDIA_CD, NULL);


	if (m_NeroDeviceInfos)
	{
		unsigned int n;
		for(n=0;n<m_NeroDeviceInfos->nsdisNumDevInfos;n++)
		{
			if (m_NeroDeviceInfos->nsdisDevInfos[n].nsdiDevType==NEA_SCSI_DEVTYPE_WORM)
			{
				if (m_device.length()==0 || !strcmp(m_device,m_NeroDeviceInfos->nsdisDevInfos[n].nsdiDeviceName))
					m_NeroDeviceHandle = NeroOpenDevice (&m_NeroDeviceInfos->nsdisDevInfos[n]);
				if (m_NeroDeviceHandle) break;
			}
		}
	}

	if (m_NeroDeviceHandle)
	{
		int iSize;
		NERO_WRITE_CD * pWriteCD;
		NERO_ISO_ITEM * pItem = NULL;

		// Calculate the size required for NERO_WRITE_CD plus the given number of tracks

		iSize = sizeof (NERO_WRITE_CD) + (readers.get_count()-1) * sizeof (NERO_AUDIO_TRACK);

		// Allocate the required memory and assign it to the NERO_WRITE_CD pointer

		pWriteCD = (NERO_WRITE_CD *) new char[iSize];

		// Fill the allocated memory with null bytes

		memset (pWriteCD, 0, iSize);

		// Fill in the basic information

		pWriteCD->nwcdNumTracks = readers.get_count();
		pWriteCD->nwcdArtist = 0;
		pWriteCD->nwcdTitle = 0;
		pWriteCD->nwcdIsoTrack = 0;
		pWriteCD->nwcdCDExtra = 0;
		pWriteCD->nwcdpCDStamp = 0;
		pWriteCD->nwcdNumTracks = readers.get_count();
		pWriteCD->nwcdMediaType = MEDIA_CD;



		{
			int i;
			for (i = 0; i < readers.get_count(); i ++)
			{
				// Write continously; first track requires minimal pause of 150, though.

				if (0 == i) pWriteCD->nwcdTracks[i].natPauseInBlksBeforeThisTrack = 150;
				else pWriteCD->nwcdTracks[i].natPauseInBlksBeforeThisTrack = 0;

//				pWriteCD->nwcdTracks[i].natPauseInBlksBeforeThisTrack = 150;

				NERO_DATA_EXCHANGE* ndeShort = &(pWriteCD->nwcdTracks[i].natSourceDataExchg);

				{
					ndeShort->ndeType = NERO_ET_IO_CALLBACK;
					ndeShort->ndeData.ndeIO.nioIOCallback = ReadIOCallback;
					ndeShort->ndeData.ndeIO.nioEOFCallback = EOFCallback;
					ndeShort->ndeData.ndeIO.nioErrorCallback = ErrorCallback;
					// Calculate the length in blocks. Block size for Audio compilations is 2352 bytes.
					pWriteCD->nwcdTracks [i].natLengthInBlocks = (unsigned long)(readers[i]->get_length() / 2352);
					readers[i]->seek(0);
					ndeShort->ndeData.ndeIO.nioUserData = reinterpret_cast<void*>(readers[i]);
				}
			}

			// Perform the actual burn process

			err = NeroBurn (m_NeroDeviceHandle,
							NERO_ISO_AUDIO_MEDIA,
							pWriteCD,
							NBF_DAO|NBF_WRITE|NBF_DISABLE_EJECT,//NBF_BUF_UNDERRUN_PROT
							0,//fixme, speed
							&g_progress);

		}
		// Free allocated memory

		delete [] (char *)pWriteCD;

	}

	if (m_NeroDeviceInfos) NeroFreeMem (m_NeroDeviceInfos);

	NeroDone ();
	NeroAPIGlueDone();
	return err == NEROAPI_BURN_OK;
}

void burner::on_thread_burn()
{
	bool rv = burninate();
	if (!kill) PostMessage(wnd,MSG_BURN_DONE,!!rv,0);
}

static bool dump_chunks(dsp_chunk_list * list,cvt_float_to_linear * cvt,reader * out,bool use_dither)
{
	int n,m=list->get_count();
	bool rv = true;
	for(n=0;n<m;n++)
	{
		audio_chunk chunk = list->get_chunk(n);
		if (chunk.srate != 44100) {console::error("unsupported sample rate"); rv = false; break;}
		else if (chunk.nch > 2) {console::error("multichannel input not supported"); rv = false; break;}
		output_chunk out_chunk;
		cvt->run(&chunk,&out_chunk,16,!!use_dither,16);
		if (out_chunk.nch==2)
		{
			int delta = out_chunk.size&~3;
			if (out->write(out_chunk.data,delta)!=delta) {rv = false;break;}
		}
		else if (out_chunk.nch==1)
		{
			const short * ptr = (const short*)out_chunk.data;
			int num = out_chunk.size / sizeof(short);
			while(num--)
			{
				if (out->write(ptr,sizeof(short))!=sizeof(short)) {rv=false;break;}
				if (out->write(ptr,sizeof(short))!=sizeof(short)) {rv=false;break;}
				ptr++;
			}

		}
	}
	list->remove_all();
	return rv;
}

void burner::on_thread()
{
	bool use_dither = !!cfg_dither;
	bool error = false;
	bool use_rg = !!cfg_use_rg;
	bool use_dsp = !!cfg_use_dsp;
	bool use_temp_files = !!cfg_use_temp_files;

	dsp * p_resampler_dsp = 0;
	dsp_manager_old * p_dsp_manager = 0;

	if (use_temp_files)
	{
		dsp * p_resampler_dsp = resampler::create(44100,resampler::FLAG_SLOW);
		if (use_dsp) p_dsp_manager = dsp_manager_old::create();
	}

	int num_file;
	for(num_file = 0;num_file < sources.get_count() && !kill; num_file ++)
	{
		metadb_handle * handle = sources[num_file];
		if (use_temp_files)
		{
			reader * out = file::g_open_temp();
			readers[num_file] = out;

			input_helper in;
			audio_chunk chunk;
			dsp_chunk_list_i chunk_list;
			cvt_float_to_linear * cvt = cvt_float_to_linear::get();
			file_info_i_full info(handle->handle_get_location());
			if (!in.open(&info))
			{
				string8 msg = "error opening file : ";
				msg += handle->handle_get_path();
				console::error(msg);
				error = true;
			}
			else
			{
				in.hint(input::HINT_NO_LOOPING);
				in.hint(input::HINT_NO_SEEKING);
				double length = info.get_length();
				double position = 0;
				int last_progress = -1,new_progress;
				bool need_flush = true;
				double rg_scale = 1.0;
				
				if (use_rg) rg_scale = replaygain::g_query_scale(handle);
				
				while(!kill && in.run(&chunk)>0)
				{
					if (chunk.nch>2) {error = true;console::error("multichannel input not supported");need_flush = false;break;}
					position += (double)chunk.samples / (double)chunk.srate;
					new_progress = (int)(position * PROGRESS_MAX / length);
					if (new_progress<0) new_progress = 0;
					else if (new_progress>PROGRESS_MAX) new_progress = PROGRESS_MAX;
					if (last_progress != new_progress)
					{
						last_progress = new_progress;
						PostMessage(wnd,MSG_PROGRESS,num_file,new_progress);
					}
					
					if (rg_scale!=1.0) dsp_util::scale_chunk(&chunk,rg_scale);
					
					chunk_list.add_chunk(chunk);
					if (p_dsp_manager) p_dsp_manager->run(&chunk_list,handle,0);
					if (p_resampler_dsp) p_resampler_dsp->run(&chunk_list,handle,0);
					if (!dump_chunks(&chunk_list,cvt,out,use_dither)) {error = true;need_flush = false;break;}
				}
				if (!kill && (p_resampler_dsp || p_dsp_manager) && need_flush)
				{
					if (p_resampler_dsp) p_resampler_dsp->run(&chunk_list,handle,dsp::END_OF_TRACK|dsp::FLUSH);
					if (p_dsp_manager) p_dsp_manager->run(&chunk_list,handle,dsp::END_OF_TRACK|dsp::FLUSH);
					if (!dump_chunks(&chunk_list,cvt,out,use_dither)) {error = true;}
				}
			}
			
			{
				__int64 pos = out->get_position();
				if (pos%2352)
				{
					static const char asdf[256];
					int bytes = 2352 - (int)(pos%2352);
					while(bytes>0)
					{
						int d = bytes;
						if (d>sizeof(asdf)) d=sizeof(asdf);
						out->write(asdf,d);
						bytes -= d;
					}
				}
			}
			out->seek(0);
			cvt->service_release();
		}
		else
		{
			reader * dec = new decoding_reader;
			if (!dec->open((const char *) handle, reader::MODE_READ))
			{
				error = true;
				break;
			}
			readers[num_file] = dec;
		}
	}

	if (p_resampler_dsp) p_resampler_dsp->service_release();
	if (p_dsp_manager) p_dsp_manager->service_release();
	if (!kill) PostMessage(wnd,MSG_THREAD_DONE,!error,0);
}





class contextmenu_burn : public contextmenu
{
public:
	virtual int get_num_items() {return 1;}
	virtual const char * enum_item(int n,const ptr_list_interface<metadb_handle> * data)
	{
		if (n==0 && data->get_count()>0) return "Write Audio CD...";
		else return 0;
	}
	
	virtual void perform_command(int n,const ptr_list_interface<metadb_handle> * data)
	{
		if (n==0 && data->get_count()>0)
		{
			if (g_instances>0) console::error("Another writing process is already running.");
			else new burner(data);
		}
	}
};

static service_factory_single_t<contextmenu,contextmenu_burn> foo;

class config_burn : public config
{
	static NeroUserDlgInOut NERO_CALLBACK_ATTR NeroUserDialog (void* pUserData, NeroUserDlgInOut type, void *data)
	{
		return DLG_RETURN_FALSE;
	}

	static BOOL CALLBACK DialogProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				static ptr_list_t<char> g_device_list;
				static bool g_inited,g_have_nero;

				SendDlgItemMessage(wnd,IDC_DITHER,BM_SETCHECK,cfg_dither,0);
				SendDlgItemMessage(wnd,IDC_RG,BM_SETCHECK,cfg_use_rg,0);
				SendDlgItemMessage(wnd,IDC_DSP,BM_SETCHECK,cfg_use_dsp,0);
				SendDlgItemMessage(wnd,IDC_OTFLY,BM_SETCHECK,!cfg_use_temp_files,0);

				if (!g_inited)
				{
					if (!NeroAPIGlueConnect(0)) g_have_nero = false;
					else
					{
						g_have_nero = true;
						NERO_SETTINGS settings = 
						{
							NULL,
							"ahead", "Nero - Burning Rom",
							"Nero.txt",
							{0, 0},
							{NeroUserDialog, 0}
						};

						
						NeroInit (&settings, NULL);


						NERO_SCSI_DEVICE_INFOS * m_NeroDeviceInfos = NeroGetAvailableDrivesEx(MEDIA_CD, NULL);

						if (m_NeroDeviceInfos)
						{
							unsigned int n;
							for(n=0;n<m_NeroDeviceInfos->nsdisNumDevInfos;n++)
							{
								if (m_NeroDeviceInfos->nsdisDevInfos[n].nsdiDevType==NEA_SCSI_DEVTYPE_WORM)
									g_device_list.add_item(strdup(m_NeroDeviceInfos->nsdisDevInfos[n].nsdiDeviceName));
							}
							NeroFreeMem (m_NeroDeviceInfos);
						}
						NeroDone ();
						NeroAPIGlueDone();
					}

					g_inited = true;
				}

				if (!g_have_nero) {ShowWindow(GetDlgItem(wnd,IDC_CRUD),SW_SHOW);}
				else
				{
					string8 m_device = cfg_device;
					HWND list = GetDlgItem(wnd,IDC_DEVICE);
					int n;
					bool found = false;
					for(n=0;n<g_device_list.get_count();n++)
					{
						int idx = SendMessage(list,CB_ADDSTRING,0,(long)(const char*)string_os_from_ansi(g_device_list[n]));
						if (!found && (m_device.length()==0 || !strcmp(m_device,g_device_list[n])))
						{
							found = true;
							SendMessage(list,CB_SETCURSEL,idx,0);
						}
					}
				}

				SetDlgItemText(wnd,IDC_IMAGE_PATH,cfg_image_path);
			}
			break;
		case WM_COMMAND:
			switch(wp)
			{
			case IDC_IMAGE_PATH_BUTTON:
				{
					TCHAR path[MAX_PATH];
					GetDlgItemText(wnd,IDC_IMAGE_PATH,path,MAX_PATH);
					OPENFILENAME ofn;
					memset(&ofn,0,sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = wnd;
					ofn.lpstrFilter = TEXT("Nero CD images (*.nrg)\0*.nrg\0");
					ofn.lpstrFile = path;
					ofn.nMaxFile = MAX_PATH;
					ofn.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
					ofn.lpstrDefExt = "nrg";
					if (GetSaveFileName(&ofn))
					{
						cfg_image_path = string_utf8_from_os(path);
						SetDlgItemText(wnd,IDC_IMAGE_PATH,path);
					}
				}
				break;
			case IDC_DITHER:
				cfg_dither = SendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_RG:
				cfg_use_rg = SendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_DSP:
				cfg_use_dsp = SendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_OTFLY:
				cfg_use_temp_files = !SendMessage((HWND)lp,BM_GETCHECK,0,0);
				break;
			case IDC_DEVICE | (CBN_SELCHANGE<<16):
				{
					cfg_device = string_ansi_from_os(string_os_from_window((HWND)lp));
				}
				break;
			}
		}
		return 0;
	}
	virtual HWND create(HWND parent)
	{
		return CreateDialog(service_factory_base::get_my_instance(),MAKEINTRESOURCE(IDD_CONFIG),parent,DialogProc);
	}
	virtual const char * get_name() {return "Audio CD writer";}
	virtual const char * get_parent_name() {return "Components";}//optional, retuns name of parent config item
};

static service_factory_single_t<config,config_burn> foo2;

DECLARE_COMPONENT_VERSION("Audio CD Writer","v1.1",0)