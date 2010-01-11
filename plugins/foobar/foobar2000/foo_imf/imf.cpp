// changelog

// 2003-06-26 07:05 - kode54
// - Updated to 0.7 API
// - Version is now 0.2

// 2003-04-12 23:39 - kode54
// - added bitspersample info

#include "../SDK/foobar2000.h"

#include "../../fmopl.h"

#include "imfcrc.h"

#define MASTER_CLOCK        (3579545)
//#define MASTER_CLOCK (3600000)

static const int srate = MASTER_CLOCK / 72. + .5;

// {DEE39839-DB32-489b-AD01-5DCFFC8E24EB}
/*static const GUID guid_cfg_srate = 
{ 0xdee39839, 0xdb32, 0x489b, { 0xad, 0x1, 0x5d, 0xcf, 0xfc, 0x8e, 0x24, 0xeb } };*/

//static cfg_int cfg_srate(guid_cfg_srate, 44100);

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	unsigned char reg,val;
	unsigned short time;
} Sdata;

typedef struct
{
	unsigned short reg;
	unsigned char val;
	unsigned short time;
} Xdata;
#pragma pack(pop)

static const unsigned long crctab[256] =
{
  0x0,
  0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B,
  0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6,
  0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
  0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC,
  0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F,
  0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A,
  0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
  0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58,
  0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033,
  0xA4AD16EA, 0xA06C0B5D, 0xD4326D90, 0xD0F37027, 0xDDB056FE,
  0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
  0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4,
  0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0,
  0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5,
  0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
  0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 0x7897AB07,
  0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C,
  0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1,
  0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
  0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B,
  0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698,
  0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D,
  0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
  0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F,
  0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34,
  0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80,
  0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
  0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A,
  0x58C1663D, 0x558240E4, 0x51435D53, 0x251D3B9E, 0x21DC2629,
  0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C,
  0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF,
  0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E,
  0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65,
  0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8,
  0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
  0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2,
  0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71,
  0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74,
  0x857130C3, 0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
  0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21,
  0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A,
  0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 0x18197087,
  0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC,
  0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D,
  0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE,
  0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB,
  0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
  0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 0x89B8FD09,
  0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662,
  0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF,
  0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4
};

static unsigned long crc32(unsigned char *buf, unsigned long size)
{
	unsigned long crc=0,i;

	for(i=0;i<size;i++)
		crc = (crc << 8) ^ crctab[((crc >> 24) ^ buf[i]) & 0xFF];

	for(i=size;i>0;i>>=8)
		crc = (crc << 8) ^ crctab[((crc >> 24) ^ i) & 0xFF];

	crc = ~crc & 0xFFFFFFFF;
	return crc;
}

static float getrate(unsigned long crc, unsigned long size)
{
	unsigned int i;

	for(i=0;filetab[i].size;i++)
		if(crc == filetab[i].crc && size == filetab[i].size)
			return filetab[i].rate;

	return 0.f;
}

class input_imf
{
	pfc::array_t<Xdata> imf_data;

	Xdata *pLog;
	float TicksPerSecond;
	//UINT srate;
	int LogCount;
	int CurrentLog;
	int CurrentTick;
	int remainder;

	int Chips;

	YMF * chip;

	t_filestats m_stats;

	bool no_loop, extended;

	void extend()
	{
		Sdata * in = (Sdata*) imf_data.get_ptr();
		Xdata * out = (Xdata*) in;
		for (int i = LogCount - 1; i >= 0; i--)
		{
			out[i].time = byte_order::word_le_to_native( in[i].time );
			out[i].val = in[i].val;
			out[i].reg = in[i].reg;
		}
	}

	void swap()
	{
		Xdata * ptr = imf_data.get_ptr();
		for ( int i = 0; i < LogCount; ++i )
		{
			ptr[ i ].reg = byte_order::word_le_to_native( ptr[ i ].reg );
			ptr[ i ].time = byte_order::word_le_to_native( ptr[ i ].time );
		}
	}

	double get_length()
	{
		Xdata * pLog = imf_data.get_ptr();
		int ticks = 0;
		for (int i = 0; i < LogCount; i++)
		{
			ticks += pLog->time;
			pLog++;
		}
		return (double)ticks / TicksPerSecond;
	}

	void render(audio_sample * ptr, int samples)
	{
		for (int c = 0; c < Chips; c++)
		{
			chip[c].Update(ptr, samples);
		}
	}

	void write(int reg, int val)
	{
		int c = reg >> 8;
		chip[c].Write(reg & 0xFF, val);
	}

	void reset()
	{
		for (int i = 0; i < Chips; i++)
		{
			chip[i].Reset();
			chip[i].Write(1, 32);
		}
	}

public:

	input_imf()
	{
		chip = NULL;
	}

	~input_imf()
	{
		if (chip)
		{
			delete [] chip;
		}
	}

	void open( service_ptr_t<file> p_file,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( p_file.is_empty() )
		{
			filesystem::g_open( p_file, p_path, filesystem::open_mode_read, p_abort );
		}

		m_stats = p_file->get_stats( p_abort );

		if ( m_stats.m_size > (1 << 30)) throw exception_io_data();

		//srate = cfg_srate;

		{
			unsigned wanted;

			union
			{
				t_uint8  byte;
				t_uint16 word;
			};

			//try
			{
				p_file->read_lendian_t( word, p_abort );
				LogCount = word;

				Chips = 1;
				extended = 0;

				if ( !LogCount )
				{
					TicksPerSecond = 280.f;
					LogCount = (int)(m_stats.m_size / 4);
					p_file->seek( 0, p_abort );
				}
				else
				{
					if ( LogCount == 'MI' )
					{
						unsigned char blah[9];
						p_file->read_object( &blah, 9, p_abort );
						if (blah[0] != 'F' || blah[1] != 26) throw exception_io_data();
						if ( !(blah[2] | blah[3] | blah[4] | blah[5] | blah[6] | blah[7] | blah[8]) ) throw exception_io_data();
						extended = 1;
						LogCount = blah[2] | (blah[3] << 8) | (blah[4] << 16) | (blah[5] << 24);
						TicksPerSecond = (float)(blah[6] | (blah[7] << 8));
						Chips = blah[8];
					}
					else
					{
						TicksPerSecond = 700.f;
						LogCount /= 4;
					}
				}

				imf_data.set_size( LogCount );

				pLog = imf_data.get_ptr();

				wanted = LogCount * 4;
				if (Chips > 1) wanted += LogCount;

				p_file->read_object(pLog, wanted, p_abort);
			}
			//catch(exception_io const & e) {return e.get_code();}

			if ( !extended )
			{
				float meh = getrate(crc32((unsigned char*) pLog, wanted), wanted);
				if (meh) TicksPerSecond = meh;
			}

			if (Chips == 1) extend();
			else swap();
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set_int("samplerate", srate);
		p_info.info_set_int("bitspersample", 16);
		p_info.info_set_int("channels", 1);
		p_info.info_set("codec", "Adlib register log");

		string8 temp;

		p_info.info_set("imf_ticks_per_second", format_float( TicksPerSecond, 0, 2 ) );

		p_info.set_length( get_length() );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! chip )
		{
			chip = new YMF[Chips];

			for (int i = 0; i < Chips; i++)
			{
				chip[i].init(OPL_TYPE_WAVESEL, MASTER_CLOCK, srate);
			}
		}

		for (int i = 0; i < Chips; i++)
		{
			chip[i].Reset();
			chip[i].Write(1, 32);
		}

		CurrentLog = 0;
		CurrentTick = 0;
		remainder = 0;

		no_loop = p_flags & input_flag_no_looping;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( CurrentLog == LogCount && !remainder )
		{
			if ( no_loop ) return false;
			else
			{
				CurrentLog = 0;
				CurrentTick = 0;
			}
		}

		p_chunk.check_data_size( 1024 );
		p_chunk.set_srate( srate );
		p_chunk.set_channels( 1 );

		int samples, rendered;
		float remain;
		audio_sample *p = p_chunk.get_data();

		memset( p, 0, sizeof( audio_sample ) * 1024 );

		rendered = 0;

		if (remainder)
		{
			if (remainder > 1024)
			{
				render( p, 1024 );
				remainder -= 1024;
				p_chunk.set_sample_count( 1024 );
				return true;
			}
			render(p, remainder);
			p += remainder;
			rendered = remainder;
			remainder = 0;
			if (CurrentLog == LogCount)
			{
				if (no_loop)
				{
					p_chunk.set_sample_count( rendered );
					return true;
				}
				else
				{
					CurrentLog = 0;
					CurrentTick = 0;
				}
			}
		}

		remain = 0.;

		while ( CurrentLog < LogCount )
		{
			int time;
			while ( CurrentLog < LogCount )
			{
another:
				write( pLog[CurrentLog].reg, pLog[CurrentLog].val );
				time = pLog[CurrentLog].time;
				CurrentLog++;
				if (time) break;
			}
			if (CurrentLog == LogCount)
			{
				if (no_loop) break;
				else
				{
					CurrentLog = 0;
					if (!time) goto another;
				}
			}
			CurrentTick += time;
			if (TicksPerSecond != (float)srate)
			{
				float meh = ((float)time * (float)srate / TicksPerSecond) + remain;
				samples = (int)meh;
				remain = meh - (float)samples;
			}
			else
				samples = time;
			if (samples > (1024 - rendered))
			{
				remainder = samples - (1024 - rendered);
				samples = 1024 - rendered;
				rendered = 1024;
				render(p, samples);
				break;
			}
			if (samples)
			{
				render(p, samples);
				rendered += samples;
				p += samples;
			}
		}

		if (rendered)
		{
			p_chunk.set_sample_count( rendered );
			return true;
		}
		else
			return false;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		double current = (double)CurrentTick + remainder * (float)srate / TicksPerSecond;
		p_seconds *= TicksPerSecond;
		if (current > p_seconds)
		{
			CurrentLog = 0;
			CurrentTick = 0;
			reset();
		}
		else
			p_seconds -= current;

		remainder = 0;

		while (p_seconds > 0.)
		{
			p_abort.check();

			write(pLog[CurrentLog].reg, pLog[CurrentLog].val);
			if ((double)pLog[CurrentLog].time <= p_seconds)
			{
				p_seconds -= (double)pLog[CurrentLog].time;
				CurrentLog++;
			}
			else
			{
				remainder = (int)(((double)pLog[CurrentLog].time - p_seconds) * (float)srate / TicksPerSecond);
				pfc::array_t<audio_sample> meh;
				int wanted = (int)(p_seconds * (float)srate / TicksPerSecond);
				meh.set_size( wanted );
				render( meh.get_ptr(), wanted );
				break;
			}
		}
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort )
	{
	}

	void retag( const file_info & p_info,abort_callback & p_abort )
	{
		throw exception_io_data();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "imf" );
	}
};

DECLARE_FILE_TYPE("IMF files", "*.IMF");

static input_singletrack_factory_t<input_imf> g_input_imf_factory;

DECLARE_COMPONENT_VERSION("IMF decoder", "0.2", "Plays OPL2 register logs, also known as IMF files.");
