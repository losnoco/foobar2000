#define MYVERSION "1.0"

/*
	changelog

2010-07-03 23:34 UTC - kode54
- Updated to support the "new" SPU log format
- Version is now 1.0

2010-07-02 10:43 UTC - kode54
- Updated for foobar2000 1.0

2003-04-12 23:39 - kode54
- added bitspersample info

*/

#include <foobar2000.h>

#include "../helpers/file_cached.h"

#include "spu.h"
#include "spucore.h"

#include "PSXFilter.h"

struct WriteRegLogOld
{
	t_uint32 Tick;
	t_uint32 reg;
	t_uint32 val;
};

typedef struct
{
	long	y0, y1;
} ADPCM_Decode_t;

typedef struct
{                                                                   
	int				freq;
	int				nbits;
	int				stereo;
	int				nsamples;
	ADPCM_Decode_t	left, right;
	short			pcm[16384];
} xa_decode_t;

struct WriteRegLog
{
	enum
	{
		type_reg_write = 0,
		type_reg_read,
		type_dma_write,
		type_dma_read,
		type_play_adpcm,
		type_play_cdda
	};

	t_uint32 Tick;
	t_uint8 type;
	t_uint32 reg;
	t_uint16 val;
	pfc::array_t<xa_decode_t> xa;
	pfc::array_t<t_uint16> data;

	const WriteRegLog & operator= ( const WriteRegLog & src )
	{
		Tick = src.Tick;
		type = src.type;

		switch ( type )
		{
		case type_reg_write:
			reg = src.reg;
			val = src.val;
			break;
		case type_reg_read:
			reg = src.reg;
			break;
		case type_dma_write:
			reg = src.reg;
			data = src.data;
			break;
		case type_dma_read:
			reg = src.reg;
			break;
		case type_play_adpcm:
			xa = src.xa;
			break;
		case type_play_cdda:
			reg = src.reg;
			data = src.data;
			break;
		}
		return *this;
	}
};

static struct initializer { initializer() { spu_init(); spucore_init(); } } dostuff;

class input_spu
{
	pfc::array_t<signed short> samplebuffer;
	pfc::array_t<t_uint8> spu_state;

	pfc::array_t<WriteRegLog> log_data;
	t_uint32 TicksPerSecond;
	t_uint32 LogCount;
	t_uint32 CurrentLog;
	t_uint32 CurrentTick;
	t_uint32 remainder;
	t_uint32 length;

	service_ptr_t<file> m_file;

	CPSXFilter filter;

public:

	input_spu()
	{
	}

	~input_spu()
	{
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( m_file.is_empty() )
		{
			if ( p_reason == input_open_info_write ) throw exception_io_data();
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}

		service_ptr_t<file> m_temp = m_file;
		file_cached::g_create( m_file, m_temp, p_abort, 4096 );

		t_filesize sz = m_file->get_size_ex( p_abort );
		if ( sz < 524808 || sz > (1 << 30) ) throw exception_io_data();

		m_file->seek(0x80200, p_abort);
		m_file->read_lendian_t( TicksPerSecond, p_abort );
		m_file->read_lendian_t( LogCount, p_abort );

		if ( m_file->get_size_ex( p_abort ) == 0x80200 + 8 + LogCount * sizeof(WriteRegLogOld) )
		{
			m_file->seek( 0x80200 + 8 + sizeof(WriteRegLogOld) * (LogCount - 1), p_abort );
			m_file->read_object_t( length, p_abort );
		}
		else
		{
			length = TicksPerSecond;
			TicksPerSecond = 44100;
			LogCount = 0;
		}

		this->m_file = m_file;
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set_int( "samplerate", 44100 );
		p_info.info_set_int( "bitspersample", 16 );
		p_info.info_set_int( "channels", 2 );
		p_info.info_set("codec", "SPU log");
		p_info.set_length( (double)length / (double)TicksPerSecond );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		spu_state.set_size( spu_get_state_size(1) );
		void * ptr = spu_state.get_ptr();
		spu_clear_state( ptr, 1 );
		spu_enable_reverb( ptr, 1 );

		m_file->seek( 0, p_abort );

		pfc::array_t<short> data;
		unsigned long i;
#if 0
		// Could do it this way, but I don't know when the struct will change
		r->read(ptr, 0x80000);
#else
		// Safely upload by simulating DMA tranfer
		data.set_size(262144);
		short * pts = data.get_ptr();
		m_file->read( pts, 0x80000, p_abort );
		spu_sh( ptr, 0x1f801da6, 0 );
		for ( i=0; i < 0x40000; i++ )
			spu_sh(ptr, 0x1f801da8, pts[i]);
#endif
		data.set_size(256);
		pts = data.get_ptr();
		m_file->read( pts, 0x200, p_abort );
		for ( unsigned long reg = 0x1f801c00; reg <= 0x1f801dff; reg += 2 )
		{
			// ignore DMA registers
			if ( reg == 0x1f801da8 ) continue;
			spu_sh( ptr, reg, pts[(reg - 0x1f801c00) >> 1] );
		}

		if ( !log_data.get_size() )
		{
			if ( LogCount )
			{
				m_file->seek( 0x80200 + 8, p_abort );
				log_data.set_size( LogCount );
				
				for ( unsigned i = 0; i < LogCount; i++ )
				{
					t_uint32 val;
					m_file->read_lendian_t( val, p_abort );
					log_data[ i ].Tick = val;
					m_file->read_lendian_t( val, p_abort );
					log_data[ i ].reg = val;
					m_file->read_lendian_t( val, p_abort );
					log_data[ i ].val = (t_uint16) val;
					log_data[ i ].type = WriteRegLog::type_reg_write;
				}
			}
			else
			{
				m_file->seek( 0x80200 + 4, p_abort );
				
				while ( !m_file->is_eof( p_abort ) )
				{
					WriteRegLog data;

					m_file->read_lendian_t( data.Tick, p_abort );
					m_file->read_object_t( data.type, p_abort );

					switch ( data.type )
					{
					case WriteRegLog::type_reg_write:
						m_file->read_lendian_t( data.reg, p_abort );
						m_file->read_lendian_t( data.val, p_abort );
						break;

					case WriteRegLog::type_reg_read:
						m_file->read_lendian_t( data.reg, p_abort );
						break;

					case WriteRegLog::type_dma_write:
						{
							m_file->read_lendian_t( data.reg, p_abort );
							data.data.set_count( data.reg );
							t_uint16 * words = data.data.get_ptr();
							for ( unsigned i = 0; i < data.reg; i++ )
								m_file->read_lendian_t( words[ i ], p_abort );
						}
						break;

					case WriteRegLog::type_dma_read:
						m_file->read_lendian_t( data.reg, p_abort );
						break;

					case WriteRegLog::type_play_adpcm:
						{
							data.xa.set_count( 1 );
							xa_decode_t * xa = data.xa.get_ptr();
							m_file->read_lendian_t( xa->freq, p_abort );
							m_file->read_lendian_t( xa->nbits, p_abort );
							m_file->read_lendian_t( xa->stereo, p_abort );
							m_file->read_lendian_t( xa->nsamples, p_abort );
							m_file->read_lendian_t( xa->left.y0, p_abort );
							m_file->read_lendian_t( xa->left.y1, p_abort );
							m_file->read_lendian_t( xa->right.y0, p_abort );
							m_file->read_lendian_t( xa->right.y1, p_abort );
							for ( unsigned i = 0; i < 16384; i++ )
								m_file->read_lendian_t( xa->pcm[ i ], p_abort );
						}
						break;

					case WriteRegLog::type_play_cdda:
						{
							data.data.set_count( data.reg / 2 );
							t_int16 * samples = ( t_int16 * ) data.data.get_ptr();
							for ( unsigned i = 0, j = data.reg / 2; i < j; i++ )
								m_file->read_lendian_t( samples[ i ], p_abort );
						}
						break;

					default:
						throw exception_io_data( "Unknown event type" );
					}

					log_data.append_single( data );
					LogCount++;
				}
			}
		}

		CurrentLog = remainder = 0;
		CurrentTick = 0;

		/*char err[32];
		sprintf(err, "samplebuffer.get_ptr() = %x", samplebuffer.set_size(4096));
		OutputDebugString(err);*/

		samplebuffer.set_size(2048);

		filter.Redesign( 44100 );
		filter.Reset();
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (CurrentLog == LogCount && !remainder) return false;

		int samples, rendered, remain;
		signed short *p = samplebuffer.get_ptr();

		void *spu = spu_state.get_ptr();

		rendered = 0;

		do
		{
			if (remainder)
			{
				if (remainder > 1024)
				{
					/*sprintf(err, "SPUrender(%x, %d)", p, 4096);
					OutputDebugString(err);*/
					//				SPUrender(p, 4096);
					spu_render(spu, p, 1024);
					remainder -= 1024;
					rendered = 1024;
					break;
				}
				/*sprintf(err, "SPUrender(%x, %d)", p, remainder);
				OutputDebugString(err);*/
				//			SPUrender(p, remainder);
				spu_render(spu, p, remainder);
				p += remainder << 1;
				rendered = remainder;
				remainder = 0;
				if (CurrentLog == LogCount)
				{
					break;
				}
			}

			remain = 0;

			while ( CurrentLog < LogCount )
			{
				while ( ( CurrentLog < LogCount ) && ( log_data[ CurrentLog ].Tick <= CurrentTick ) )
				{
					//SPUwriteRegister(pLog[CurrentLog].reg, pLog[CurrentLog].val);
					WriteRegLog & entry = log_data[ CurrentLog ];
					switch ( entry.type )
					{
					case WriteRegLog::type_reg_write:
						spu_sh( spu, entry.reg, entry.val );
						break;
					case WriteRegLog::type_dma_write:
						spu_dma( spu, 0, entry.data.get_ptr(), 0, ~0, entry.reg * 2, 1 );
						break;
					case WriteRegLog::type_dma_read:
						t_uint32 temp;
						spu_dma( spu, 0, &temp, 0, 0, entry.reg * 2, 0 );
						break;
					}
					CurrentLog++;
				}
				if ( CurrentLog == LogCount )
				{
					samples = length - log_data[ LogCount - 1 ].Tick;
				}
				else
				{
					if (TicksPerSecond != 44100)
					{
						samples = (log_data[CurrentLog].Tick - CurrentTick) * 44100 + remain;
						remain = samples % TicksPerSecond;
						samples = samples / TicksPerSecond;
					}
					else
						samples = log_data[CurrentLog].Tick - CurrentTick;
					CurrentTick = log_data[CurrentLog].Tick;
				}
				if (samples > (1024 - rendered))
				{
					remainder = samples - (1024 - rendered);
					samples = 1024 - rendered;
					rendered = 1024;
					/*sprintf(err, "SPUrender(%x, %d)", p, samples);
					OutputDebugString(err);*/
					//				SPUrender(p, samples);
					spu_render(spu, p, samples);
					break;
				}
				if (samples)
				{
					/*sprintf(err, "SPUrender(%x, %d)", p, samples);
					OutputDebugString(err);*/
					//				SPUrender(p, samples);
					spu_render(spu, p, samples);
					rendered += samples;
					p += samples << 1;
				}
			}
		} while (0);

		if ( rendered )
		{
			p_chunk.set_data_fixedpoint( samplebuffer.get_ptr(), rendered * 4, 44100, 2, 16, audio_chunk::channel_config_stereo );

			filter.Process( p_chunk.get_data(), rendered );

			return true;
		}

		return false;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		throw exception_io_data();
	}
	bool decode_can_seek()
	{
		return false;
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
		m_file->on_idle( p_abort );
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
		return !stricmp( p_extension, "spu" );
	}
};

DECLARE_FILE_TYPE( "SPU log files", "*.SPU" );

static input_singletrack_factory_t<input_spu> foo;

DECLARE_COMPONENT_VERSION( "SPU log decoder", MYVERSION, "" );
