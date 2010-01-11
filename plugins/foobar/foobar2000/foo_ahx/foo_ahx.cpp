#define MY_VERSION "1.0"

#include <foobar2000.h>

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

class input_ahx : public input
{
	AHXPlayer * m_player;
	AHXOutput * m_output;

	bool using_waves, can_loop;

	int srate, nch, oversample;

	int pos, swallow;

public:
	input_ahx() : m_player(0), m_output(0), using_waves(false), can_loop(cfg_loop),
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

	inline static bool g_test_filename(const char * full_path, const char * extension)
	{
		return !stricmp(extension, "ahx");
	}

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
		if (p_want_info || p_decode)
		{
			m_player = new AHXPlayer;
			if (!m_player) return io_result_error_generic;

			mem_block_t<unsigned char> song_buffer;

			/*if ( strstr( p_location.get_path(), "draft-hawk.ahx" ) )
			{
				console::info( "fucked file (draft-hawk.ahx)" );
				return io_result_error_data;
			}*/

			try
			{
				t_filesize size = p_file->get_size_e(p_abort);

				if (size < 0 || size > 65536) return io_result_error_data;

				p_file->read_object_e(song_buffer.set_size((unsigned int)size), (unsigned int)size, p_abort);

				if (! m_player->LoadSong(song_buffer.get_ptr(), (int)size ) ) return io_result_error_data;
			}
			catch(t_io_result code)
			{
				return code;
			}

			using_waves = true;
			{
				insync( g_waves_sync );
				if ( g_waves_refcount++ == 0 )
				{
					g_waves = new AHXWaves;
				}
			}
		}

		if (p_want_info)
		{
			AHXSong & Song = m_player->Song;

			if ( Song.Name[0] ) p_info.meta_set_ansi( "title", Song.Name );

			for ( int i = 1; i < Song.InstrumentNr + 1; i++ )
			{
				string8_fastalloc temp;
				const char * s = Song.Instruments[i].Name;
				if ( * s )
				{
					temp = "inst";
					if (i < 10) temp.add_byte('0');
					temp.add_int(i);
					p_info.meta_set_ansi( temp, s );
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

			m_player->Init( g_waves );
			m_player->InitSubsong( 0 );

#ifdef _DEBUG
			g_filename = p_location.get_path();
#endif

			AHXFooOut output;
			output.Play(m_player);
			p_info.set_length(output.GetLength());
		}

		if (p_decode)
		{
			pos = 0;
			swallow = 0;

			m_player->Init( g_waves );
			m_player->InitSubsong( 0 );

			if (can_loop) can_loop = p_can_loop;

			m_output = new AHXFooOut( nch, can_loop );
			m_output->Init( srate, 16, 1, 1.0f, 50 );
			m_output->SetOption( AHXOI_OVERSAMPLING, oversample );
			m_output->Play( m_player );
		}

		return io_result_success;
	}

public:
	virtual t_io_result set_info( const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort )
	{
		return io_result_error_data;
	}

	virtual t_io_result run( audio_chunk * chunk,abort_callback & p_abort )
	{
		int done;
		audio_sample * out = chunk->check_data_size( srate / 50 * nch );
		
		do
		{
			void * vout = out;
			if ( swallow >= (srate / 50) ) vout = 0;
			if ( ! m_output->CopyBuffer( vout, & done ) ) return io_result_error_generic;

			if ( ! done ) return io_result_eof;

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
						audio_sample * foo = chunk->get_data();
						audio_sample * bar = foo + swallow * nch;
						memcpy( foo, bar, done * nch );
					}

					done -= swallow;
					swallow = 0;
				}
			}
		}
		while ( ! done );

		chunk->set_srate( srate );
		chunk->set_channels( nch );
		chunk->set_sample_count( done );

		return io_result_success;
	}

	virtual t_io_result seek( double seconds,abort_callback & p_abort )
	{
		swallow = int( seconds * double(srate) + .5 );
		if (swallow > pos)
		{
			swallow -= pos;
			return io_result_success;
		}
		pos = 0;
		delete m_output;

		m_player->InitSubsong( 0 );

		m_output = new AHXFooOut( nch, can_loop );
		m_output->Init( srate, 16, 1, 1.0f, 50 );
		m_output->SetOption( AHXOI_OVERSAMPLING, oversample );
		m_output->Play( m_player );

		return io_result_success;
	}

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}
	inline static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {BC52C3D0-9DAD-4cfd-AE34-14AD5830E5F0}
		static const GUID guid = 
		{ 0xbc52c3d0, 0x9dad, 0x4cfd, { 0xae, 0x34, 0x14, 0xad, 0x58, 0x30, 0xe5, 0xf0 } };
		return guid;
	}

	static const char * g_get_name() {return "Abyss AHX decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};

DECLARE_FILE_TYPE("Abyss AHX files", "*.AHX");

static input_factory_t<input_ahx> g_input_ahx_factory;

DECLARE_COMPONENT_VERSION("Abyss AHX decoder", MY_VERSION, "Based on the AHX Replayer SDK v0.9, with miscellaneous fixes.\n\nAbyss homepage: http://abyss.moving-people.net\nAHX homepage: http://abyss.moving-people.net/ahxpc.html");
