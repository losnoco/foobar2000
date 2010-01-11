#include "../SDK/foobar2000.h"
#include <mmreg.h>
#include "ms/kssample.h"
#include "resource.h"

static cfg_int cfg_buf_size("ks_buf_size",1000);
static cfg_string cfg_device_name("ks_device_name","");
static cfg_int cfg_whined("whined",0);

static bool test_device_name(const TCHAR * ptr)
{
	return 1;
}

static CKsAudRenFilter * create_filter(CKsEnumFilters * p_enum)
{
	CKsAudRenFilter * p_filter = 0;
	if (cfg_device_name[0]==0)
	{
		LISTPOS lp = p_enum->m_listFilters.GetHeadPosition();
		while(p_enum->m_listFilters.GetNext(lp, (CKsFilter**)&p_filter))
		{
			if (p_filter==0) break;
			const TCHAR * p_name = p_filter->GetName();
			if (test_device_name(p_name)) break;
			p_filter = 0;
		}
	}
	else
	{
		string_os_from_utf8 name(cfg_device_name);
		LISTPOS lp = p_enum->m_listFilters.GetHeadPosition();
		while(p_enum->m_listFilters.GetNext(lp, (CKsFilter**)&p_filter))
		{
			if (p_filter==0) break;
			const TCHAR * p_name = p_filter->GetName();
			if (test_device_name(p_name) && !_tcscmp(p_name,name)) break;
			p_filter = 0;
		}
	}
	return p_filter;
}

class output_ks : public output_i_base
{
	struct PACKET
	{
		KSSTREAM_HEADER header;
		OVERLAPPED sync;
	};
	mem_block_t<char> buffer;
	int buffer_size_bytes;
	int packet_size_bytes,n_packets,cur_packet,packet_data_ptr;
	int m_bps,m_nch;

	__int64 total_written;

	int paused;

	mem_block_t<PACKET> packets;

	HRESULT hr;
	CKsAudRenFilter* p_filter;
	CKsAudRenPin* p_pin;
	CKsEnumFilters * p_enum;

	KSSTATE ks_state;

	mem_block_t<HANDLE> events;

	virtual int open_ex(int srate,int bps,int nch,int format_code);


	int write(const char *buf, int len);
		
	virtual int do_flush() {return 1;} //return 1 if you need reopen, 0 if successfully flushed

	int can_write()
	{
		if (paused) return 0;
		else
		{
			int cw = 0;
			int n;
			for(n=0;n<n_packets;n++)
			{
				if (WaitForSingleObject(events[n],0)==WAIT_OBJECT_0) cw+=packet_size_bytes;
			}
			cw-=packet_data_ptr;
			return cw;
		}
	}


	void close();
	int output_packet();

public:
	output_ks()
	{
		paused = 0;
		hr = S_OK;
		p_filter = 0;
		p_pin = 0;
		ks_state = KSSTATE_STOP;
		p_enum = 0;
		m_nch = m_bps = 0;
	}

	~output_ks()
	{
		close();
		
	}
	virtual int get_latency_bytes()
	{
		KSAUDIO_POSITION pos;
		if (p_pin==0) return 0;
		if (FAILED(p_pin->GetPosition(&pos))) return 0;
		int delta = (int)(total_written - (__int64)pos.PlayOffset);
		if (delta<0) delta=0;
		if (delta==0)
		{
			int done = 0;
			int n;
			for(n=0;n<n_packets;n++)
			{
				if (WaitForSingleObject(events[n],0)==WAIT_OBJECT_0) done++;
			}
			if (done<n_packets) delta = (m_bps/8)*m_nch;
		}
		return delta;
	}

	virtual void pause(int state)
	{
		paused=state;

		if (p_pin)
		{
			if(state)	// pausing
			{
				if (ks_state == KSSTATE_STOP)
				{
					for (int i = 0; i < n_packets; i++)
						SetEvent(events[i]);
				}

				ks_state = KSSTATE_PAUSE;
				hr = p_pin->SetState(ks_state);
			}
			else	// unpausing
			{
				if (ks_state == KSSTATE_STOP)
				{
					for (int i = 0; i < n_packets; i++)
						SetEvent(events[i]);
				}

				ks_state = KSSTATE_RUN;
				hr = p_pin->SetState(ks_state);
			}
		}
	}
	virtual void force_play()
	{
		if (packet_data_ptr>0)
		{
			output_packet();
		}
	}

	virtual GUID get_guid()
	{
		// {2C1B1FDD-3595-4da1-BC2A-F77334D36EFE}
		static const GUID guid = 
		{ 0x2c1b1fdd, 0x3595, 0x4da1, { 0xbc, 0x2a, 0xf7, 0x73, 0x34, 0xd3, 0x6e, 0xfe } };
		return guid;
	}
	virtual const char * get_name() {return "Kernel Streaming";}

	virtual const char * get_config_page_name() {return "Kernel Streaming";}

};

int output_ks::output_packet()
{
	if (p_pin==0) return 0;
	if (WaitForSingleObject(events[cur_packet],0)!=WAIT_OBJECT_0) return 0;
	packets[cur_packet].header.FrameExtent = packet_data_ptr;
	packets[cur_packet].header.DataUsed = packet_data_ptr;

	ResetEvent(packets[cur_packet].sync.hEvent);

	if (FAILED(p_pin->WriteData(&packets[cur_packet].header, &packets[cur_packet].sync))) return 0;
	packet_data_ptr = 0;
	cur_packet = (cur_packet+1) % n_packets;
	
	return 1;

}

int output_ks::write(const char *buf, int len)
{
	while(len>0)
	{
		int d = packet_size_bytes - packet_data_ptr;
		if (d>len) d = len;

		memcpy((char*)packets[cur_packet].header.Data + packet_data_ptr, buf, d);
		buf += d;
		len -= d;
		packet_data_ptr += d;

		if (packet_data_ptr == packet_size_bytes)
		{
			if (!output_packet()) return 0;
		}

		total_written += d;
	}
	return 1;
}

static CKsEnumFilters * enum_create()
{
	HRESULT hr = 0;
	CKsEnumFilters* p_enum = new CKsEnumFilters(&hr);
	if (FAILED(hr))
	{
		if (p_enum) {delete p_enum;p_enum=0;}
	}
	if (p_enum)
	{
		GUID  aguidEnumCats[] = { STATIC_KSCATEGORY_AUDIO, STATIC_KSCATEGORY_RENDER };

		if (FAILED(p_enum->EnumFilters
			(
				eAudRen,            // create audio render filters ...
				aguidEnumCats,      // ... of these categories
				2,                  // There are 2 categories
				TRUE,               // While you're at it, enumerate the pins
				FALSE,              // ... but don't bother with nodes
				TRUE                // Instantiate the filters
			)))
		{
			delete p_enum;
			p_enum = 0;
		}
	}
	return p_enum;
}

static DWORD get_channel_mask(int nch)
{
	DWORD rv;
	switch(nch)
	{
	default: 
		rv = 0;
		break;
	case 1:
		rv = KSAUDIO_SPEAKER_MONO;
		break;
	case 2:
		rv = KSAUDIO_SPEAKER_STEREO;
		break;
	case 4:
		rv = KSAUDIO_SPEAKER_QUAD;
		break;
	case 6:
		rv = KSAUDIO_SPEAKER_5POINT1;
		break;
	}
	return rv;
}

int output_ks::open_ex(int srate,int bps,int nch,int format_code)
{
	close();

	if (format_code!=DATA_FORMAT_LINEAR_PCM && format_code!=DATA_FORMAT_IEEE_FLOAT) return 0;

	if (!cfg_whined)
	{
		cfg_whined = 1;
		console::error("one-time reminder: kernel streaming output is EXPERIMENTAL, use it at your own risk; using it may lead to system instability on certain configurations. if you encounter any problems while using kernel streaming output, please switch back to directsound output.");
		return 0;
	}


	bool use_float = format_code == DATA_FORMAT_IEEE_FLOAT;

	{
		enum {
			min_packet_samples = 128,
			max_packet_bytes = 0x2000,
		};
		int buf_size_samples = MulDiv(cfg_buf_size,srate,1000);
		int max_packet_samples = max_packet_bytes / (bps/8 * nch);

		int packet_samples = buf_size_samples / 16;
		
		if (packet_samples < min_packet_samples) packet_samples = min_packet_samples;
		else if (packet_samples > max_packet_samples) packet_samples = max_packet_samples;

		packet_size_bytes = packet_samples * (bps/8) * nch;

		n_packets = buf_size_samples / packet_samples;
		if (n_packets<4) n_packets = 4;

		buffer_size_bytes = n_packets * packet_size_bytes;
		buffer.set_size(buffer_size_bytes);


		buffer.zeromemory();
		
		packets.set_size(n_packets);
		packets.zeromemory();

		events.set_size(n_packets);
		events.zeromemory();

		total_written = 0;
		cur_packet = 0;
		packet_data_ptr = 0;
	}


	// enumerate audio renderers
	p_enum = enum_create();
	if (p_enum==0) return 0;

	p_filter = create_filter(p_enum);

	if(p_filter == NULL)
	{
		set_error("KS output error: unable to find filter corresponding to selected device.");
		return 0;
	}

	int valid_bps = bps;
	{
		// fuckety fuck fuck
		service_enum_t<config_var> e;
		config_var * ptr;
		for (ptr=e.first();ptr;ptr=e.next())
		{
			uOutputDebugString((const char *) string_printf("%s: %s%s", ptr->get_library(), ptr->get_name(), ptr->is_public() ? " (public)" : ""));
			if (!strcmp(ptr->get_library(), "CORE") && ptr->get_type() == config_var_int::get_var_type() && !strcmp(ptr->get_name(), "bps_bits"))
			{
				config_var_int * bits = static_cast<config_var_int*>(ptr);
				valid_bps = bits->get_value();
				bits->service_release();
				uOutputDebugString(string_printf("Value found: %d", valid_bps));
				break;
			}
			ptr->service_release();
		}
	}

	// instantiate the pin
	// use WAVEFORMATEXTENSIBLE to describe wave format
	WAVEFORMATEXTENSIBLE wfx;
	wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	wfx.Format.nChannels = nch;
	wfx.Format.nSamplesPerSec = srate;
	wfx.Format.nBlockAlign = nch*(bps/8);
	wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
	wfx.Format.wBitsPerSample = bps;
	wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
	wfx.Samples.wValidBitsPerSample = valid_bps;
	wfx.SubFormat = use_float ? KSDATAFORMAT_SUBTYPE_IEEE_FLOAT : KSDATAFORMAT_SUBTYPE_PCM;
	wfx.dwChannelMask = get_channel_mask(nch);
	p_pin = p_filter->CreateRenderPin(&wfx.Format, FALSE);

	if (!p_pin)
	{
		// driver can't handle WAVEFORMATEXTENSIBLE, so fall back to
		// WAVEFORMATEX format descriptor and try again
		wfx.Format.wFormatTag = use_float ? WAVE_FORMAT_IEEE_FLOAT : WAVE_FORMAT_PCM;
		// set unused members to zero
		wfx.Format.cbSize = 0;
		wfx.Samples.wValidBitsPerSample = 0;
		wfx.dwChannelMask = 0;
		wfx.SubFormat = GUID_NULL;

		p_pin = p_filter->CreateRenderPin(&wfx.Format, FALSE);

		if (!p_pin)
		{
			set_error("KS output error: error opening device.");
			return 0;
		}
	}

	

	// set up the data packets
	for(int i = 0; i < n_packets; i++)
	{
		events[i] = CreateEvent(NULL, TRUE, TRUE, NULL);

		packets[i].sync.hEvent = events[i];
		packets[i].header.Data = buffer  + i * packet_size_bytes;
		packets[i].header.FrameExtent = packet_size_bytes;
		packets[i].header.DataUsed = packet_size_bytes;
		packets[i].header.Size = sizeof(packets[i].header);
		packets[i].header.PresentationTime.Numerator = 1;
		packets[i].header.PresentationTime.Denominator = 1;
	}


	// set new state
	ks_state = KSSTATE_RUN;
	hr = p_pin->SetState(ks_state);
	if(!SUCCEEDED(hr))
	{
		set_error("KS output error: unable to start playback.");
		return 0;
	}

	m_nch = nch;
	m_bps = bps;

	return 1;
}

void output_ks::close()
{
	if (p_pin)
	{
	// stop the pin
		hr = p_pin->SetState(KSSTATE_PAUSE);
		hr = p_pin->SetState(KSSTATE_STOP);

		ks_state = KSSTATE_STOP;

		// all done for this demo
		p_pin->ClosePin();

		p_pin = 0;

		for(int i = 0; i < n_packets; i++)
		{
			CloseHandle(events[i]);
		}

	}
	if (p_enum)
	{
		delete p_enum;
		p_enum = 0;
	}

}


static service_factory_t<output,output_ks> foo;




static void update_bufsize(HWND wnd)
{
	int val = cfg_buf_size;
	char msg[64];
	wsprintfA(msg,"%u ms",val);
	SetDlgItemTextA(wnd,IDC_BUFSIZE_DISPLAY,msg);
}

static ptr_list_t<char> damnit;

static void update_device_info(HWND wnd)
{
	CKsAudRenFilter* p_filter;
	CKsEnumFilters * p_enum;
	p_enum = enum_create();
	UINT bps = 0;
	if (p_enum)
	{
		p_filter = create_filter(p_enum);
		if (p_filter)
		{
			bps = p_filter->GetMaxBitsPerSample();
		}
		delete p_enum;
	}
	if (bps)
	{
		TCHAR temp[256];
		wsprintf(temp,TEXT("Optimal resolution: %u bits per sample"),bps);
		SetDlgItemText(wnd,IDC_OPTIMAL_RESOLUTION,temp);
	}
	else
	{
		SetDlgItemText(wnd,IDC_OPTIMAL_RESOLUTION,TEXT("Error querying device capabilities."));
	}
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			HWND list = GetDlgItem(wnd,IDC_DEVICE);

			CKsEnumFilters* p_enum = enum_create();
			if (p_enum)
			{
				CKsAudRenFilter* p_filter;
				string_os_from_utf8 name(cfg_device_name);

				LISTPOS lp = p_enum->m_listFilters.GetHeadPosition();
				while(p_enum->m_listFilters.GetNext(lp, (CKsFilter**)&p_filter))
				{
					if (p_filter==0) break;
					const TCHAR * p_name = p_filter->GetName();
					if (test_device_name(p_name))
					{
						const TCHAR * p_friendly_name = p_filter->GetFriendlyName();
						if (p_friendly_name==0) p_friendly_name = p_name;
						int idx = SendMessage(list,CB_ADDSTRING,0,(long)p_friendly_name);
						if (!_tcscmp(p_name,name)) SendMessage(list,CB_SETCURSEL,idx,0);
						damnit.add_item(strdup(string_utf8_from_os(p_name)));
					}
					p_filter = 0;
				}
				
				delete p_enum;

				if (cfg_device_name[0]==0)
				{
					SendMessage(list,CB_SETCURSEL,0,0);
				}
			}

			SendDlgItemMessage(wnd,IDC_BUFSIZE,TBM_SETRANGE,0,MAKELONG(20,10000));
			SendDlgItemMessage(wnd,IDC_BUFSIZE,TBM_SETPOS,1,cfg_buf_size);
			update_bufsize(wnd);
			update_device_info(wnd);
		}
		break;
	case WM_HSCROLL:
		cfg_buf_size = SendDlgItemMessage(wnd,IDC_BUFSIZE,TBM_GETPOS,0,0);
		update_bufsize(wnd);
		break;
	case WM_COMMAND:
		switch(wp)
		{
		case (CBN_SELCHANGE<<16)|IDC_DEVICE:
			{
				int idx = SendMessage((HWND)lp,CB_GETCURSEL,0,0);
				if (idx>=0 && (unsigned)idx<damnit.get_count())
					cfg_device_name = damnit[idx];
				update_device_info(wnd);
			}
			break;
		}
		break;
	case WM_DESTROY:
		damnit.free_all();
		break;
	}
	return 0;
}

class config_ks : public config
{
public:
	HWND create(HWND parent)
	{
		return CreateDialog(core_api::get_my_instance(),MAKEINTRESOURCE(IDD_CONFIG),parent,ConfigProc);
	}
	const char * get_name() {return "Kernel Streaming";}
	const char * get_parent_name() {return "Output";}
};

static service_factory_single_t<config,config_ks> foo2;

DECLARE_COMPONENT_VERSION("Kernel Streaming output","1.0",0);