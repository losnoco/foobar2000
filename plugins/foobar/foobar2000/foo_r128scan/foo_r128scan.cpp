/* See LICENSE file for copyright and license details. */

#include "stdafx.h"

#include "resource.h"

#define MY_VERSION "1.20"

/*
	change log

2011-01-31 01:33 UTC - kode54
- Fixed album scanning results reporting
- Version is now 1.20

2011-01-31 01:22 UTC - kode54
- Disabled apply tags button when there are no tags to apply results to
- Version is now 1.19

2011-01-30 23:53 UTC - kode54
- Implemented full error reporting
- Version is now 1.18

2011-01-30 06:23 UTC - kode54
- Fixed another crash issue with IO error handling in the scanner thread
- Version is now 1.17

2011-01-30 05:28 UTC - kode54
- Fixed a potential problem with threaded album scanning where not all of the threads would
  result in valid scanner instances
- Version is now 1.16

2011-01-29 02:44 UTC - kode54
- Updated to libebur128-0.1.9
- Changed scanner function to reconfigure libebur128 instead of resampling or
  failing outright
- Version is now 1.15

2011-01-28 05:48 UTC - kode54
- Implemented proper track and album skipping
- Version is now 1.14

2011-01-28 05:04 UTC - kode54
- Implemented multi-threaded album scanning
- Version is now 1.13

2011-01-27 07:04 UTC - kode54
- Scanner status now only lists the files currently being scanned
- Scanner status window now shows the count of processed files and the total count
- Limited thread count by the number of jobs to execute
- Version is now 1.12

2011-01-27 06:15 UTC - kode54
- Fixed channel mapping setup
- Added an abort check to the scanner processing loop just in case
- Changed the scanner results to a modeless dialog to get rid of the Windows 7
  taskbar progress indicator artifact
- Changed all applicable get_item() calls to [] operators
- Version is now 1.11

2011-01-26 10:34 UTC - kode54
- Changed "true" peak upsampling to 4x over-sampling instead of always 192KHz
- Version is now 1.10

2011-01-26 09:32 UTC - kode54
- Fixed mixed up infinity/NaN in ebur128.cpp
- Fixed rg_offset to return the correct invalid gain constant when the result is
  invalid or out of range
- Version is now 1.9

2011-01-26 08:47 UTC - kode54
- Implemented support for mid-file and mid-album sample rate changes
- Version is now 1.8

2011-01-26 08:15 UTC - kode54
- Fixed albums-by-tags scanning mode to include the last album detected in the batch
- Disabled multi-threading in debug builds
- Fixed "true" peak calculation to instantiate the resampler for more than just the
  first track in the album
- Restructured "true" peak calculation resampler code to properly handle the last
  chunk produced by the decoder
- Version is now 1.7

2011-01-26 06:03 UTC - kode54
- Implemented thread priority control
- Version is now 1.6

2011-01-26 05:53 UTC - kode54
- Implemented finer grained progress indicating
- Version is now 1.5

2011-01-26 05:16 UTC - kode54
- Fixed RG scanner result display dialog showing the track gain in the album peak column when
  scanning in track gain mode
- Version is now 1.4

2011-01-26 05:02 UTC - kode54
- Made "true" peak scanning optional
- Version is now 1.3

2011-01-26 04:24 UTC - kode54
- Fixed RG result info display when there are results from more than one job in the batch
- Fixed RG scanner to remove the current job's track names and update the display on I/O error
- Version is now 1.2

2011-01-26 03:45 UTC - kode54
- Fixed RG info applying filter result finding function to actually return the item indices
  so that album set tags are written correctly
- Version is now 1.1

2011-01-26 02:31 UTC - kode54
- Album scan mode now keeps track of the sample rate and channel information across tracks
- Version is now 1.0

2011-01-26 02:18 UTC - kode54
- Completed for initial release

2011-01-25 18:53 UTC - kode54
- Began work

*/

static inline double rg_offset(double lu)
{
	if ( lu == std::numeric_limits<double>::quiet_NaN() || lu == std::numeric_limits<double>::infinity() || lu < -70 ) return replaygain_info::gain_invalid;
	else return -18.0 - lu;
}

static const GUID guid_r128_branch = { 0x168d8789, 0xd829, 0x47a8, { 0xb4, 0x53, 0x7e, 0x84, 0x62, 0x61, 0xdb, 0x40 } };
static const GUID guid_cfg_album_pattern = { 0x75ebec52, 0xfdc0, 0x43f0, { 0xb1, 0x95, 0xf8, 0x3f, 0x2f, 0x7e, 0x1, 0xeb } };
static const GUID guid_cfg_true_peak_scanning = { 0x6d7153f9, 0x8ee6, 0x4e88, { 0x9a, 0xe8, 0xfa, 0xae, 0xf4, 0x99, 0x14, 0x15 } };
static const GUID guid_cfg_thread_priority = { 0x2b01ea31, 0x2524, 0x481d, { 0xaf, 0xa, 0x42, 0x52, 0x76, 0x6c, 0x53, 0xd1 } };

static advconfig_branch_factory r128_tools_branch("EBU R128 Gain Scanner", guid_r128_branch, advconfig_branch::guid_branch_tools, 0);
static advconfig_string_factory cfg_album_pattern("Album grouping pattern", guid_cfg_album_pattern, guid_r128_branch, 0, "%album artist% | %date% | %album%");
static advconfig_checkbox_factory cfg_true_peak_scanning("\"True\" peak scanning", guid_cfg_true_peak_scanning, guid_r128_branch, 0, false);
static advconfig_integer_factory cfg_thread_priority("Thread priority (1-7)", guid_cfg_thread_priority, guid_r128_branch, 0, 2, 1, 7 );

static const int thread_priority_levels[7] = { THREAD_PRIORITY_IDLE, THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };

struct last_chunk_info
{
	unsigned last_srate, last_channels, last_channel_config;

	last_chunk_info()
	{
		last_srate = 0;
		last_channels = 0;
		last_channel_config = 0;
	}

	const last_chunk_info & operator= ( const last_chunk_info & in )
	{
		last_srate = in.last_srate;
		last_channels = in.last_channels;
		last_channel_config = in.last_channel_config;
		return *this;
	}
};

bool change_parameters( ebur128_state * state, unsigned sample_rate, unsigned channels, unsigned channel_config )
{
	int rval = ebur128_change_parameters( state, channels, sample_rate );

	if ( rval != 0 && rval != 2 ) return false;

	for ( unsigned i = 0; i < channels; i++ )
	{
		int channel = EBUR128_UNUSED;
		switch ( audio_chunk::g_extract_channel_flag( channel_config, i ) )
		{
		case audio_chunk::channel_front_left:   channel = EBUR128_LEFT;           break;
		case audio_chunk::channel_front_right:  channel = EBUR128_RIGHT;          break;
		case audio_chunk::channel_front_center: channel = EBUR128_CENTER;         break;
		case audio_chunk::channel_back_left:    channel = EBUR128_LEFT_SURROUND;  break;
		case audio_chunk::channel_back_right:   channel = EBUR128_RIGHT_SURROUND; break;
		}
		ebur128_set_channel( state, i, channel );
	}

	return true;
}

double scan_track( ebur128_state * & state, audio_sample & peak, last_chunk_info & last_info, metadb_handle_ptr p_handle, service_ptr_t<dsp> & m_resampler, double & p_progress, unsigned track_total, threaded_process_status & p_status, critical_section & p_critsec, abort_callback & p_abort )
{
	input_helper               m_decoder;
	service_ptr_t<file>        m_file;
	audio_chunk_impl_temporary m_chunk, m_previous_chunk;
	file_info_impl             m_info;

	bool first_chunk = true;
	unsigned last_srate, last_channels, last_channel_config;

	double duration = 0, length = 0;

	m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );

	m_decoder.get_info( p_handle->get_subsong_index(), m_info, p_abort );
	length = m_info.get_length();

	while ( m_decoder.run( m_chunk, p_abort ) )
	{
		p_abort.check();

		if ( ! first_chunk && m_resampler.is_valid() )
		{
			dsp_chunk_list_impl chunks; chunks.add_chunk( &m_previous_chunk );
			m_resampler->run_abortable( &chunks, p_handle, 0, p_abort );

			for ( unsigned i = 0; i < chunks.get_count(); i++ )
			{
				const audio_chunk * chunk = chunks.get_item( i );
				audio_sample current_peak = audio_math::calculate_peak( chunk->get_data(), chunk->get_sample_count() * chunk->get_channels() );
				peak = max( peak, current_peak );
			}
		}

		if ( ! state )
		{
			last_srate = m_chunk.get_srate();
			last_channels = m_chunk.get_channels();
			last_channel_config = m_chunk.get_channel_config();

			if ( cfg_true_peak_scanning.get() )
			{
				resampler_entry::g_create( m_resampler, last_srate, last_srate * 4, 0 );
			}

			state = ebur128_init( last_channels, last_srate, EBUR128_MODE_I );
			if ( !state ) throw std::bad_alloc();

			if ( !change_parameters( state, last_srate, last_channels, last_channel_config ) )
				throw std::bad_alloc();

			last_info.last_srate = last_srate;
			last_info.last_channels = last_channels;
			last_info.last_channel_config = last_channel_config;
			first_chunk = false;
		}
		else if ( first_chunk )
		{
			last_srate = last_info.last_srate;
			last_channels = last_info.last_channels;
			last_channel_config = last_info.last_channel_config;

			if ( cfg_true_peak_scanning.get() && ( m_resampler.is_empty() || last_srate != m_chunk.get_srate() ) )
			{
				resampler_entry::g_create( m_resampler, last_srate, last_srate * 4, 0 );
			}

			first_chunk = false;
		}

		if ( last_srate != m_chunk.get_srate() || last_channels != m_chunk.get_channels() || last_channel_config != m_chunk.get_channel_config() )
		{
			last_info.last_srate          = last_srate          = m_chunk.get_srate();
			last_info.last_channels       = last_channels       = m_chunk.get_channels();
			last_info.last_channel_config = last_channel_config = m_chunk.get_channel_config();

			if ( !change_parameters( state, last_srate, last_channels, last_channel_config ) )
				throw std::bad_alloc();
		}

		if ( ebur128_add_frames_float( state, m_chunk.get_data(), m_chunk.get_sample_count() ) ) break;

		if ( m_resampler.is_valid() ) m_previous_chunk.copy( m_chunk );
		else
		{
			audio_sample current_peak = audio_math::calculate_peak( m_chunk.get_data(), m_chunk.get_sample_count() * m_chunk.get_channels() );
			peak = max( peak, current_peak );
		}

		duration += m_chunk.get_duration();

		if ( length )
		{
			insync(p_critsec);
			p_progress += m_chunk.get_duration() / length / double(track_total);
			p_status.set_progress_float( p_progress );
		}
	}

	if ( m_resampler.is_valid() )
	{
		dsp_chunk_list_impl chunks; chunks.add_chunk( &m_previous_chunk );
		m_resampler->run_abortable( &chunks, p_handle, dsp::END_OF_TRACK, p_abort );

		for ( unsigned i = 0; i < chunks.get_count(); i++ )
		{
			const audio_chunk * chunk = chunks.get_item( i );
			audio_sample current_peak = audio_math::calculate_peak( chunk->get_data(), chunk->get_sample_count() * chunk->get_channels() );
			peak = max( peak, current_peak );
		}
	}

	if ( first_chunk ) throw exception_io_data("File ended without producing any output");

	{
		insync(p_critsec);
		p_progress += ( length ? ( ( length - duration ) / length ) : 1.0 ) / double(track_total);
		p_status.set_progress_float( p_progress );
	}

	return duration;
}

struct r128_scanner_result
{
	enum status
	{
		error = 0,
		track,
		album
	};

	status                     m_status;
	pfc::string8               m_error_message;
	metadb_handle_list         m_handles;
	double                     m_album_gain;
	audio_sample               m_album_peak;
	pfc::array_t<double>       m_track_gain;
	pfc::array_t<audio_sample> m_track_peak;

	r128_scanner_result(status p_status)
		: m_status(p_status), m_album_gain(0), m_album_peak(0) { }

	r128_scanner_result(status p_status, const metadb_handle_list & p_handles)
		: m_status(p_status), m_handles(p_handles), m_album_gain(0), m_album_peak(0) { }

	r128_scanner_result(const r128_scanner_result * in)
	{
		m_status        = in->m_status;
		m_error_message = in->m_error_message;
		m_handles       = in->m_handles;
		m_album_gain    = in->m_album_gain;
		m_album_peak    = in->m_album_peak;
		m_track_gain    = in->m_track_gain;
		m_track_peak    = in->m_track_peak;
	}
};

static void RunR128ResultsPopup( pfc::ptr_list_t<r128_scanner_result> & m_results, double sample_duration, unsigned __int64 processing_duration, HWND p_parent );

class r128_scanner : public threaded_process_callback
{
	critical_section lock_status;
	threaded_process_status * status_callback;
	double m_progress;

	unsigned thread_count;

	abort_callback * m_abort;

	pfc::array_t<HANDLE> m_extra_threads;

	struct job
	{
		bool                           m_is_album;
		metadb_handle_list             m_handles;
		pfc::ptr_list_t<const char>    m_names;

		// Album mode stuff
		critical_section               m_lock;
		pfc::array_t<bool>             m_thread_in_use;
		size_t                         m_tracks_left;
		size_t                         m_current_thread;
		pfc::array_t<last_chunk_info>  m_last_info;
		pfc::ptr_list_t<ebur128_state> m_states;
		service_list_t<dsp>            m_resamplers;
		metadb_handle_list             m_handles_done;
		r128_scanner_result          * m_scanner_result;

		job( bool p_is_album, const metadb_handle_list & p_handles )
		{
			m_is_album = p_is_album;
			m_handles = p_handles;
			m_tracks_left = p_handles.get_count();
			m_current_thread = 0;
			m_scanner_result = NULL;
		}

		~job()
		{
			m_names.delete_all();
			for ( unsigned i = 0; i < m_states.get_count(); i++ )
			{
				ebur128_state * state = m_states[ i ];
				ebur128_destroy( &state );
			}
			delete m_scanner_result;
		}
	};

	LONG input_items_total;
	volatile LONG input_items_remaining;

	critical_section lock_input_job_list;
	pfc::ptr_list_t<job> input_job_list;

	critical_section lock_input_name_list;
	pfc::ptr_list_t<const char> input_name_list;

	critical_section lock_output_list;
	pfc::ptr_list_t<r128_scanner_result> output_list;
	double output_duration;

	FILETIME start_time, end_time;

	void report_error( const char * message, metadb_handle_ptr track )
	{
		r128_scanner_result * result = NULL;
		try
		{
			result = new r128_scanner_result( r128_scanner_result::error );
			result->m_error_message = message;
			result->m_handles.add_item( track );
			{
				insync( lock_output_list );
				output_list.add_item( result );
			}
		}
		catch (...)
		{
			delete result;
		}
	}

	void report_error( const char * message, metadb_handle_list tracks )
	{
		r128_scanner_result * result = NULL;
		try
		{
			result = new r128_scanner_result( r128_scanner_result::error );
			result->m_error_message = message;
			result->m_handles = tracks;
			{
				insync( lock_output_list );
				output_list.add_item( result );
			}
		}
		catch (...)
		{
			delete result;
		}
	}

	void scanner_process()
	{
		for (;;)
		{
			job * m_current_job = NULL;

			m_abort->check();

			{
				insync( lock_input_job_list );

				if ( ! input_job_list.get_count() ) break;

				m_current_job = input_job_list[ 0 ];

				if ( m_current_job->m_is_album )
				{
					insync( m_current_job->m_lock );
					if ( m_current_job->m_handles.get_count() <= 1 )
						input_job_list.remove_by_idx( 0 );
					if ( m_current_job->m_handles.get_count() == 0 )
						continue;
				}
				else input_job_list.remove_by_idx( 0 );
			}

			ebur128_state * m_state = NULL;

			try
			{
				if ( !m_current_job->m_is_album )
				{
					{
						insync(lock_input_name_list);
						input_name_list.add_item( m_current_job->m_names[ 0 ] );
					}
					update_status();

					audio_sample m_current_peak = 0;
					last_chunk_info last_info;
					service_ptr_t<dsp> m_resampler;
					double duration = scan_track( m_state, m_current_peak, last_info, m_current_job->m_handles[ 0 ], m_resampler, m_progress, input_items_total, *status_callback, lock_status, *m_abort );
					r128_scanner_result * m_current_result = new r128_scanner_result(r128_scanner_result::track, m_current_job->m_handles);
					m_current_result->m_album_gain = ebur128_loudness_global( m_state );
					m_current_result->m_album_peak = m_current_peak;
					{
						insync(lock_output_list);
						output_list.add_item( m_current_result );
						output_duration += duration;
					}

					{
						insync(lock_input_name_list);
						input_name_list.remove_item( m_current_job->m_names[ 0 ] );
					}
					InterlockedDecrement( &input_items_remaining );
					update_status();
				}
				else
				{
					size_t m_current_thread;
					metadb_handle_ptr m_current_track;
					const char * m_current_name = NULL;
					r128_scanner_result * m_current_result = NULL;
					last_chunk_info m_last_info;
					service_ptr_t<dsp> m_resampler;
					{
						insync( m_current_job->m_lock );
						m_current_result = m_current_job->m_scanner_result;
						if ( !m_current_result )
						{
							m_current_result = new r128_scanner_result(r128_scanner_result::album);
							m_current_job->m_scanner_result = m_current_result;
							m_current_job->m_thread_in_use.append_multi( false, thread_count );
							m_current_job->m_last_info.set_count( thread_count );
							m_current_job->m_states.add_items_repeat( NULL, thread_count );
							m_current_job->m_resamplers.set_count( thread_count );
						}
						do
						{
							m_abort->check();
							m_current_thread = m_current_job->m_current_thread;
							m_current_job->m_current_thread = ( m_current_thread + 1 ) % thread_count;
							Sleep(10);
						}
						while ( m_current_job->m_thread_in_use[ m_current_thread ] );
						m_current_job->m_thread_in_use[ m_current_thread ] = true;
						m_state = m_current_job->m_states[ m_current_thread ];
						m_current_track = m_current_job->m_handles[ 0 ];
						m_current_job->m_handles.remove_by_idx( 0 );
						m_last_info = m_current_job->m_last_info[ m_current_thread ];
						m_resampler = m_current_job->m_resamplers[ m_current_thread ];
						{
							insync(lock_input_name_list);
							input_name_list.add_item( m_current_name = m_current_job->m_names[ 0 ] );
							m_current_job->m_names.remove_by_idx( 0 );
						}
					}
					try
					{
						update_status();

						audio_sample m_current_track_peak = 0;
						double duration = scan_track( m_state, m_current_track_peak, m_last_info, m_current_track, m_resampler, m_progress, input_items_total, *status_callback, lock_status, *m_abort );
						double m_current_track_gain = ebur128_loudness_segment( m_state );

						ebur128_start_new_segment( m_state );

						{
							insync(lock_input_name_list);
							input_name_list.delete_item( m_current_name );
							m_current_name = NULL;
						}
						{
							insync(lock_output_list);
							output_duration += duration;
						}
						InterlockedDecrement( &input_items_remaining );
						update_status();

						try
						{
							m_current_job->m_lock.enter();

							m_current_result->m_track_gain.append_single( m_current_track_gain );
							m_current_result->m_track_peak.append_single( m_current_track_peak );
							m_current_result->m_album_peak = max(m_current_result->m_album_peak, m_current_track_peak);
							m_current_job->m_handles_done.add_item( m_current_track );

							m_current_job->m_states.replace_item( m_current_thread, m_state );
							m_current_job->m_last_info[ m_current_thread ] = m_last_info;
							m_current_job->m_resamplers[ m_current_thread ] = m_resampler;

							m_state = NULL;

							if ( --m_current_job->m_tracks_left == 0 )
							{
								unsigned null_state = m_current_job->m_states.find_item( ( ebur128_state * ) NULL );
								while ( null_state != ~0 )
								{
									m_current_job->m_states.remove_by_idx( null_state );
									null_state = m_current_job->m_states.find_item( ( ebur128_state * ) NULL );
								}
								if ( !m_current_job->m_states.get_count() )
									throw exception_io_data( "No tracks were successfully scanned" );

								m_current_result->m_album_gain = ebur128_loudness_global_multiple( m_current_job->m_states.get_ptr(), m_current_job->m_states.get_count() );

								m_current_result->m_handles.add_items( m_current_job->m_handles_done );
								{
									insync(lock_output_list);
									output_list.add_item( m_current_result );
									m_current_job->m_scanner_result = m_current_result = NULL;
								}
								{
									insync( lock_input_job_list );
									input_job_list.remove_item( m_current_job );
									m_current_job->m_lock.leave();
									delete m_current_job;
								}
							}
							else
							{
								m_current_job->m_thread_in_use[ m_current_thread ] = false;
								m_current_job->m_lock.leave();
							}

							m_current_job = NULL;
						}
						catch (std::exception & e)
						{
							m_current_job->m_lock.leave();
							report_error( e.what(), m_current_track );
							throw;
						}
						catch (...)
						{
							m_current_job->m_lock.leave();
							throw;
						}
					}
					catch (...)
					{
						{
							insync(lock_input_name_list);
							input_name_list.delete_item( m_current_name );
						}
						{
							insync( m_current_job->m_lock );
							if ( m_state )
								m_current_job->m_states.replace_item( m_current_thread, m_state );
							m_current_job->m_thread_in_use[ m_current_thread ] = false;
							if ( m_current_job->m_tracks_left == 0 || --m_current_job->m_tracks_left == 0 )
							{
								{
									insync(lock_input_name_list);
									for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
									{
										input_name_list.remove_item( m_current_job->m_names[ i ] );
										InterlockedDecrement( &input_items_remaining );
									}
								}

								{
									insync(lock_status);
									m_progress += double( m_current_job->m_names.get_count() ) / double( input_items_total );
								}

								delete m_current_job;
							}
							m_current_job = NULL;
						}
						m_state = NULL;
						throw;
					}
				}
			}
			catch (exception_io & e)
			{
				if ( m_current_job )
				{
					report_error( e.what(), m_current_job->m_handles );

					{
						insync(lock_input_name_list);
						for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
						{
							input_name_list.remove_item( m_current_job->m_names[ i ] );
							InterlockedDecrement( &input_items_remaining );
						}
					}

					{
						insync(lock_status);
						m_progress += double( m_current_job->m_names.get_count() ) / double( input_items_total );
					}

					update_status();
				}
			}
			catch (exception_aborted & e)
			{
				if (m_state) ebur128_destroy(&m_state);
				if ( m_current_job )
				{
					report_error( e.what(), m_current_job->m_handles );

					{
						insync(lock_input_name_list);
						for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
						{
							input_name_list.remove_item( m_current_job->m_names[ i ] );
						}
					}
				}
				delete m_current_job;
				break;
			}
			catch (std::exception & e)
			{
				if (m_state) ebur128_destroy(&m_state);
				if ( m_current_job )
				{
					report_error( e.what(), m_current_job->m_handles );
					{
						insync(lock_input_name_list);
						for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
						{
							input_name_list.remove_item( m_current_job->m_names[ i ] );
						}
					}
				}
				delete m_current_job;
				throw;
			}
			catch (...)
			{
				if (m_state) ebur128_destroy(&m_state);
				if ( m_current_job )
				{
					insync(lock_input_name_list);
					for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
					{
						input_name_list.remove_item( m_current_job->m_names[ i ] );
					}
				}
				delete m_current_job;
				throw;
			}

			if (m_state) ebur128_destroy(&m_state);
			if ( m_current_job )
			{
				insync(lock_input_name_list);
				for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
				{
					input_name_list.remove_item( m_current_job->m_names[ i ] );
				}
			}
			delete m_current_job; m_current_job = NULL;
		}
	}

	void update_status()
	{
		pfc::string8 title, paths;

		{
			insync( lock_input_name_list );

			for ( unsigned i = 0; i < input_name_list.get_count(); i++ )
			{
				const char * temp = input_name_list[ i ];
				if ( paths.length() ) paths += "; ";
				paths.add_string( temp );
			}
		}

		title = "EBU R128 Gain Scan Progress";
		title += " (";
		title += pfc::format_int( input_items_total - input_items_remaining );
		title += "/";
		title += pfc::format_int( input_items_total );
		title += ")";

		{
			insync( lock_status );
			status_callback->set_title( title );
			status_callback->set_item( paths );
			status_callback->set_progress_float( m_progress );
		}
	}

	static DWORD CALLBACK g_entry(void* p_instance)
	{
		try
		{
			reinterpret_cast<r128_scanner*>(p_instance)->scanner_process();
		}
		catch (...) { }
		return 0;
	}

	void threads_start( unsigned count )
	{
		int priority = GetThreadPriority( GetCurrentThread() );

		for ( unsigned i = 0; i < count; i++ )
		{
			HANDLE thread = CreateThread( NULL, 0, g_entry, reinterpret_cast<void*>(this), CREATE_SUSPENDED, NULL );
			if ( thread != NULL )
			{
				SetThreadPriority( thread, priority );
				m_extra_threads.append_single( thread );
			}
		}

		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			ResumeThread( m_extra_threads[ i ] );
		}
	}

	void threads_stop()
	{
		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			HANDLE thread = m_extra_threads[ i ];
			WaitForSingleObject( thread, INFINITE );
			CloseHandle( thread );
		}

		m_extra_threads.set_count( 0 );
	}

public:
	r128_scanner()
	{
		input_items_remaining = input_items_total = 0;
		output_duration = 0;
		m_progress = 0;
	}

	void add_job_track( const metadb_handle_ptr & p_input )
	{
		metadb_handle_list p_list; p_list.add_item( p_input );

		job * m_job = new job( false, p_list );

		const char * m_path = p_input->get_path();
		t_size filename = pfc::scan_filename( m_path );
		t_size length = strlen( m_path + filename );
		char * m_name = new char[length + 1];
		strcpy_s(m_name, length + 1, m_path + filename);

		m_job->m_names.add_item( m_name );

		input_job_list.add_item( m_job );

		input_items_remaining = input_items_total += 1;
	}

	void add_job_album( const metadb_handle_list & p_input )
	{
		job * m_job = new job( true, p_input );

		for ( unsigned i = 0; i < p_input.get_count(); i++ )
		{
			const char * m_path = p_input[ i ]->get_path();
			t_size filename = pfc::scan_filename( m_path );
			t_size length = strlen( m_path + filename );
			char * m_name = new char[length + 1];
			strcpy_s(m_name, length + 1, m_path + filename);

			m_job->m_names.add_item( m_name );
		}

		input_job_list.add_item( m_job );

		input_items_remaining = input_items_total += p_input.get_count();
	}

	~r128_scanner()
	{
		input_job_list.delete_all();
		output_list.delete_all();
	}

	virtual void run(threaded_process_status & p_status,abort_callback & p_abort)
	{
		status_callback = &p_status;
		m_abort = &p_abort;

		update_status();

		SetThreadPriority( GetCurrentThread(), thread_priority_levels[ cfg_thread_priority.get() - 1 ] );

		GetSystemTimeAsFileTime( &start_time );

#if defined(NDEBUG) || 1
		thread_count = 0;

		for ( unsigned i = 0; i < input_job_list.get_count(); i++ ) thread_count += input_job_list[ i ]->m_handles.get_count();

		thread_count = pfc::getOptimalWorkerThreadCountEx( min( thread_count, 4 ) );

		if ( thread_count > 1 ) threads_start( thread_count - 1 );
#else
		thread_count = 1;
#endif

		try
		{
			scanner_process();
		}
		catch (...) { }

		threads_stop();

		GetSystemTimeAsFileTime( &end_time );
	}

	virtual void on_done( HWND p_wnd, bool p_was_aborted )
	{
		threads_stop();

		if ( !p_was_aborted )
		{
			DWORD high = end_time.dwHighDateTime - start_time.dwHighDateTime;
			DWORD low = end_time.dwLowDateTime - start_time.dwLowDateTime;

			if ( end_time.dwLowDateTime < start_time.dwLowDateTime ) high--;

			unsigned __int64 timestamp = ((unsigned __int64)(high) << 32) + low;

			RunR128ResultsPopup( output_list, output_duration, timestamp, core_api::get_main_window() );
		}
	}
};

class rg_remove_filter : public file_info_filter
{
	metadb_handle_list m_handles;

public:
	rg_remove_filter( const pfc::list_base_const_t<metadb_handle_ptr> & p_list )
	{
		pfc::array_t<t_size> order;
		order.set_size(p_list.get_count());
		order_helper::g_fill(order.get_ptr(),order.get_size());
		p_list.sort_get_permutation_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,order.get_ptr());
		m_handles.set_count(order.get_size());
		for(t_size n = 0; n < order.get_size(); n++) {
			m_handles[n] = p_list[order[n]];
		}
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		t_size index;
		if (m_handles.bsearch_t(pfc::compare_t<metadb_handle_ptr,metadb_handle_ptr>,p_location,index))
		{
			replaygain_info info = p_info.get_replaygain();
			p_info.reset_replaygain();
			return info != p_info.get_replaygain();
		}
		else
		{
			return false;
		}
	}
};

class rg_apply_filter : public file_info_filter
{
	pfc::ptr_list_t<r128_scanner_result> m_results;

	bool find_offsets( metadb_handle_ptr p_location, unsigned & result_offset, unsigned & item_offset )
	{
		for ( unsigned i = 0; i < m_results.get_count(); i++ )
		{
			r128_scanner_result * result = m_results[ i ];
			for ( unsigned j = 0; j < result->m_handles.get_count(); j++ )
			{
				if ( result->m_handles[ j ] == p_location )
				{
					result_offset = i;
					item_offset = j;
					return true;
				}
			}
		}
		result_offset = 0;
		item_offset = 0;
		return false;
	}

public:
	rg_apply_filter( const pfc::ptr_list_t<r128_scanner_result> & p_list )
	{
		for(t_size n = 0; n < p_list.get_count(); n++) {
			r128_scanner_result * in_result = p_list[n];
			if ( in_result->m_status != r128_scanner_result::error )
			{
				r128_scanner_result * result = new r128_scanner_result( in_result );
				m_results.add_item(result);
			}
		}
	}

	~rg_apply_filter()
	{
		m_results.delete_all();
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		unsigned result_offset, item_offset;
		if (find_offsets(p_location, result_offset, item_offset))
		{
			r128_scanner_result * result = m_results[ result_offset ];

			replaygain_info info = p_info.get_replaygain();
			replaygain_info orig = info;

			if ( result->m_status == r128_scanner_result::album )
			{
				info.m_album_gain = float(rg_offset(result->m_album_gain));
				info.m_album_peak = result->m_album_peak;
				info.m_track_gain = float(rg_offset(result->m_track_gain[ item_offset ]));
				info.m_track_peak = result->m_track_peak[ item_offset ];
			}
			else
			{
				info.remove_album_gain();
				info.remove_album_peak();
				info.m_track_gain = float(rg_offset(result->m_album_gain));
				info.m_track_peak = result->m_album_peak;
			}

			p_info.set_replaygain( info );
			
			return info != orig;
		}
		else
		{
			return false;
		}
	}
};

class CMyResultsPopup : public CDialogImpl<CMyResultsPopup>, public CDialogResize<CMyResultsPopup>
{
public:
	CMyResultsPopup( const pfc::ptr_list_t<r128_scanner_result> & initData, double sample_duration, unsigned __int64 processing_duration )
		: m_sample_duration( sample_duration ), m_processing_duration( processing_duration )
	{
		for ( unsigned i = 0; i < initData.get_count(); i++ )
		{
			r128_scanner_result * result = new r128_scanner_result( initData[ i ] );
			m_initData.add_item( result );
		}		
	}

	~CMyResultsPopup()
	{
		m_initData.delete_all();
	}

	enum { IDD = IDD_RESULTS };

	BEGIN_MSG_MAP( CMyResultsPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnCancel )
		COMMAND_HANDLER_EX( IDOK, BN_CLICKED, OnAccept )
		MSG_WM_NOTIFY( OnNotify )
		CHAIN_MSG_MAP(CDialogResize<CMyResultsPopup>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP( CMyResultsPopup )
		DLGRESIZE_CONTROL( IDC_LISTVIEW, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_STATUS, DLSZ_SIZE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		DlgResize_Init();

		double processing_duration = (double)(m_processing_duration) * 0.0000001;
		double processing_ratio = m_sample_duration / processing_duration;

		pfc::string8_fast temp;

		temp = "Calculated in: ";
		temp += pfc::format_time_ex( processing_duration );
		temp += ", speed: ";
		temp += pfc::format_float( processing_ratio, 0, 2 );
		temp += "x";

		uSetDlgItemText( m_hWnd, IDC_STATUS, temp );

		m_listview = GetDlgItem( IDC_LISTVIEW );

		LVCOLUMN lvc = { 0 };
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = _T("Name");
		lvc.cx = 225;
		lvc.iSubItem = 0;
		m_listview.InsertColumn( 0, &lvc );
		lvc.pszText = _T("Status");
		lvc.cx = 150;
		lvc.iSubItem = 1;
		m_listview.InsertColumn( 1, &lvc );
		lvc.fmt = LVCFMT_RIGHT;
		lvc.pszText = _T("Album gain");
		lvc.cx = 75;
		m_listview.InsertColumn( 2, &lvc );
		lvc.pszText = _T("Track gain");
		m_listview.InsertColumn( 3, &lvc );
		lvc.pszText = _T("Album peak");
		m_listview.InsertColumn( 4, &lvc );
		lvc.pszText = _T("Track peak");
		m_listview.InsertColumn( 5, &lvc );

		for ( unsigned i = 0, index = 0; i < m_initData.get_count(); i++ )
		{
			for ( unsigned j = 0; j < m_initData[ i ]->m_handles.get_count(); j++, index++ )
			{
				m_listview.InsertItem( index, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 1, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 2, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 3, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 4, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 5, LPSTR_TEXTCALLBACK );
			}
		}

		if ( !static_api_ptr_t<titleformat_compiler>()->compile( m_script, "%title%" ) )
			m_script.release();

		ShowWindow(SW_SHOW);

		unsigned error_tracks = 0;
		unsigned total_tracks = 0;

		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			unsigned count = m_initData[ i ]->m_handles.get_count();
			if ( m_initData[ i ]->m_status == r128_scanner_result::error )
			{
				error_tracks += count;
			}
			total_tracks += count;
		}

		if ( error_tracks )
		{
			if ( error_tracks == total_tracks ) GetDlgItem( IDOK ).EnableWindow( FALSE );

			popup_message::g_show( pfc::string_formatter() << pfc::format_int( error_tracks ) << " out of " << pfc::format_int( total_tracks ) << " items could not be processed.", "EBU R128 Gain Scan - warning", popup_message::icon_error );
		}

		return TRUE;
	}

	void get_offsets( unsigned item, unsigned & job_number, unsigned & item_number )
	{
		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			r128_scanner_result * result = m_initData[ i ];
			if ( item >= result->m_handles.get_count() ) item -= result->m_handles.get_count();
			else
			{
				job_number = i;
				item_number = item;
				return;
			}
		}
		job_number = 0;
		item_number = 0;
	}

	LRESULT OnNotify( int, LPNMHDR message )
	{
		if ( message->hwndFrom == m_listview.m_hWnd )
		{
			switch ( message->code )
			{
			case LVN_GETDISPINFO:
				{
					LV_DISPINFO *pLvdi = (LV_DISPINFO *)message;

					unsigned job_number = 0, item_number = 0;

					get_offsets( pLvdi->item.iItem, job_number, item_number );
					r128_scanner_result * result = m_initData[ job_number ];

					switch (pLvdi->item.iSubItem)
					{
					case 0:
						if ( m_script.is_valid() ) result->m_handles[ item_number ]->format_title( NULL, m_temp, m_script, NULL );
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 1:
						if ( result->m_status != r128_scanner_result::error ) m_temp = "Success";
						else m_temp = result->m_error_message;
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 2:
						if ( result->m_status != r128_scanner_result::error )
						{
							if ( result->m_status == r128_scanner_result::album )
							{
								char m_gain[32];
								if ( replaygain_info::g_format_gain( float(rg_offset(result->m_album_gain)), m_gain ) ) m_temp = m_gain;
								else m_temp.reset();
							}
							else m_temp.reset();
						}
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 3:
						if ( result->m_status != r128_scanner_result::error )
						{
							double m_track_gain = (result->m_status == r128_scanner_result::album ? result->m_track_gain[ item_number ] : result->m_album_gain);
							char m_gain[32];
							if ( replaygain_info::g_format_gain( float(rg_offset(m_track_gain)), m_gain ) ) m_temp = m_gain;
							else m_temp.reset();
						}
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 4:
						if ( result->m_status != r128_scanner_result::error )
						{
							if ( result->m_status == r128_scanner_result::album )
							{
								char m_peak[32];
								if ( replaygain_info::g_format_peak( result->m_album_peak, m_peak ) ) m_temp = m_peak;
								else m_temp.reset();
							}
							else m_temp.reset();
						}
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 5:
						if ( result->m_status != r128_scanner_result::error )
						{
							audio_sample m_track_peak = (result->m_status == r128_scanner_result::album ? result->m_track_peak[ item_number ] : result->m_album_peak);
							char m_peak[32];
							if ( replaygain_info::g_format_peak( m_track_peak, m_peak ) ) m_temp = m_peak;
							else m_temp.reset();
						}
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;
					}
				}
				break;
			}
		}

		return 0;
	}

	void OnAccept( UINT, int id, CWindow )
	{
		metadb_handle_list list;

		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			r128_scanner_result * result = m_initData[ i ];
			if ( result->m_status != r128_scanner_result::error )
			{
				list.add_items( result->m_handles );
			}
		}

		static_api_ptr_t<metadb_io_v2>()->update_info_async( list, new service_impl_t< rg_apply_filter >( m_initData ), core_api::get_main_window(), 0, 0 );

		DestroyWindow();
	}

	void OnCancel( UINT, int id, CWindow )
	{
		DestroyWindow();
	}

	double m_sample_duration;
	unsigned __int64 m_processing_duration;

	pfc::ptr_list_t<r128_scanner_result> m_initData;

	CListViewCtrl m_listview;
	service_ptr_t<titleformat_object> m_script;
	pfc::string8_fast m_temp;
	pfc::stringcvt::string_os_from_utf8_fast m_convert;
};

static void RunR128ResultsPopup( pfc::ptr_list_t<r128_scanner_result> & p_data, double sample_duration, unsigned __int64 processing_duration, HWND p_parent )
{
	CMyResultsPopup * popup = new CWindowAutoLifetime<ImplementModelessTracking<CMyResultsPopup>>( p_parent, p_data, sample_duration, processing_duration );
}

static const GUID guid_context_group_r128 = { 0x64b18d93, 0x7187, 0x48be, { 0x97, 0x7f, 0xa8, 0xb6, 0x2d, 0xe7, 0x85, 0xdf } };

class context_r128 : public contextmenu_item_simple
{
public:
	virtual unsigned get_num_items() { return 4; }

	virtual void get_item_name(unsigned n, pfc::string_base & out)
	{
		if (n > 3) uBugCheck();
		static const char * names[4] = { "Scan per-file track gain", "Scan selection as a single album", "Scan selection as albums (by tags)", "Remove ReplayGain information from files" };
		out = names[n];
	}

	GUID get_parent() {return guid_context_group_r128;}

	virtual bool get_item_description(unsigned n, pfc::string_base & out)
	{
		if (n > 3) uBugCheck();
		else if (n < 3)
		{
			static const char * descriptions[3] = { "independent tracks.", "one album.", "one or more albums, dividing albums according to tags." };
			out = "EBU R128 Gain scans the selected items as ";
			out += descriptions[n];
		}
		else out = "Removes ReplayGain information from the selected items.";
			
		return true;
	}

	virtual GUID get_item_guid(unsigned p_index)
	{
		if (p_index > 3) uBugCheck();
		static const GUID guids[4] = {
			{ 0xc3373b6a, 0x25bd, 0x4b81, { 0xb4, 0x2c, 0x6a, 0x81, 0xd3, 0x5b, 0xfe, 0x5f } },
			{ 0xcc9b097d, 0xa74e, 0x4526, { 0xb8, 0x69, 0xf, 0x4c, 0x30, 0x87, 0x1, 0x1c } },
			{ 0xc8836b76, 0xaa6a, 0x4d36, { 0xa4, 0x7f, 0x67, 0xff, 0xe8, 0x29, 0xf4, 0x64 } },
			{ 0xc81f2c8d, 0x848e, 0x47ca, { 0x95, 0xd7, 0x17, 0xa2, 0x18, 0xfb, 0x22, 0xa3 } }
		};
		return guids[p_index];
	}

	virtual bool context_get_display(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,pfc::string_base & out,unsigned & displayflags,const GUID &)
	{
		if (n > 3) uBugCheck();
		get_item_name( n, out );
		displayflags = 0;
		return true;
	}

	virtual void context_command(unsigned n,const pfc::list_base_const_t<metadb_handle_ptr> & data,const GUID& caller)
	{
		metadb_handle_list input_files = data;
		input_files.remove_duplicates();

		service_ptr_t<r128_scanner> p_callback = new service_impl_t< r128_scanner >;

		if ( n == 3 )
		{
			static_api_ptr_t<metadb_io_v2>()->update_info_async( input_files, new service_impl_t< rg_remove_filter >( data ), core_api::get_main_window(), 0, 0 );
			return;
		}

		if ( n == 0 )
		{
			unsigned tags_present_count = 0;

			bit_array_bittable rg_tracks( input_files.get_count() );

			for ( unsigned i = 0; i < input_files.get_count(); i++ )
			{
				file_info_impl info;
				replaygain_info rg_info;

				if ( input_files[ i ]->get_info_async( info ) )
				{
					rg_info = info.get_replaygain();

					if ( rg_info.is_album_gain_present() || rg_info.is_album_peak_present() ||
						rg_info.is_track_gain_present() || rg_info.is_track_peak_present() )
					{
						tags_present_count++;
						rg_tracks.set( i, true );
					}
				}
			}

			if ( tags_present_count )
			{
				if ( tags_present_count == input_files.get_count() )
				{
					if ( uMessageBox( core_api::get_main_window(), "All tracks you have selected already have ReplayGain info. Would you like to scan them anyway?", "Information", MB_YESNO ) == IDNO )
					{
						return;
					}
				}
				else
				{
					input_files.remove_mask( rg_tracks );
				}
			}

			for ( unsigned i = 0; i < input_files.get_count(); i++ )
			{
				p_callback->add_job_track( input_files[ i ] );
			}
		}
		else if ( n == 1 )
		{
			unsigned tags_present_count = 0;

			for ( unsigned i = 0; i < input_files.get_count(); i++ )
			{
				file_info_impl info;
				replaygain_info rg_info;

				if ( input_files[ i ]->get_info_async( info ) )
				{
					rg_info = info.get_replaygain();

					if ( rg_info.is_album_gain_present() && rg_info.is_album_peak_present() &&
						rg_info.is_track_peak_present() )
					{
						tags_present_count++;
					}
				}
			}

			if ( tags_present_count == input_files.get_count() )
			{
				if ( uMessageBox( core_api::get_main_window(), "All tracks you have selected already have ReplayGain info. Would you like to scan them anyway?", "Information", MB_YESNO ) == IDNO )
				{
					return;
				}
			}

			p_callback->add_job_album( input_files );
		}
		else if ( n == 2 )
		{
			service_ptr_t<titleformat_object> m_script;

			pfc::string8 sort_format;

			cfg_album_pattern.get( sort_format );
			sort_format += "|%path_sort%";

			if ( !static_api_ptr_t<titleformat_compiler>()->compile( m_script, sort_format ) )
				m_script.release();

			input_files.sort_by_format( m_script, NULL );

			sort_format.truncate( sort_format.length() - strlen( "|%path_sort%" ) );

			if ( !static_api_ptr_t<titleformat_compiler>()->compile( m_script, sort_format ) )
				m_script.release();

			pfc::string8_fast current_album, temp_album;

			pfc::list_t<metadb_handle_list> nested_list;
			metadb_handle_list list;

			for ( unsigned i = 0, j = input_files.get_count(); i < j; i++ )
			{
				metadb_handle_ptr ptr = input_files[ i ];
				if (!ptr->format_title(NULL, temp_album, m_script, NULL)) temp_album.reset();
				if (stricmp_utf8(current_album, temp_album))
				{
					if ( list.get_count() ) nested_list.add_item( list );
					list.remove_all();
					current_album = temp_album;
				}
				list.add_item( ptr );
			}
			if ( list.get_count() ) nested_list.add_item( list );

			unsigned albums_have_all_tags = 0;

			bit_array_bittable rg_albums( nested_list.get_count() );

			for ( unsigned i = 0; i < nested_list.get_count(); i++ )
			{
				unsigned tags_present_count = 0;

				metadb_handle_list & list = nested_list[ i ];

				for ( unsigned j = 0; j < list.get_count(); j++ )
				{
					file_info_impl info;
					replaygain_info rg_info;

					if ( list[ j ]->get_info_async( info ) )
					{
						rg_info = info.get_replaygain();

						if ( rg_info.is_album_gain_present() && rg_info.is_album_peak_present() &&
							rg_info.is_track_peak_present() )
						{
							tags_present_count++;
						}
					}
				}

				if ( tags_present_count == list.get_count() )
				{
					albums_have_all_tags++;
					rg_albums.set( i, true );
				}
			}

			if ( albums_have_all_tags == nested_list.get_count() )
			{
				if ( uMessageBox( core_api::get_main_window(), "All tracks you have selected already have ReplayGain info. Would you like to scan them anyway?", "Information", MB_YESNO ) == IDNO )
				{
					return;
				}
			}
			else
			{
				nested_list.remove_mask( rg_albums );
			}

			for ( unsigned i = 0; i < nested_list.get_count(); i++ )
			{
				p_callback->add_job_album( nested_list[ i ] );
			}
		}

		threaded_process::g_run_modeless( p_callback, threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item | threaded_process::flag_show_delayed, core_api::get_main_window(), "EBU R128 Gain scan status" );
	}
};

static contextmenu_group_popup_factory g_contextmenu_group_r128_factory( guid_context_group_r128, contextmenu_groups::root, "R128Gain" );
static contextmenu_item_factory_t<context_r128> g_contextmenu_item_r128_factory;

DECLARE_COMPONENT_VERSION("EBU R128 Gain scanner", MY_VERSION,
	"EBU R128 Gain processor.\n"
	"\n"
	"Copyright (C) 2011 Chris Moeller\n"
	"\n"
	"Portions copyright (c) 2011 Jan Kokemüller\n"
	"\n"
	"Permission is hereby granted, free of charge, to any person obtaining a copy\n"
	"of this software and associated documentation files (the \"Software\"), to deal\n"
	"in the Software without restriction, including without limitation the rights\n"
	"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n"
	"copies of the Software, and to permit persons to whom the Software is\n"
	"furnished to do so, subject to the following conditions:\n"
	"\n"
	"The above copyright notice and this permission notice shall be included in\n"
	"all copies or substantial portions of the Software.\n"
	"\n"
	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
	"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n"
	"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n"
	"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
	"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n"
	"THE SOFTWARE."
);

VALIDATE_COMPONENT_FILENAME("foo_r128scan.dll");
