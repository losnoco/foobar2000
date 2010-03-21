#define MYVERSION "1.0"

/*

	change log

2010-03-21 00:41 UTC - kode54
- Changed encoder parameters to use highest quality VBR

2010-03-11 17:04 UTC - kode54
- Initial release

*/

#include <foobar2000.h>

#include <aften.h>

class aften_dsp : public dsp_impl_base {
	AftenContext state;

    audio_sample buffered_samples[A52_SAMPLES_PER_FRAME * 6];
	pfc::array_t<t_uint8> output_buffer;

	service_ptr_t< dsp > m_resampler;

    unsigned nch, target_nch, channel_mask, srate, target_srate, buffered_sample_count;

    bool init()
    {
		aften_set_defaults( &state );
		if ( aften_wav_channels_to_acmod( nch, channel_mask, &state.acmod, &state.lfe ) )
			return false;

		state.verbose = 0;
		state.channels = nch;
    	state.samplerate = target_srate;
		state.params.encoding_mode = AFTEN_ENC_MODE_VBR;
		state.params.quality = 1023;
		state.params.bitrate = 640;
	    state.sample_format = A52_SAMPLE_FMT_FLT;

		if ( aften_encode_init( &state ) )
			return false;

		if ( !resampler_entry::g_create( m_resampler, srate, target_srate, 0 ) )
			return false;

		output_buffer.grow_size( A52_SAMPLES_PER_FRAME * target_nch * 2 );

		if ( output_buffer.get_size() < A52_MAX_CODED_FRAME_SIZE )
			return false;

        return true;
    }

    void cleanup()
    {
		aften_encode_close( &state );
		m_resampler.release();
		buffered_sample_count = 0;
		target_srate = 0;
		nch = 0;
		target_nch = 0;
		channel_mask = 0;
    }

public:
    aften_dsp()
    {
		memset( &state, 0, sizeof( state ) );
        cleanup();
    }

    ~aften_dsp()
    {
        cleanup();
    }

    static GUID g_get_guid()
    {
        static const GUID guid = { 0x80611a29, 0x6f41, 0x4dfa, { 0x85, 0xc0, 0x4b, 0x31, 0x5a, 0x19, 0x5c, 0x14 } };
        return guid;
    }

    static void g_get_name(pfc::string_base &p_out)
    {
        p_out = "Aften AC-3 encoder";
    }

    virtual void on_endoftrack(abort_callback &p_abort)
    {
        flush();
    }

    virtual void on_endofplayback(abort_callback &p_abort)
    {
        flush();
    }

    virtual bool on_chunk(audio_chunk *chunk, abort_callback &p_abort)
    {
        if (chunk->get_channels() <= 2) {
            return true;
        }

		unsigned new_target_srate;

		static const int sample_rates[] = { 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000 };
		for ( unsigned i = 0, j = tabsize( sample_rates ); i < j; i++ )
		{
			new_target_srate = sample_rates [i];
			if ( new_target_srate >= chunk->get_sample_rate() ) break;
		}

		if ( new_target_srate != target_srate || nch != chunk->get_channels() || channel_mask != chunk->get_channel_config() )
		{
			cleanup();
			srate = chunk->get_sample_rate();
			target_srate = new_target_srate;
			nch = chunk->get_channels();
			target_nch = nch >= 2 ? 2 : 1;
			channel_mask = chunk->get_channel_config();
			if (!init())
			{
				flush();
				return true;
			}
		}

		dsp_chunk_list_impl chunks; chunks.add_chunk( chunk );
		m_resampler->run_abortable( &chunks, NULL, 0, p_abort );

		unsigned sample_count = 0;
		for ( unsigned i = 0, j = chunks.get_count(); i < j; i++ )
		{
			sample_count += chunks.get_item( i )->get_sample_count();
		}

		unsigned current_chunk = 0;

		while ( sample_count )
		{
			while ( buffered_sample_count < A52_SAMPLES_PER_FRAME )
			{
				audio_chunk * chunk = chunks.get_item( current_chunk );
				unsigned samples_to_copy = chunk->get_sample_count();
				if ( samples_to_copy + buffered_sample_count > A52_SAMPLES_PER_FRAME )
					samples_to_copy = A52_SAMPLES_PER_FRAME - buffered_sample_count;
				memcpy( buffered_samples + buffered_sample_count * nch, chunk->get_data(), samples_to_copy * nch * sizeof(audio_sample) );
				buffered_sample_count += samples_to_copy;
				sample_count -= samples_to_copy;
				unsigned samples_remain = chunk->get_sample_count() - samples_to_copy;
				if ( samples_remain )
				{
					memcpy( chunk->get_data(), chunk->get_data() + samples_to_copy * nch, samples_remain * nch * sizeof(audio_sample) );
					chunk->set_sample_count( samples_remain );
				}
				else
				{
					if ( ++current_chunk >= chunks.get_count() ) break;
				}
			}
			if ( buffered_sample_count < A52_SAMPLES_PER_FRAME ) break;

			aften_remap_wav_to_a52( buffered_samples, A52_SAMPLES_PER_FRAME, nch, A52_SAMPLE_FMT_FLT, state.acmod );

			t_uint8 * ptr = output_buffer.get_ptr();

			int frame_size = aften_encode_frame( &state, ptr + 8, buffered_samples, A52_SAMPLES_PER_FRAME );
			if ( frame_size < 0 )
			{
				flush();
				return false;
			}

			buffered_sample_count = 0;

			ptr [0] = 0x72;
			ptr [1] = 0xF8;
			ptr [2] = 0x1F;
			ptr [3] = 0x4E;
			ptr [4] = 1;
			ptr [5] = 0;
			ptr [6] = ( frame_size << 3 ) & 0xFF;
			ptr [7] = ( frame_size >> 5 ) & 0xFF;
			for ( int i = 0; i < frame_size; i += 2 )
			{
				t_uint8 temp = ptr [8 + i];
				ptr [8 + i] = ptr [9 + i];
				ptr [9 + i] = temp;
			}
			memset( ptr + frame_size + 8, 0, output_buffer.get_size() - frame_size - 8 );

			audio_chunk * chunk = insert_chunk( A52_SAMPLES_PER_FRAME * target_nch );
			chunk->set_data_fixedpoint( ptr, output_buffer.get_size(), target_srate, target_nch, 16, audio_chunk::g_guess_channel_config( target_nch ) );
		}

		return false;
    }

    virtual void flush()
    {
		cleanup();
    }

    virtual double get_latency()
    {
		double latency = 0.;
		if ( buffered_sample_count && target_srate ) {
			latency += (double)buffered_sample_count / (double)target_srate;
		}
		if ( m_resampler.is_valid() ) latency += m_resampler->get_latency();
        return latency; // return (double)(buffer_data / 4) / 44100.0;
    }

    virtual bool need_track_change_mark()
    {
        return false;
    }
};

class version_aften : public componentversion
{
public:
	virtual void get_file_name(pfc::string_base & out) { out = core_api::get_my_file_name(); }
	virtual void get_component_name(pfc::string_base & out) { out = "Aften AC-3 Encoder"; }
	virtual void get_component_version(pfc::string_base & out) { out = MYVERSION; }
	virtual void get_about_message(pfc::string_base & out)
	{
    	out << "Aften: A/52 audio encoder\n"
                 "Version "
		<< aften_get_version()
		<< "\n"
			"(c) 2006-2009 Justin Ruggles, Prakash Punnoor, et al.";
	}
};

static dsp_factory_nopreset_t   <aften_dsp>     g_aften_dsp_factory;
static service_factory_single_t <version_aften> g_componentversion_aften_factory;

VALIDATE_COMPONENT_FILENAME("foo_dsp_aften.dll");
