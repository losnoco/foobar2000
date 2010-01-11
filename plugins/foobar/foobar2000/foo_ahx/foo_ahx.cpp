#define MY_VERSION "1.0"

#include <foobar2000.h>

#include <excpt.h>

#include "ahx.h"

/*
// {D659E590-C0BB-4294-A71E-CC5335B7393C}
static const GUID guid_cfg_srate = 
{ 0xd659e590, 0xc0bb, 0x4294, { 0xa7, 0x1e, 0xcc, 0x53, 0x35, 0xb7, 0x39, 0x3c } };
// {4A2BE956-6AE9-409b-B340-49DF90546196}
static const GUID guid_cfg_loop = 
{ 0x4a2be956, 0x6ae9, 0x409b, { 0xb3, 0x40, 0x49, 0xdf, 0x90, 0x54, 0x61, 0x96 } };
// {310C60DD-9885-4e99-9577-15A4F9224F75}
static const GUID guid_cfg_nch = 
{ 0x310c60dd, 0x9885, 0x4e99, { 0x95, 0x77, 0x15, 0xa4, 0xf9, 0x22, 0x4f, 0x75 } };
// {87EA595B-12B3-4ced-86B5-0865E8D2DEE0}
static const GUID guid_cfg_oversample = 
{ 0x87ea595b, 0x12b3, 0x4ced, { 0x86, 0xb5, 0x8, 0x65, 0xe8, 0xd2, 0xde, 0xe0 } };

cfg_int cfg_srate(guid_cfg_srate, 44100);
cfg_int cfg_loop(guid_cfg_loop, 0);
cfg_int cfg_nch(guid_cfg_nch, 2);
cfg_int cfg_oversample(guid_cfg_oversample, 0);
*/
const int cfg_srate = 44100, cfg_loop = 1, cfg_nch = 2, cfg_oversample = 0;

#ifdef _DEBUG
const char * g_filename;
#endif

critical_section g_waves_sync;
unsigned long    g_waves_refcount = 0;
AHXWaves       * g_waves;

class AHXFooOut : public AHXOutput
{
	int Chans;
	bool bLoop;

public:
	AHXFooOut(int nch = 1, bool Loop = false)
	{
		Player = 0;
		Chans = nch;
		bLoop = Loop;
	}

	int StartBackgroundPlay() { return 0; }
	int StopBackgroundPlay() { return 0; }
	int Stop() { return 0; }
	int Pause() { return 0; }
	int Resume() { return 0; }
	int SetVolume(int) { return 0; }

	int Init(int Frequency, int Bits, int MixLen, float Boost, int Hz)
	{
		if ( ! AHXOutput::Init(Frequency, Bits, MixLen * Chans, Boost, Hz) ) return 0;
		this->MixLen = MixLen;
		return 1;
	}

	void MixChunkStereo(int NrSamples, int** mb)
	{
		for(int v = 0; v < 4; v++)
		{
			if(Player->Voices[v].VoiceVolume == 0) continue;

			float freq = Period2Freq(Player->Voices[v].VoicePeriod);
			int delta = (int)(freq * (1 << 16) / Frequency);
			int samples_to_mix = NrSamples;
			int mixpos = (v == 0 || v == 3) ? 0 : 1;

			while(samples_to_mix)
			{
				if (MixingPos[v] > (0x280 << 16))
					MixingPos[v] -= 0x280 << 16;

				int thiscount = min(samples_to_mix, ((0x280 << 16)-MixingPos[v]-1) / delta + 1);
				samples_to_mix -= thiscount;
				int* VolTab = &VolumeTable[Player->Voices[v].VoiceVolume][128];

				//INNER LOOP
				if(Oversampling) {
					for(int i = 0; i < thiscount; i++)
					{
						int offset = MixingPos[v] >> 16;
						int sample1 = VolTab[Player->Voices[v].VoiceBuffer[offset]];
						int sample2 = VolTab[Player->Voices[v].VoiceBuffer[offset+1]];
						int frac1 = MixingPos[v] & ((1 << 16) - 1);
						int frac2 = (1 << 16) - frac1;
						(*mb)[mixpos] += ((sample1 * frac2) + (sample2 * frac1)) >> 16;
						mixpos += 2;
						MixingPos[v] += delta;
					}
				} else {
					for(int i = 0; i < thiscount; i++)
					{
						(*mb)[mixpos] += VolTab[Player->Voices[v].VoiceBuffer[MixingPos[v] >> 16]];
						mixpos += 2;
						MixingPos[v] += delta;
					}
				}
			} // while
		} // v = 0-3
		*mb += NrSamples * Chans;
	}

	void MixBuffer()
	{
	    if (!Hz || !Player->Song.SpeedMultiplier)
			return;

	    int NrSamples = Frequency / Hz / Player->Song.SpeedMultiplier;
	    int* mb = MixingBuffer;
	
		memset(MixingBuffer, 0, Chans*MixLen*Frequency/Hz*sizeof(int));
		for(int f = 0; f < MixLen*Player->Song.SpeedMultiplier /* MixLen = # frames */; f++) {
			Player->PlayIRQ();
			if (Chans == 1)
				AHXOutput::MixChunk(NrSamples, &mb);
			else
				MixChunkStereo(NrSamples, &mb);
		} // frames
	}

	int Play(AHXPlayer * Player)
	{
		if (Player)
		{
			this->Player = Player;
			return 1;
		}

		return 0;
	}

	int CopyBuffer(void * lpBuffer, int * lpValid)
	{
		if ( ! Player )
		{
			if ( lpValid ) *lpValid = 0;
			return 0;
		}

		if ( !bLoop && Player->SongEndReached )
		{
			if ( lpValid ) *lpValid = 0;
		}
		else
		{
			MixBuffer();

			int todo = Frequency / Hz * MixLen;
			if ( lpValid ) *lpValid = todo;

			if ( lpBuffer )
			{
				audio_sample * lpOut = (audio_sample *) lpBuffer;

				if (Chans == 1)
				{
					for (int i = 0; i < todo; i++)
					{
						lpOut[i] = audio_sample(MixingBuffer[i]) / audio_sample(1 << 9);
					}
				}
				else
				{
					for (int i = 0; i < todo; i++)
					{
						audio_sample l, r;
						l = audio_sample(MixingBuffer[i * 2]) / audio_sample(1 << 9);
						r = audio_sample(MixingBuffer[i * 2 + 1]) / audio_sample(1 << 9);
						lpOut[i * 2] = l * .75 + r * .25;
						lpOut[i * 2 + 1] = l * .25 + r * .75;
					}
				}
			}
		}

		return 1;
	}

	double GetLength()
	{
		if ( Player )
		{
			while ( ! Player->SongEndReached ) Player->PlayIRQ();
			unsigned frames = (Player->PlayingTime + Player->Song.SpeedMultiplier) / Player->Song.SpeedMultiplier - 1;
			return double(frames) / 50.;
		}

		return 0;
	}
};

class input_ahx
{
	AHXPlayer * m_player;
	AHXOutput * m_output;

	bool using_waves, g_can_loop, can_loop;

	int srate, nch, oversample;

	int pos, swallow;

	t_filestats m_stats;

public:
	input_ahx() : m_player(0), m_output(0), using_waves(false), g_can_loop(cfg_loop),
		srate(cfg_srate), nch(cfg_nch), oversample(cfg_oversample) {}

	~input_ahx()
	{
		if (m_output)
		{
			m_output->Free();
			delete m_output;
		}

		if (m_player) delete m_player;

		if (using_waves)
		{
			insync(g_waves_sync);
			if ( --g_waves_refcount == 0 )
			{
				delete g_waves;
			}
		}
	}

	void open( service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( p_filehint, p_path, filesystem::open_mode_read, p_abort );
		}

		m_stats = p_filehint->get_stats( p_abort );

		m_player = new AHXPlayer;

		pfc::array_t<unsigned char> song_buffer;

		t_filesize size = p_filehint->get_size_ex( p_abort );

		if ( size > 65536 ) throw exception_io_data();

		song_buffer.set_size( t_size( size ) );

		p_filehint->read_object( song_buffer.get_ptr(), t_size( size ), p_abort );

		if ( ! m_player->LoadSong( song_buffer.get_ptr(), (int)size ) ) throw exception_io_data();
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		AHXSong & Song = m_player->Song;

		if ( Song.Name[0] ) p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( Song.Name ) );

		pfc::string8_fastalloc temp;

		for ( int i = 1; i < Song.InstrumentNr + 1; i++ )
		{
			const char * s = Song.Instruments[i].Name;
			if ( * s )
			{
				temp = "inst";
				temp += pfc::format_int( i, 2 );
				p_info.meta_set( temp, pfc::stringcvt::string_utf8_from_ansi( s ) );
			}
		}

		p_info.info_set_int( "samplerate", srate );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set( "codec", "AHX" );

		p_info.info_set_int( "ahx_revision", Song.Revision );
		p_info.info_set_int( "ahx_speed_multiplier", Song.SpeedMultiplier );
		if ( Song.SubsongNr ) p_info.info_set_int( "ahx_subsongs", Song.SubsongNr );
		p_info.info_set_int( "ahx_positions", Song.PositionNr );
		p_info.info_set_int( "ahx_tracks", Song.TrackNr + 1 );
		p_info.info_set_int( "ahx_instruments", Song.InstrumentNr );

		/*m_player->Init( g_waves );
		m_player->InitSubsong( 0 );

		AHXFooOut output;
		output.Play(m_player);
		p_info.set_length(output.GetLength());*/
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		if ( ! using_waves )
		{
			using_waves = true;
			{
				insync( g_waves_sync );
				if ( g_waves_refcount++ == 0 )
				{
					g_waves = new AHXWaves;
				}
			}
		}

		pos = 0;
		swallow = 0;

		m_player->Init( g_waves );
		m_player->InitSubsong( 0 );

		can_loop = g_can_loop && ! ( p_flags & input_flag_no_looping );

		if ( m_output )
		{
			m_output->Free();
			delete m_output;
		}

		m_output = new AHXFooOut( nch, can_loop );
		m_output->Init( srate, 16, 1, 1.0f, 50 );
		m_output->SetOption( AHXOI_OVERSAMPLING, oversample );
		m_output->Play( m_player );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		p_chunk.set_data_size( srate / 50 * nch );

		int done;
		audio_sample * out = p_chunk.get_data();
		
		do
		{
			p_abort.check();

			void * vout = out;
			if ( swallow >= (srate / 50) ) vout = 0;

			int rval;

			rval = m_output->CopyBuffer( vout, & done );

			if ( ! rval ) throw exception_io_data();

			if ( ! done ) return false;

			pos += done;

			if ( swallow )
			{
				if ( swallow > done )
				{
					swallow -= done;
					done = 0;
				}
				else
				{
					if ( done - swallow )
					{
						audio_sample * foo = p_chunk.get_data();
						audio_sample * bar = foo + swallow * nch;
						memmove( foo, bar, done * nch );
					}

					done -= swallow;
					swallow = 0;
				}
			}
		}
		while ( ! done );

		p_chunk.set_srate( srate );
		p_chunk.set_channels( nch );
		p_chunk.set_sample_count( done );

		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		swallow = audio_math::time_to_samples( p_seconds, srate );
		if ( swallow > pos )
		{
			swallow -= pos;
			return;
		}
		pos = 0;
		delete m_output;

		m_player->InitSubsong( 0 );

		m_output = new AHXFooOut( nch, can_loop );
		m_output->Init( srate, 16, 1, 1.0f, 50 );
		m_output->SetOption( AHXOI_OVERSAMPLING, oversample );
		m_output->Play( m_player );
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

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_data();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return ! stricmp( p_extension, "ahx" );
	}
};

DECLARE_FILE_TYPE("Abyss AHX files", "*.AHX");

static input_singletrack_factory_t<input_ahx> g_input_ahx_factory;

DECLARE_COMPONENT_VERSION("Abyss AHX decoder", MY_VERSION, "Based on the AHX Replayer SDK v0.9, with miscellaneous fixes.\n\nAbyss homepage: http://abyss.moving-people.net\nAHX homepage: http://abyss.moving-people.net/ahxpc.html");
