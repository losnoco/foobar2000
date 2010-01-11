#include <foobar2000.h>

#include "../ima_adpcm.h"

extern cfg_int cfg_loop;

static const char sig_header[] = {'S', 'C', 'H', 'l'};
static const char sig_header_pt[] = {'P', 'T', 0, 0};
static const char sig_header_patl[] = {'P', 'A', 'T', 'l'};
static const char sig_header_patl_tmpl[] = {'T', 'M', 'p', 'l'};

static const char sig_chunk_count[] = {'S', 'C', 'C', 'l'};

static const char sig_data[] = {'S', 'C', 'D', 'l'};

static const char sig_loop[] = {'S', 'C', 'L', 'l'};

static const char sig_end[] = {'S', 'C', 'E', 'l'};

static const char sig_map_header[] = {'P', 'F', 'D', 'x'};

static const int ea_adpcm_table[] = {
    0, 240, 460, 392, 0, 0, -208, -220, 0, 1,
    3, 4, 7, 8, 10, 11, 0, -1, -3, -4
};

class input_ea_mus
{
	service_ptr_t<file> m_file;

	pfc::array_t<t_uint8> data_buffer;
	pfc::array_t<t_uint8> sample_buffer;

	struct chunk_info
	{
		t_filesize offset;
		unsigned size;
		t_uint32 samples;
	};

	pfc::array_t< chunk_info > chunk_infos;

	unsigned current_chunk, samples_decoded, samples_skip;

	bool loop;

	unsigned sample_rate, channels, compression, num_samples, data_start,
		loop_offset, loop_length, bytes_per_sample, split, split_compression;

	unsigned read_var( file & p_file, abort_callback & p_abort )
	{
		t_uint8 len;
		p_file.read_object_t( len, p_abort );
		unsigned value = 0;
		while ( len-- )
		{
			t_uint8 byte;
			p_file.read_object_t( byte, p_abort );
			value = ( value << 8 ) | byte;
		}
		return value;
	}

	void decode_ima_mono( const t_uint8 * src, t_int16 * dst, unsigned count )
	{
		int index = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		int current_sample = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		if ( index < 0 || index > ISSTMAX ) return;
		for ( unsigned i = 0; i < count; ++i )
		{
			int code, sign;
			if ( i & 1 ) sign = ( *src++ ) & 0x0F;
			else sign = ( *src ) >> 4;
			code = sign & 7;
			int step = imaStepSizeTable[ index ];
			index = imaStateAdjustTable[ index ][ code ];
			int delta;
#ifdef STRICT_IMA
			delta = 0;
			if ( code & 4 ) delta += step;
			step >>= 1;
			if ( code & 2 ) delta += step;
			step >>= 1;
			if ( code & 1 ) delta += step;
			step >>= 1;
			delta += step;
#else
			delta = ( ( code * 2 + 1 ) * step ) >> 3;
#endif
			if ( code != sign )
			{
				current_sample -= delta;
				if ( current_sample < -32768 ) current_sample = -32768;
			}
			else
			{
				current_sample += delta;
				if ( current_sample > 32767 ) current_sample = 32767;
			}

			*dst++ = current_sample;
		}
	}

	void decode_ima_stereo( const t_uint8 * src, t_int16 * dst, unsigned count )
	{
		int index_l = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		int index_r = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		int current_sample_l = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		int current_sample_r = byte_order::dword_le_to_native( * ( ( t_int32 * ) src ) );
		src += 4;
		if ( index_l < 0 || index_l > ISSTMAX ||
			index_r < 0 || index_r > ISSTMAX ) return;
		for ( unsigned i = 0; i < count; ++i )
		{
			int code, sign;
			sign = ( *src ) >> 4;
			code = sign & 7;
			int step = imaStepSizeTable[ index_l ];
			index_l = imaStateAdjustTable[ index_l ][ code ];
			int delta;
#ifdef STRICT_IMA
			delta = 0;
			if ( code & 4 ) delta += step;
			step >>= 1;
			if ( code & 2 ) delta += step;
			step >>= 1;
			if ( code & 1 ) delta += step;
			step >>= 1;
			delta += step;
#else
			delta = ( ( code * 2 + 1 ) * step ) >> 3;
#endif
			if ( code != sign )
			{
				current_sample_l -= delta;
				if ( current_sample_l < -32768 ) current_sample_l = -32768;
			}
			else
			{
				current_sample_l += delta;
				if ( current_sample_l > 32767 ) current_sample_l = 32767;
			}

			sign = ( *src++ ) & 0x0F;
			code = sign & 7;
			step = imaStepSizeTable[ index_r ];
			index_r = imaStateAdjustTable[ index_r ][ code ];
#ifdef STRICT_IMA
			delta = 0;
			if ( code & 4 ) delta += step;
			step >>= 1;
			if ( code & 2 ) delta += step;
			step >>= 1;
			if ( code & 1 ) delta += step;
			step >>= 1;
			delta += step;
#else
			delta = ( ( code * 2 + 1 ) * step ) >> 3;
#endif
			if ( code != sign )
			{
				current_sample_r -= delta;
				if ( current_sample_r < -32768 ) current_sample_r = -32768;
			}
			else
			{
				current_sample_r += delta;
				if ( current_sample_r > 32767 ) current_sample_r = 32767;
			}

			*dst++ = current_sample_l;
			*dst++ = current_sample_r;
		}
	}

	void decode_ea_mono( const t_uint8 * src, t_int16 * dst, unsigned count )
	{
		int current_sample = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;
		int previous_sample = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;

		while ( count )
		{
			unsigned todo = count;
			if ( todo > 28 ) todo = 28;
			count -= todo;

			int coeff1 = ea_adpcm_table[ ( *src >> 4 ) & 0x0F ];
			int coeff2 = ea_adpcm_table[ ( ( *src >> 4 ) & 0x0F ) + 4 ];

			int shift = ( *src & 0x0F ) + ( sizeof( int ) * 8 - 24 );

			src++;

			for ( unsigned i = 0; i < todo; ++i )
			{
				int next_sample;
				if ( i & 1 ) next_sample = ( ( ( ( *src++ ) & 0x0F ) << ( sizeof( int ) * 8 - 4 ) ) >> shift );
				else next_sample = ( ( ( *src & 0xF0 ) << ( sizeof( int ) * 8 - 8 ) ) >> shift );

				next_sample = ( next_sample + ( current_sample * coeff1 ) + ( previous_sample * coeff2 ) + 0x80 ) >> 8;
				if ( next_sample != ( t_int16 ) next_sample )
					next_sample = 0x7FFF - ( next_sample >> ( sizeof( int ) * 8 - 1 ) );

				previous_sample = current_sample;
				current_sample = next_sample;

				*dst++ = current_sample;
			}
		}
	}

	void decode_ea_stereo( const t_uint8 * src, t_int16 * dst, unsigned count )
	{
		int current_sample_l = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;
		int previous_sample_l = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;
		int current_sample_r = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;
		int previous_sample_r = ( t_int16 ) byte_order::word_le_to_native( * ( ( t_uint16 * ) src ) );
		src += 2;

		while ( count )
		{
			unsigned todo = count;
			if ( todo > 28 ) todo = 28;
			count -= todo;

			int coeff1_l = ea_adpcm_table[ ( *src >> 4 ) & 0x0F ];
			int coeff2_l = ea_adpcm_table[ ( ( *src >> 4 ) & 0x0F ) + 4 ];
			int coeff1_r = ea_adpcm_table[ *src & 0x0F ];
			int coeff2_r = ea_adpcm_table[ ( *src & 0x0F ) + 4 ];
			src++;

			int shift_l = ( ( *src >> 4 ) & 0x0F ) + ( sizeof( int ) * 8 - 24 );
			int shift_r = ( *src & 0x0F ) + ( sizeof( int ) * 8 - 24 );
			src++;

			for ( unsigned i = 0; i < todo; ++i )
			{
				int next_sample_l = ( ( ( *src & 0xF0 ) << ( sizeof( int ) * 8 - 8 ) ) >> shift_l );
				int next_sample_r = ( ( ( *src & 0x0F ) << ( sizeof( int ) * 8 - 4 ) ) >> shift_r );
				src++;

				next_sample_l = ( next_sample_l + ( current_sample_l * coeff1_l ) + ( previous_sample_l * coeff2_l ) + 0x80 ) >> 8;
				next_sample_r = ( next_sample_r + ( current_sample_r * coeff1_r ) + ( previous_sample_r * coeff2_r ) + 0x80 ) >> 8;
				if ( next_sample_l != ( t_int16 ) next_sample_l )
					next_sample_l = 0x7FFF - ( next_sample_l >> ( sizeof( int ) * 8 - 1 ) );
				if ( next_sample_r != ( t_int16 ) next_sample_r )
					next_sample_r = 0x7FFF - ( next_sample_r >> ( sizeof( int ) * 8 - 1 ) );

				previous_sample_l = current_sample_l;
				current_sample_l = next_sample_l;
				previous_sample_r = current_sample_r;
				current_sample_r = next_sample_r;

				*dst++ = current_sample_l;
				*dst++ = current_sample_r;
			}
		}
	}

	void seek( unsigned sample_offset )
	{
		current_chunk = 0;
		samples_decoded = 0;

		if ( loop && sample_offset >= loop_length )
		{
			sample_offset = ( ( sample_offset - loop_offset ) % loop_length ) + loop_offset;
		}

		unsigned chunk_count = chunk_infos.get_size();
		while ( current_chunk < chunk_count )
		{
			chunk_info & info = chunk_infos[ current_chunk ];
			if ( samples_decoded + info.samples > sample_offset ) break;
			samples_decoded += info.samples;
			current_chunk++;
		}

		if ( current_chunk < chunk_count ) samples_skip = sample_offset - samples_decoded;
	}

public:
	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( m_file, p_path, filesystem::open_mode_read, p_abort );
		}
		else m_file = p_filehint;

		// check header, parse it
		data_buffer.set_size( 8 );

		m_file->read_object( data_buffer.get_ptr(), 8, p_abort );
		if ( memcmp( data_buffer.get_ptr(), sig_header, 4 ) ) throw exception_io_data();

		service_impl_single_t<reader_limited> m_file2;
		m_file2.init( m_file, 8, byte_order::dword_le_to_native( * ( ( t_uint32 * ) ( data_buffer.get_ptr() + 4 ) ) ), p_abort );

		m_file2.read_object( data_buffer.get_ptr(), 4, p_abort );

		if ( ! memcmp( data_buffer.get_ptr(), sig_header_pt, 4 ) )
		{
			sample_rate = 22050;
			channels = 2;
			compression = 0;
			num_samples = 0;
			data_start = 0;
			loop_offset = 0;
			loop_length = 0;
			bytes_per_sample = 2;
			split = 0;
			split_compression = 0;

			bool in_header, in_sub_header;

			in_header = true;
			while ( in_header )
			{
				t_uint8 byte;
				m_file2.read_object_t( byte, p_abort );
				switch ( byte )
				{
				case 0xFF:
					in_header = false;
				case 0xFE:
				case 0xFC:
					break;

				case 0xFD:
					in_sub_header = true;
					while ( in_sub_header )
					{
						m_file2.read_object_t( byte, p_abort );
						switch ( byte )
						{
						case 0x82:
							channels = read_var( m_file2, p_abort );
							break;

						case 0x83:
							compression = read_var( m_file2, p_abort );
							break;

						case 0x84:
							sample_rate = read_var( m_file2, p_abort );
							break;

						case 0x85:
							num_samples = read_var( m_file2, p_abort );
							break;

						case 0x86:
							loop_offset = read_var( m_file2, p_abort );
							break;

						case 0x87:
							loop_length = read_var( m_file2, p_abort );
							break;

						case 0x88:
							data_start = read_var( m_file2, p_abort );
							break;

						case 0x92:
							bytes_per_sample = read_var( m_file2, p_abort );
							break;

						case 0x80:
							split = read_var( m_file2, p_abort );
							break;

						case 0xA0:
							split_compression = read_var( m_file2, p_abort );
							break;

						case 0xFF: // ???
							in_sub_header = false;
							in_header = false;
							break;

						case 0x8A:
							in_sub_header = false;

						default:
							m_file2.read_object_t( byte, p_abort );
							m_file2.seek_ex( byte, file::seek_from_current, p_abort );
							break;
						}
					}
					break;

				default:
					m_file2.read_object_t( byte, p_abort );
					if ( byte == 0xFF )
						m_file2.seek_ex( 4, file::seek_from_current, p_abort );
					m_file2.seek_ex( byte, file::seek_from_current, p_abort );
					break;
				}
			}

			if ( ( channels != 1 && channels != 2 ) ||
				( compression != 0 && compression != 7 ) ||
				( split != 0 && split != 1 ) ||
				( split != 0 && channels == 1 ) ||
				( split_compression != 0 && split_compression != 8 ) ||
				( split == 0 && split_compression != 0 ) ||
				( split_compression == 8 && compression != 0 ) ||
				( split_compression == 0 && compression != 7 ) )
				throw exception_io_data();
		}
		else if ( ! memcmp( data_buffer.get_ptr(), sig_header_patl, 4 ) )
		{
			m_file2.read_object( data_buffer.get_ptr(), 4, p_abort );
			if ( memcmp( data_buffer.get_ptr(), sig_header_patl_tmpl, 4 ) )
				throw exception_io_data();

			union
			{
				t_uint8 byte;
				t_uint16 word;
				t_uint32 dword;
			};

			m_file2.read_object_t( byte, p_abort );
			m_file2.read_object_t( byte, p_abort );
			bytes_per_sample = byte;
			m_file2.read_object_t( byte, p_abort );
			channels = byte;
			m_file2.read_object_t( byte, p_abort );
			compression = byte;
			m_file2.read_lendian_t( word, p_abort );
			m_file2.read_lendian_t( word, p_abort );
			sample_rate = word;
			m_file2.read_lendian_t( dword, p_abort );
			num_samples = dword;

			if ( ( bytes_per_sample != 8 && bytes_per_sample != 16 ) ||
				( channels != 1 && channels != 2 ) ||
				( compression != 0 && compression != 2 ) ||
				( sample_rate == 0 ) )
				throw exception_io_data();

			bytes_per_sample /= 8;

			data_start = 0;
			loop_offset = 0;
			loop_length = 0;
			split = 0;
			split_compression = 0;
		}
		else throw exception_io_data();

		m_file2.seek_ex( 0, file::seek_from_eof, p_abort );

		t_uint32 chunk_count = 0, current_chunk = 0;

		unsigned num_samples = 0;

		while ( m_file->get_position( p_abort ) < m_file->get_size( p_abort ) )
		{
			m_file->read_object( data_buffer.get_ptr(), 8, p_abort );
			t_filesize offset = m_file->get_position( p_abort );
			m_file2.init( m_file, offset, offset - 8 + byte_order::dword_le_to_native( * ( ( t_uint32 * ) ( data_buffer.get_ptr() + 4 ) ) ), p_abort );

			if ( ! memcmp( data_buffer.get_ptr(), sig_chunk_count, 4 ) )
			{
				m_file2.read_lendian_t( chunk_count, p_abort );
				chunk_infos.set_size( chunk_count );
			}
			else if ( ! memcmp( data_buffer.get_ptr(), sig_data, 4 ) )
			{
				chunk_info info;

				info.offset = offset;
				info.size = m_file2.get_size( p_abort );
				m_file2.read_lendian_t( info.samples, p_abort );

				unsigned min_size = 4;
				if ( ! split )
				{
					if ( compression == 0 ) min_size += info.samples * bytes_per_sample * channels;
					else if ( compression == 2 ) min_size += 8 * channels + ( info.samples * channels + 1 ) / 2;
					else if ( compression == 7 ) min_size += 4 * channels + ( info.samples + 27) / ( 15 * channels ) * 28;
				}
				else
				{
					t_uint32 offset_left, offset_right;
					m_file2.read_lendian_t( offset_left, p_abort );
					m_file2.read_lendian_t( offset_right, p_abort );

					if ( offset_right > offset_left ) min_size += offset_right;
					else min_size += offset_left;

					if ( compression == 0 ) min_size += info.samples * bytes_per_sample;
					else if ( compression == 2 ) min_size += 8 + ( info.samples + 1 ) / 2;
					else if ( compression == 7 ) min_size += 4 + ( info.samples + 27 ) / 30 * 28;
				}

				if ( min_size > info.size ) throw exception_io_data();

				num_samples += info.samples;

				if ( current_chunk >= chunk_count )
				{
					chunk_infos.append_single( info );
					++ current_chunk;
					++ chunk_count;
				}
				else
				{
					chunk_infos[ current_chunk++ ] = info;
				}
			}
			else if ( ! memcmp( data_buffer.get_ptr(), sig_loop, 4 ) )
			{
				t_uint32 dword;

				m_file2.read_lendian_t( dword, p_abort );
				loop_offset = dword;

				loop_length = num_samples;
			}
			else if ( ! memcmp( data_buffer.get_ptr(), sig_end, 4 ) )
			{
				break;
			}

			m_file2.seek_ex( 0, file::seek_from_eof, p_abort );
		}

		if ( num_samples != this->num_samples )
		{
			console::formatter() << "Actual sample count differs from header. (Header: "
				<< this->num_samples << ", Actual: "
				<< num_samples << ")";
			this->num_samples = num_samples;
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set_int( "samplerate", sample_rate );
		p_info.info_set_int( "channels", channels );
		p_info.info_set( "codec", "EA MUS" );

		if ( loop_offset != 0xFFFFFFFF && loop_length != 0 )
		{
			p_info.info_set_int( "ea_loop_start", loop_offset );
			p_info.info_set_int( "ea_loop_end", loop_length );
		}

		p_info.set_length( double( num_samples ) / double( sample_rate ) );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		loop = cfg_loop && ! ( p_flags & input_flag_no_looping ) &&
			( loop_offset != 0xFFFFFFFF && loop_length != 0 );

		current_chunk = 0;
		samples_decoded = 0;
		samples_skip = 0;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( current_chunk == chunk_infos.get_size() )
			return false;

		const chunk_info & info = chunk_infos[ current_chunk++ ];

		data_buffer.grow_size( info.size );

		m_file->seek( info.offset, p_abort );
		m_file->read_object( data_buffer.get_ptr(), info.size, p_abort );

		if ( ! split )
		{
			if ( compression == 0 )
			{
				p_chunk.set_data_fixedpoint_ex( data_buffer.get_ptr() + 4, info.size - 4, sample_rate, channels, bytes_per_sample * 8, audio_chunk::FLAG_SIGNED | audio_chunk::FLAG_LITTLE_ENDIAN, audio_chunk::g_guess_channel_config( channels ) );
			}
			else if ( compression == 2 )
			{
				sample_buffer.grow_size( info.samples * channels * 2 );
				if ( channels == 1 ) decode_ima_mono( data_buffer.get_ptr() + 4, ( t_int16 * ) sample_buffer.get_ptr(), info.samples );
				else if ( channels == 2 ) decode_ima_stereo( data_buffer.get_ptr() + 4, ( t_int16 * ) sample_buffer.get_ptr(), info.samples );
				p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), info.samples * channels * 2, sample_rate, channels, 16, audio_chunk::g_guess_channel_config( channels ) );
			}
			else if ( compression == 7 )
			{
				sample_buffer.grow_size( info.samples * channels * 2 );
				if ( channels == 1 ) decode_ea_mono( data_buffer.get_ptr() + 4, ( t_int16 * ) sample_buffer.get_ptr(), info.samples );
				else if ( channels == 2 ) decode_ea_stereo( data_buffer.get_ptr() + 4, ( t_int16 * ) sample_buffer.get_ptr(), info.samples );
				p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), info.samples * channels * 2, sample_rate, channels, 16, audio_chunk::g_guess_channel_config( channels ) );
			}
		}
		else
		{
			if ( compression == 0 )
			{
				sample_buffer.grow_size( info.size - 4 );
				if ( bytes_per_sample == 1 )
				{
					const t_int8 * srcl = ( const t_int8 * ) ( data_buffer.get_ptr() + 4 );
					const t_int8 * srcr = srcl + info.samples;
					t_int8 * dst = ( t_int8 * ) sample_buffer.get_ptr();

					for ( unsigned i = info.samples; i--; )
					{
						*dst++ = *srcl++;
						*dst++ = *srcr++;
					}
				}
				else if ( bytes_per_sample == 2 )
				{
					const t_int16 * srcl = ( const t_int16 * ) ( data_buffer.get_ptr() + 4 );
					const t_int16 * srcr = srcl + info.samples;
					t_int16 * dst = ( t_int16 * ) sample_buffer.get_ptr();

					for ( unsigned i = info.samples; i--; )
					{
						*dst++ = *srcl++;
						*dst++ = *srcr++;
					}
				}

				p_chunk.set_data_fixedpoint_ex( sample_buffer.get_ptr(), info.size - 4, sample_rate, 2, bytes_per_sample * 8, audio_chunk::FLAG_SIGNED | audio_chunk::FLAG_LITTLE_ENDIAN, audio_chunk::channel_config_stereo );
			}
			else if ( compression == 2 )
			{
				sample_buffer.grow_size( info.samples * 2 * 2 );
				const t_uint8 * src = data_buffer.get_ptr();
				t_uint32 offset_left = byte_order::dword_le_to_native( * ( ( t_uint32 * ) src ) );
				src += 4;
				t_uint32 offset_right = byte_order::dword_le_to_native( * ( ( t_uint32 * ) src ) );
				src += 4;
				t_int16 * dst = ( t_int16 * ) sample_buffer.get_ptr();
				decode_ima_mono( src + offset_left, dst, info.samples );
				decode_ima_mono( src + offset_right, dst + info.samples, info.samples );

				data_buffer.grow_size( info.samples * 2 * 2 );

				const t_int16 * srcl = ( const t_int16 * ) sample_buffer.get_ptr();
				const t_int16 * srcr = srcl + info.samples;
				dst = ( t_int16 * ) data_buffer.get_ptr();

				for ( unsigned i = info.samples; i--; )
				{
					*dst++ = *srcl++;
					*dst++ = *srcr++;
				}

				p_chunk.set_data_fixedpoint( data_buffer.get_ptr(), info.samples * 2 * 2, sample_rate, 2, 16, audio_chunk::channel_config_stereo );
			}
			else if ( compression == 7 )
			{
				sample_buffer.grow_size( info.samples * 2 * 2 );
				const t_uint8 * src = data_buffer.get_ptr();
				t_uint32 offset_left = byte_order::dword_le_to_native( * ( ( t_uint32 * ) src ) );
				src += 4;
				t_uint32 offset_right = byte_order::dword_le_to_native( * ( ( t_uint32 * ) src ) );
				src += 4;
				t_int16 * dst = ( t_int16 * ) sample_buffer.get_ptr();
				decode_ea_mono( src + offset_left, dst, info.samples );
				decode_ea_mono( src + offset_right, dst + info.samples, info.samples );

				data_buffer.grow_size( info.samples * 2 * 2 );

				const t_int16 * srcl = ( const t_int16 * ) sample_buffer.get_ptr();
				const t_int16 * srcr = srcl + info.samples;
				dst = ( t_int16 * ) data_buffer.get_ptr();

				for ( unsigned i = info.samples; i--; )
				{
					*dst++ = *srcl++;
					*dst++ = *srcr++;
				}

				p_chunk.set_data_fixedpoint( data_buffer.get_ptr(), info.samples * 2 * 2, sample_rate, 2, 16, audio_chunk::channel_config_stereo );
			}
		}

		unsigned samples_max = loop ? loop_length : num_samples;
		unsigned samples_done = samples_decoded + p_chunk.get_sample_count();

		if ( samples_done > samples_max ) samples_done = samples_max;
		samples_done -= samples_decoded;

		samples_decoded += samples_done;

		if ( samples_skip )
		{
			samples_done -= samples_skip;
			audio_sample * ptr = p_chunk.get_data();
			memmove( ptr, ptr + samples_skip * channels, samples_done * channels * sizeof( audio_sample ) );
			samples_skip = 0;
		}

		p_chunk.set_sample_count( samples_done );

		if ( loop && samples_decoded >= loop_length )
		{
			seek( loop_offset );
		}

		return true;
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		seek( unsigned ( p_seconds * double( sample_rate ) ) );
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
		return ! stricmp( p_extension, "asf" ) || ! stricmp( p_extension, "mus" );
	}
};

class input_ea_map
{
	service_ptr_t<file> m_file;

	struct section
	{
		input_ea_mus m_decoder;
		file_info_impl m_info;

		unsigned next_section;
		bool played;
	};

	pfc::array_t< section > sections;

	unsigned first_section, current_section;

	bool loop;

	t_filestats m_stats;

public:
	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		if ( p_filehint.is_empty() )
		{
			filesystem::g_open( p_filehint, p_path, filesystem::open_mode_read, p_abort );
		}

		m_stats = p_filehint->get_stats( p_abort );
		if ( m_stats.m_size > 256 * 256 ) throw exception_io_data();

		pfc::array_t< t_uint8 > buffer;
		buffer.set_size( m_stats.m_size + 4 );

		t_uint8 * ptr = buffer.get_ptr();

		p_filehint->read_object( ptr, m_stats.m_size, p_abort );
		if ( m_stats.m_size < 12 || memcmp( ptr, sig_map_header, 4 ) ) throw exception_io_data();

		first_section = ptr[ 5 ];
		unsigned num_sections = ptr[ 6 ];
		unsigned record_size = ptr[ 7 ];
		unsigned num_records = ptr[ 11 ];

		if ( m_stats.m_size < ( 12 + num_sections * ( 28 + 4 ) + record_size * num_records ) ||
			first_section >= num_sections )
			throw exception_io_data();

		try
		{
			filesystem::g_open( m_file, string_replace_extension( p_path, "asf" ), filesystem::open_mode_read, p_abort );
		}
		catch ( const exception_io_not_found & )
		{
			filesystem::g_open( m_file, string_replace_extension( p_path, "mus" ), filesystem::open_mode_read, p_abort );
		}

		ptr += 12;
		sections.set_size( num_sections );

		t_uint32 * offsets = ( t_uint32 * ) ( ptr + num_sections * 28 + record_size * num_records );
		offsets[ num_sections ] = m_file->get_size( p_abort );

		for ( unsigned i = 0; i < num_sections; ++i )
		{
			t_uint8 * s = ptr + i * 28;

			if ( s[ 1 ] > 1 ) throw exception_io_data();

			service_ptr_t< reader_limited > partfile = new service_impl_t< reader_limited >;
			partfile->init( m_file, byte_order::dword_be_to_native( offsets[ i ] ), byte_order::dword_be_to_native( offsets[ i + 1 ] ), p_abort );
			service_ptr_t< file > m_file2 = partfile.get_ptr();

			section & sec = sections[ i ];

			sec.m_decoder.open( m_file2, "", p_reason, p_abort );
			sec.m_decoder.get_info( sections[ i ].m_info, p_abort );
			sec.m_decoder.decode_initialize( input_flag_playback | input_flag_no_looping, p_abort );

			sec.next_section = s[ 6 ];
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.copy( sections[ 0 ].m_info );

		double length = 0;
		for ( unsigned i = 0, j = sections.get_size(); i < j; ++i )
		{
			length += sections[ i ].m_info.get_length();
		}
		p_info.set_length( length );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		loop = cfg_loop && ! ( p_flags & input_flag_no_looping );

		current_section = first_section;

		for ( unsigned i = 0, j = sections.get_size(); i < j; ++i )
		{
			sections[ i ].played = false;
		}

		sections[ current_section ].m_decoder.decode_seek( 0, p_abort );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( current_section >= sections.get_size() )
			return false;

		for (;;)
		{
			p_abort.check();

			section & s = sections[ current_section ];

			if ( ! loop && s.played )
				return false;

			if ( s.m_decoder.decode_run( p_chunk, p_abort ) ) return true;

			s.played = true;
			current_section = s.next_section;
			sections[ current_section ].m_decoder.decode_seek( 0, p_abort );
		}
	}

	void decode_seek( double p_seconds, abort_callback & p_abort )
	{
		current_section = first_section;

		for ( unsigned i = 0, j = sections.get_size(); i < j; ++i )
		{
			sections[ i ].played = false;
		}

		while ( p_seconds >= sections[ current_section ].m_info.get_length() )
		{
			p_abort.check();

			section & s = sections[ current_section ];
			p_seconds -= s.m_info.get_length();
			s.played = true;
			current_section = s.next_section;
			if ( !loop && sections[ current_section ].played ) return;
		}

		sections[ current_section ].m_decoder.decode_seek( p_seconds, p_abort );
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
		return ! stricmp( p_extension, "lin" ) || ! stricmp( p_extension, "map" );
	}
};

static input_singletrack_factory_t<input_ea_mus> g_input_ea_mus_factory;
static input_singletrack_factory_t<input_ea_map> g_input_ea_map_factory;
