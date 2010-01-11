/*                                    */
/* foobar2000 wrapper for SNESAPU.dll */
/*
/*        hereby public domain        */
/*                                    */

#include "Types.h"
#include "SNESAPU.h"
#include "resource.h"
#include "../SDK/foobar2000.h"

#include "dll_manager.h"

static dll_manager g_dll;

// compilation variables

const int buffer_duration = 100;			// larger values cause seeking distortion


// foobar2000-configurable variables

// {4F154DB6-1B84-47e3-8AB7-C9102E2AD997}
static const GUID guid_cfg_samplerate = 
{ 0x4f154db6, 0x1b84, 0x47e3, { 0x8a, 0xb7, 0xc9, 0x10, 0x2e, 0x2a, 0xd9, 0x97 } };
// {14892BCF-0B39-4d69-9DDD-067F4C4622F6}
static const GUID guid_cfg_bitspersample = 
{ 0x14892bcf, 0xb39, 0x4d69, { 0x9d, 0xdd, 0x6, 0x7f, 0x4c, 0x46, 0x22, 0xf6 } };
// {DC220581-492C-4504-8640-DA764B38C003}
static const GUID guid_cfg_channels = 
{ 0xdc220581, 0x492c, 0x4504, { 0x86, 0x40, 0xda, 0x76, 0x4b, 0x38, 0xc0, 0x3 } };
// {646E5772-4AE8-41a4-AB87-E5549AE626F3}
static const GUID guid_cfg_mixing = 
{ 0x646e5772, 0x4ae8, 0x41a4, { 0xab, 0x87, 0xe5, 0x54, 0x9a, 0xe6, 0x26, 0xf3 } };
// {2F249D39-E41A-48b1-9CA1-0D685A6CEA05}
static const GUID guid_cfg_interpolation = 
{ 0x2f249d39, 0xe41a, 0x48b1, { 0x9c, 0xa1, 0xd, 0x68, 0x5a, 0x6c, 0xea, 0x5 } };
// {059A68C0-7637-4da6-BC13-E37DD4945C1E}
static const GUID guid_cfg_options = 
{ 0x59a68c0, 0x7637, 0x4da6, { 0xbc, 0x13, 0xe3, 0x7d, 0xd4, 0x94, 0x5c, 0x1e } };
// {9DB61CB4-86EF-4014-B121-6C5B1F0160C7}
static const GUID guid_cfg_fastseek = 
{ 0x9db61cb4, 0x86ef, 0x4014, { 0xb1, 0x21, 0x6c, 0x5b, 0x1f, 0x1, 0x60, 0xc7 } };
// {0F730072-0304-4f26-9043-32218A79A25B}
static const GUID guid_cfg_usespclength = 
{ 0xf730072, 0x304, 0x4f26, { 0x90, 0x43, 0x32, 0x21, 0x8a, 0x79, 0xa2, 0x5b } };
// {0109DA89-AD68-4836-98E0-2852035DE0B6}
static const GUID guid_cfg_forcelength = 
{ 0x109da89, 0xad68, 0x4836, { 0x98, 0xe0, 0x28, 0x52, 0x3, 0x5d, 0xe0, 0xb6 } };
// {1E648B21-CF46-43ed-89AA-FC0A2920D625}
static const GUID guid_cfg_songlength = 
{ 0x1e648b21, 0xcf46, 0x43ed, { 0x89, 0xaa, 0xfc, 0xa, 0x29, 0x20, 0xd6, 0x25 } };
// {E54F4BB8-BDFB-4199-843B-17EC7882E0B1}
static const GUID guid_cfg_fadelength = 
{ 0xe54f4bb8, 0xbdfb, 0x4199, { 0x84, 0x3b, 0x17, 0xec, 0x78, 0x82, 0xe0, 0xb1 } };


static cfg_int cfg_samplerate		(guid_cfg_samplerate,		32000);
static cfg_int cfg_bitspersample	(guid_cfg_bitspersample,	16);
static cfg_int cfg_channels			(guid_cfg_channels,		2);
static cfg_int cfg_mixing			(guid_cfg_mixing,			MIX_INT);
static cfg_int cfg_interpolation	(guid_cfg_interpolation,	INT_GAUSS);
static cfg_int cfg_options			(guid_cfg_options,		0);

static cfg_int cfg_fastseek			(guid_cfg_fastseek,		1);

static cfg_int cfg_usespclength		(guid_cfg_usespclength,	1);
static cfg_int cfg_forcelength		(guid_cfg_forcelength,	1);
static cfg_int cfg_songlength		(guid_cfg_songlength,		180000);
static cfg_int cfg_fadelength		(guid_cfg_fadelength,		2000);


// spc file header

struct SPCHEAD
{
	s8	tag[33];							// file tag
	s8	term[3];							// tag terminator
	s8	ver;								// version #/100
	u8	pc[2];								// spc registers
	u8	a;
	u8	x;
	u8	y;
	u8	psw;
	u8	sp;
	u16	__r1;
	s8	song[32];							// song title
	s8	game[32];							// game title
	s8	dumper[16];							// name of dumper
	s8	comment[32];						// comments
	s8	date[11];							// date dumped
	s8	songlen[3];							// song length (seconds before fading)
	s8	fadelen[5];							// fade length (milliseconds)
	s8	artist[32];							// artist
	u8	chandis;							// channels disabled
	u8	emulator;							// emulator used for dump
	u8	__r2[45];
};


// our input class

class input_snesapu : public input
{
private:

	SAPUFunc	sapu;						// SNESAPU instance
	HINSTANCE	hDLL;						// SNESAPU dll handle
	u32			version, minimum, options;	// SNESAPU version info

	mem_block_t<u8> spcfile;
	mem_block_t<u8> buffer;

	u32			sapu_ticks;					// keep track of how many ms have been

	u32			sapu_samplerate;			// internal copies of cfg_* vars
	u32			sapu_bitspersample;
	u32			sapu_channels;
	u32			sapu_mixing;
	u32			sapu_interpolation;
	u32			sapu_options;

	u32			sapu_fastseek;

	u32			sapu_forcelength;
	u32			sapu_usespclength;
	u32			sapu_songlength;
	u32			sapu_fadelength;

	u32			stored_songlength;			// song length info read from spc file
	u32			stored_fadelength;

public:

	input_snesapu()
	{
		hDLL = 0;
	}

	~input_snesapu()
	{
		if (hDLL) g_dll.free(hDLL);
	}

	inline static bool g_test_filename(const char *name, const char *ext)
	{
		return !stricmp(ext, "spc");
	}

private:
	int spc_header_num_test(char *str, int len)
	{
		int	pos = 0;

		while ((pos < len) && ((str[pos] >= 0x30 && str[pos] <= 0x39) || str[pos] == '/'))
		{
			++pos;
		}

		if (pos == len || str[pos] == 0)
		{
			return pos;
		}
		else
		{
			return -1;
		}
	}

	void spc_header_read(SPCHEAD *spchead, file_info & p_info)
	{
		char spc_song[33];
		char spc_game[33];
		char spc_artist[33];
		char spc_dumper[17];
		char spc_comment[33];
		char spc_songlength[4];
		char spc_fadelength[6];

		// initialise temp strings
		spc_song[32] = 0;
		spc_game[32] = 0;
		spc_artist[32] = 0;
		spc_dumper[16] = 0;
		spc_comment[32] = 0;
		spc_songlength[32] = 0;
		spc_fadelength[32] = 0;

		memcpy(spc_song, spchead->song, 32);
		memcpy(spc_game, spchead->game, 32);
		memcpy(spc_artist, spchead->artist, 32);
		memcpy(spc_dumper, spchead->dumper, 16);
		memcpy(spc_comment, spchead->comment, 32);
		memcpy(spc_songlength, spchead->songlen, 3);
		memcpy(spc_fadelength, spchead->fadelen, 5);

		stored_songlength = atoi(spc_songlength) * 1000;
		stored_fadelength = atoi(spc_fadelength);

		// clamp song lengths
		if (stored_songlength > 959000)
		{
			stored_songlength = 959000;
		}
		if (stored_fadelength > 59999)
		{
			stored_fadelength = 59999;
		}

		// decide song length
		if (stored_songlength && sapu_usespclength)
		{
			sapu_forcelength = 1;
			sapu_songlength  = stored_songlength;
			sapu_fadelength  = stored_fadelength;

			p_info.info_set_int("spc_songlength", stored_songlength / 1000);
			p_info.info_set_int("spc_fadelength", stored_fadelength);
		}
		if (sapu_forcelength)
		{
			sapu.SetAPULength(sapu_songlength * 64, sapu_fadelength * 64);
			p_info.set_length((double)(sapu_songlength + sapu_fadelength) / 1000);
		}

		// send info to foobar2000
		if (*spc_song)
		{
			p_info.meta_add("TITLE", string_utf8_from_ansi(spc_song));
		}
		if (*spc_game)
		{
			p_info.meta_add("ALBUM", string_utf8_from_ansi(spc_game));
		}
		if (*spc_artist)
		{
			p_info.meta_add("ARTIST", string_utf8_from_ansi(spc_artist));
		}
		if (*spc_dumper)
		{
			p_info.meta_add("DUMPER", string_utf8_from_ansi(spc_dumper));
		}
		if (*spc_comment)
		{
			p_info.meta_add("COMMENT", string_utf8_from_ansi(spc_comment));
		}

		p_info.info_set_int("samplerate", sapu_samplerate);
		p_info.info_set_int("channels", sapu_channels);
		p_info.info_set_int("bitspersample", sapu_bitspersample);
	}

public:
	t_io_result get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true,false);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO),!(p_flags&OPEN_FLAG_NO_LOOPING));
	}

private:
	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info,bool p_can_loop)
	{

		if (p_decode)
		{
			// attempt SNESAPU.dll load
			hDLL = g_dll.load("SNESAPU.DLL", true);

			if (hDLL)
			{
				*(void**)&sapu.SNESAPUInfo	= (void*)GetProcAddress(hDLL, "SNESAPUInfo");
				*(void**)&sapu.LoadSPCFile	= (void*)GetProcAddress(hDLL, "LoadSPCFile");
				*(void**)&sapu.EmuAPU		= (void*)GetProcAddress(hDLL, "EmuAPU");
				*(void**)&sapu.SetAPUOpt	= (void*)GetProcAddress(hDLL, "SetAPUOpt");
				*(void**)&sapu.SetAPULength	= (void*)GetProcAddress(hDLL, "SetAPULength");
				*(void**)&sapu.SeekAPU		= (void*)GetProcAddress(hDLL, "SeekAPU");

				if (sapu.SNESAPUInfo == NULL || sapu.LoadSPCFile == NULL || sapu.EmuAPU == NULL)
				{
					console::info("SNESAPU.DLL function lookups failed.");
					return io_result_error_generic;
				}
				else
				{
					sapu.SNESAPUInfo(&version, &minimum, &options);

					if (version < 0x20000 || minimum > 0x20000)
					{
						console::info("Incompatible SNESAPU.DLL version.");
						return io_result_error_generic;
					}
				}
			}
			else
			{
				console::info("Could not locate SNESAPU.DLL.");
				return io_result_error_generic;
			}
		}

		// read spc file
		t_filesize len64;
		t_io_result rv = p_file->get_size(len64, p_abort);
		if (io_result_failed(rv)) return rv;
		if (len64 > (t_filesize)1 << 30) return io_result_error_data;
		unsigned len = (u32) len64;

		rv = p_file->read_object(spcfile.set_size(len), len, p_abort);
		if (io_result_failed(rv)) return rv;

		// load settings
		sapu_samplerate		= cfg_samplerate;
		sapu_bitspersample	= cfg_bitspersample;
		sapu_channels		= cfg_channels;
		sapu_mixing			= cfg_mixing;
		sapu_interpolation	= cfg_interpolation;
		sapu_options		= cfg_options;

		sapu_fastseek		= cfg_fastseek;

		sapu_forcelength	= cfg_forcelength;
		sapu_usespclength	= cfg_usespclength;
		sapu_songlength		= cfg_songlength;
		sapu_fadelength		= cfg_fadelength;

		if (p_decode)
		{
			// send options/file to snesapu
			sapu.SetAPUOpt(sapu_mixing, sapu_channels, sapu_bitspersample, sapu_samplerate, sapu_interpolation, sapu_options);
			sapu.LoadSPCFile(spcfile.get_ptr());

			// allocate our output buffer
			len	= (u32)(((buffer_duration * sapu_samplerate) + buffer_duration) / 1000);
			len	= len * sapu_channels * (sapu_bitspersample / 8);
			buffer.set_size(len);

			sapu_ticks = 0;
		}

		// read info from spc
		if ( p_want_info || p_decode )
		{
			spc_header_read((SPCHEAD*) spcfile.get_ptr(), p_info);
		}

		// if finite playtime is wanted, enable forced length
		if (p_can_loop)
		{
			sapu_usespclength = sapu_forcelength = 1;
		}

		return io_result_success;
	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		// decide how many samples we want from sapu
		int mswanted = buffer_duration;

		// clip samples requested if nearing song end
		if (sapu_forcelength)
		{
			if (sapu_ticks >= (sapu_songlength + sapu_fadelength))
			{
				return io_result_eof;
			}

			if ( (sapu_ticks + mswanted) > (sapu_songlength + sapu_fadelength) )
			{
				mswanted = (sapu_songlength + sapu_fadelength) - sapu_ticks;
			}
		}

		//EmuAPU() wants no. of samples to render
		sapu.EmuAPU(buffer.get_ptr(), ((mswanted * sapu_samplerate) / 1000), true);

		chunk->set_data_fixedpoint(buffer, ((mswanted * sapu_samplerate) / 1000) * sapu_channels * (sapu_bitspersample / 8), sapu_samplerate, sapu_channels, sapu_bitspersample, (sapu_channels == 2) ? audio_chunk::channel_config_stereo : audio_chunk::channel_config_mono);

		sapu_ticks += mswanted;

		return io_result_success;
	}

public:
	virtual t_io_result set_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		return io_result_error_data;
	}

	virtual t_io_result seek(double seconds,abort_callback & p_abort)
	{
		// compare desired seek pos with current pos
		u32 newpos = (u32)(seconds * 1000.0);

		// reset apu if necessary
		if (newpos < sapu_ticks)
		{
			sapu.LoadSPCFile(spcfile.get_ptr());
			sapu.SetAPULength(stored_songlength * 64, stored_fadelength * 64);
			sapu.SeekAPU(newpos * 64, sapu_fastseek);
		}
		else
		{
			sapu.SeekAPU((newpos - sapu_ticks) * 64, sapu_fastseek);
		}

		sapu_ticks = newpos;

		return io_result_success;
	}

	inline static bool g_is_our_content_type(const char *url, const char *type)
	{
		return false;
	}

	inline static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {8B8D42F4-5AD5-4400-9766-BA972371E06F}
		static const GUID guid = 
		{ 0x8b8d42f4, 0x5ad5, 0x4400, { 0x97, 0x66, 0xba, 0x97, 0x23, 0x71, 0xe0, 0x6f } };
		return guid;
	}

	static const char * g_get_name() {return "SNESAPU SPC decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

static input_factory_t<input_snesapu> foo;


// our config class

class preferences_page_snesapu : public preferences_page
{
	// combo populator helper

	static void combo_populate(HWND hwnd, int combo, const char **names, int *vals, int len, int def)
	{
		int i;
		int index;
		int defindex = 0;

		uSendDlgItemMessage(hwnd, combo, CB_RESETCONTENT, 0, 0);

		for (i=0 ; i<len ; ++i)
		{
			index = uSendDlgItemMessageText(hwnd, combo, CB_INSERTSTRING, -1, names[i]);
			uSendDlgItemMessage(hwnd, combo, CB_SETITEMDATA, index, vals[i]);

			if (vals[i] == def)
			{
				defindex = i;
			}
		}

		uSendDlgItemMessage(hwnd, combo, CB_SETCURSEL, defindex, -1);
	}


	// combobox setting helper

#define COMBOGET(X) \
	if (HIWORD(wparam) == CBN_SELCHANGE) \
	{ \
		data = uSendMessage((HWND)lparam, CB_GETCURSEL, 0, 0); \
		data = uSendMessage((HWND)lparam, CB_GETITEMDATA, data, 0); \
		if (data != CB_ERR) \
		{ \
			X = data; \
		} \
	} \

	// checkbox setting helpers

#define DSPCHECKGET(X) \
	if (uSendMessage((HWND)lparam, BM_GETCHECK, 0, 0) == BST_CHECKED) \
	{ \
		cfg_options = cfg_options | X; \
	} \
	else if (cfg_options & X) \
	{ \
		cfg_options = cfg_options & ~X; \
	}

#define CHECKGET(X) \
	if (uSendMessage((HWND)lparam, BM_GETCHECK, 0, 0) == BST_CHECKED) \
	{ \
		X = 1; \
	} \
	else \
	{ \
		X = 0; \
	}

	// configuration window procedure

	static BOOL CALLBACK ConfigProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		int data;

		switch (msg)
		{
		case WM_INITDIALOG:
			{
				// populate comboboxes
				const char *sampleratenames[] = {"8000","11025","16000","22050","24000","32000","44100","48000","64000","88200","96000"};
				int samplerates[] =				{ 8000,  11025,  16000,  22050,  24000,  32000,  44100,  48000,  64000,  88200,  96000};

				const char *mixingnames[] =		{"None (faulty?)","Integer","Float"};
				int mixingmodes[] =				{ 0,	 1,		   2};

				const char *internames[] =		{"None","Linear","Cubic","Gaussian"};
				int intermodes[] =				{ 0,	 1,		  2,	  3};

				const char *bitnames[] =		{"8","16","24","32"};
				int bitmodes[] =				{ 8,  16,  24,  32};

				const char *channames[] =		{"Mono","Stereo"};
				int chanmodes[] =				{ 1,	 2};

				combo_populate(hwnd, IDC_SAMPLERATE,	sampleratenames, samplerates, 11, cfg_samplerate);
				combo_populate(hwnd, IDC_MIXING,		mixingnames,	 mixingmodes, 3,  cfg_mixing);
				combo_populate(hwnd, IDC_INTERPOLATION, internames,		 intermodes,  4,  cfg_interpolation);
				combo_populate(hwnd, IDC_BITS,			bitnames,		 bitmodes,	  4,  cfg_bitspersample);
				combo_populate(hwnd, IDC_CHANNELS,		channames,		 chanmodes,   2,  cfg_channels);

				// populate dsp options
				int dspopts[][2] = {
					{IDC_DSP_ANALOG,	DSP_ANALOG},
					{IDC_DSP_OLDADPCM,	DSP_OLDSMP},
					{IDC_DSP_SURROUND,	DSP_SURND},
					{IDC_DSP_REVERSE,	DSP_REVERSE},
					{IDC_DSP_DISECHO,	DSP_NOECHO}};

					for (int i=0 ; i<5 ; ++i)
					{
						uSendDlgItemMessage(hwnd, dspopts[i][0], BM_SETCHECK, cfg_options & dspopts[i][1], 0);
					}

					// populate song length settings
					uSendDlgItemMessage(hwnd, IDC_FORCELENGTH,  BM_SETCHECK, cfg_forcelength, 0);
					uSendDlgItemMessage(hwnd, IDC_USESPCLENGTH,  BM_SETCHECK, cfg_usespclength, 0);

					uSetDlgItemText(hwnd, IDC_SONGLENGTH, uStringPrintf("%u", (int)cfg_songlength));

					uSetDlgItemText(hwnd, IDC_FADELENGTH, uStringPrintf("%u", (int)cfg_fadelength));

					// populate misc options
					uSendDlgItemMessage(hwnd, IDC_FASTSEEK,  BM_SETCHECK, cfg_fastseek, 0);

					return 1;
			}

		case WM_COMMAND:
			{
				switch (LOWORD(wparam))
				{
				case IDC_SAMPLERATE:
					{
						COMBOGET(cfg_samplerate)
							break;
					}

				case IDC_MIXING:
					{
						COMBOGET(cfg_mixing)
							break;
					}

				case IDC_INTERPOLATION:
					{
						COMBOGET(cfg_interpolation)
							break;
					}

				case IDC_BITS:
					{
						COMBOGET(cfg_bitspersample)
							break;
					}

				case IDC_CHANNELS:
					{
						COMBOGET(cfg_channels)
							break;
					}

				case IDC_DSP_ANALOG:
					{
						DSPCHECKGET(DSP_ANALOG)
							break;
					}

				case IDC_DSP_OLDADPCM:
					{
						DSPCHECKGET(DSP_OLDSMP)
							break;
					}

				case IDC_DSP_SURROUND:
					{
						DSPCHECKGET(DSP_SURND)
							break;
					}

				case IDC_DSP_REVERSE:
					{
						DSPCHECKGET(DSP_REVERSE)
							break;
					}

				case IDC_DSP_DISECHO:
					{
						DSPCHECKGET(DSP_NOECHO)
							break;
					}

				case IDC_USESPCLENGTH:
					{
						CHECKGET(cfg_usespclength)
							break;
					}

				case IDC_FORCELENGTH:
					{
						CHECKGET(cfg_forcelength)
							break;
					}

				case IDC_SONGLENGTH:
					{
						if (HIWORD(wparam) == EN_CHANGE)
						{
							data = atoi(string_utf8_from_window((HWND)lparam));

							if (data > 0)
							{
								cfg_songlength = data;
							}
						}
						break;
					}

				case IDC_FADELENGTH:
					{
						if (HIWORD(wparam) == EN_CHANGE)
						{
							data = atoi(string_utf8_from_window((HWND)lparam));

							if (data > 0)
							{
								cfg_fadelength = data;
							}
						}
						break;
					}

				case IDC_FASTSEEK:
					{
						CHECKGET(cfg_fastseek)
							break;
					}
				}
			}
		}

		return 0;
	}

public:

	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}

	GUID get_guid()
	{
		// {F7DCE239-1DE6-454e-ACDA-E341EF85718B}
		static const GUID guid = 
		{ 0xf7dce239, 0x1de6, 0x454e, { 0xac, 0xda, 0xe3, 0x41, 0xef, 0x85, 0x71, 0x8b } };
		return guid;
	}

	virtual const char *get_name()
	{
		return "SNESAPU SPC Decoder";
	}

	GUID get_parent_guid() {return guid_input;}

	bool reset_query() {return true;}
	void reset()
	{
		cfg_samplerate = 32000;
		cfg_bitspersample = 16;
		cfg_channels = 2;
		cfg_mixing = MIX_INT;
		cfg_interpolation = INT_GAUSS;
		cfg_options = 0;

		cfg_fastseek = 1;

		cfg_usespclength = 1;
		cfg_forcelength = 1;
		cfg_songlength = 180000;
		cfg_fadelength = 2000;
	}
};

static preferences_page_factory_t<preferences_page_snesapu> foo_snesapu_config;


// version info

DECLARE_COMPONENT_VERSION("SNESAPU SPC Decoder",
						  "0.02",
						  "Uses SNESAPU.dll for SPC decoding.")

DECLARE_FILE_TYPE("SPC files", "*.SPC");
