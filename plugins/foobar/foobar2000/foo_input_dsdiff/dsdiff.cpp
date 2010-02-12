#define MYVERSION "1.2"

/*
	changelog

2010-01-11 11:47 UTC - kode54
- Updated preferences page to 1.0 API
- Version is now 1.2

2009-11-11 21:00 UTC - kode54
- Whoops, forgot to uncomment foobar2000 resampler reset code on seek
- Version is now 1.1

2009-11-10 19:07 UTC - kode54
- Initial release.

2009-11-10 03:42 UTC - kode54
- Began work.

*/

#include <foobar2000.h>
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "DST/DSTDecoder.h"

#include "resource.h"

// {B735F284-607A-4ec2-91A0-AC761AF90B7E}
static const GUID guid_cfg_max_sample_rate = 
{ 0xb735f284, 0x607a, 0x4ec2, { 0x91, 0xa0, 0xac, 0x76, 0x1a, 0xf9, 0xb, 0x7e } };
// {CC7840A4-D540-4650-913F-5FD5C7A6A737}
static const GUID guid_cfg_history_rate = 
{ 0xcc7840a4, 0xd540, 0x4650, { 0x91, 0x3f, 0x5f, 0xd5, 0xc7, 0xa6, 0xa7, 0x37 } };

enum
{
	default_cfg_max_sample_rate = 88200
};

static cfg_int cfg_max_sample_rate( guid_cfg_max_sample_rate, default_cfg_max_sample_rate );

/**
 * DSD 2 PCM: Stage 1:
 * Decimate by factor 8
 * (one byte (8 samples) -> one float sample)
 * The bits are processed from least signicifant to most signicicant.
 * @author Sebastian Gesemann
 */
class resampler_Stage1 {

	/*
	* This is the 2nd half of an even order symmetric FIR
	* lowpass filter (to be used on a signal sampled at 44100*64 Hz)
	* Passband is 0-24 kHz (ripples +/- 0.025 dB)
	* Stopband starts at 176.4 kHz (rejection: 170 dB)
	* The overall gain is 2.0
	*/
	static const float FILTER_COEFFS[64];

	int FILT_LOOKUP_PARTS;
	float * FILT_LOOKUP_TABLE;
	byte * REVERSE_BITS;
	int FIFO_LENGTH;
	int FIFO_OFS_MASK;

	int * fifo;
	int fpos;

public:
	resampler_Stage1()
	{
		FILT_LOOKUP_PARTS = ( tabsize( FILTER_COEFFS ) + 7 ) / 8;
		// The current 128 tap FIR leads to an 8 KB lookup table
		FILT_LOOKUP_TABLE = new float[ FILT_LOOKUP_PARTS << 8 ];
		double * temp = new double[0x100];
		for ( int part=0, sofs=0, dofs=0; part < FILT_LOOKUP_PARTS; )
		{
			memset( temp, 0, 0x100 * sizeof( double ) );
			for ( int bit=0, bitmask=0x80; bit<8 && sofs+bit < tabsize(FILTER_COEFFS); )
			{
				double coeff = FILTER_COEFFS[ sofs + bit ];
				for ( int bite=0; bite < 0x100; bite++ )
				{
					if ( ( bite & bitmask ) == 0 )
					{
						temp[ bite ] -= coeff;
					} else {
						temp[ bite ] += coeff;
					}
				}
				bit++;
				bitmask >>= 1;
			}
			for ( int s = 0; s < 0x100; ) {
				FILT_LOOKUP_TABLE[dofs++] = (float) temp[s++];
			}
			part++;
			sofs += 8;
		}
		delete [] temp;
		{ // calculate FIFO stuff
			int k = 1;
			while (k<FILT_LOOKUP_PARTS*2) k<<=1;
			FIFO_LENGTH = k;
			FIFO_OFS_MASK = k-1;
		}
		REVERSE_BITS = new t_uint8[0x100];
		for (int i=0, j=0; i<0x100; i++) {
			REVERSE_BITS[i] = ( t_uint8 ) j;
			// "reverse-increment" of j
			for (int bitmask=0x80;;) {
				if (((j^=bitmask) & bitmask)!=0) break;
				if (bitmask==1) break;
				bitmask >>= 1;
			}
		}

		fifo = new int[ FIFO_LENGTH ];

		reset();
	}

	~resampler_Stage1()
	{
		delete [] fifo;
		delete [] REVERSE_BITS;
		delete [] FILT_LOOKUP_TABLE;
	}

	void reset() {
		for (int i=0; i<FILT_LOOKUP_PARTS; i++) {
			fifo[i] = 0x55;
			fifo[i+FILT_LOOKUP_PARTS] = 0xAA;
		}
		fpos = FILT_LOOKUP_PARTS;
	}

	void process(t_uint8 * src, int sofs, int sinc, float * dest, int dofs, int dinc, int len)
	{
		int bite1, bite2, temp;
		float sample;
		while ( len > 0 )
		{
			fifo[ fpos ] = REVERSE_BITS[ fifo[ fpos ] ] & 0xFF;
			fifo[ ( fpos + FILT_LOOKUP_PARTS ) & FIFO_OFS_MASK ] = src[ sofs ] & 0xFF;
			sofs += sinc;
			temp = ( fpos + 1 ) & FIFO_OFS_MASK;
			sample = 0;
			for ( int k=0, lofs=0; k < FILT_LOOKUP_PARTS; )
			{
				bite1 = fifo[ ( fpos - k ) & FIFO_OFS_MASK ];
				bite2 = fifo[ ( temp + k ) & FIFO_OFS_MASK ];
				sample += FILT_LOOKUP_TABLE[ lofs + bite1 ] + FILT_LOOKUP_TABLE[ lofs + bite2 ];
				k++;
				lofs += 0x100;
			}
			fpos = temp;
			dest[ dofs ] = sample;
			dofs += dinc;
			len--;
		}
	}

};

const float resampler_Stage1::FILTER_COEFFS[64] =
{
	0.09712411121659f, 0.09613438994044f, 0.09417884216316f, 0.09130441727307f,
	0.08757947648990f, 0.08309142055179f, 0.07794369263673f, 0.07225228745463f,
	0.06614191680338f, 0.05974199351302f, 0.05318259916599f, 0.04659059631228f,
	0.04008603356890f, 0.03377897290478f, 0.02776684382775f, 0.02213240062966f,
	0.01694232798846f, 0.01224650881275f, 0.00807793792573f, 0.00445323755944f,
	0.00137370697215f,-0.00117318019994f,-0.00321193033831f,-0.00477694265140f,
	-0.00591028841335f,-0.00665946056286f,-0.00707518873201f,-0.00720940203988f,
	-0.00711340642819f,-0.00683632603227f,-0.00642384017266f,-0.00591723006715f,
	-0.00535273320457f,-0.00476118922548f,-0.00416794965654f,-0.00359301524813f,
	-0.00305135909510f,-0.00255339111833f,-0.00210551956895f,-0.00171076760278f,
	-0.00136940723130f,-0.00107957856005f,-0.00083786862365f,-0.00063983084245f,
	-0.00048043272086f,-0.00035442550015f,-0.00025663481039f,-0.00018217573430f,
	-0.00012659899635f,-0.00008597726991f,-0.00005694188820f,-0.00003668060332f,
	-0.00002290670286f,-0.00001380895679f,-0.00000799057558f,-0.00000440385083f,
	-0.00000228567089f,-0.00000109760778f,-0.00000047286430f,-0.00000017129652f,
	-0.00000004282776f, 0.00000000119422f, 0.00000000949179f, 0.00000000747450f
};



class reader_limited_v2 : public file_readonly {
	service_ptr_t<file> r;
	t_filesize begin;
	t_filesize end;
	t_filesize pos;
	
public:
	reader_limited_v2() {begin=0;end=0;}
	reader_limited_v2(const service_ptr_t<file> & p_r,t_filesize p_begin,t_filesize p_end) {
		r = p_r;
		begin = p_begin;
		end = p_end;
		pos = p_begin;
	}

	void init(const service_ptr_t<file> & p_r,t_filesize p_begin,t_filesize p_end) {
		r = p_r;
		begin = p_begin;
		end = p_end;
		pos = p_begin;
	}

	t_filetimestamp get_timestamp(abort_callback & p_abort) {return r->get_timestamp(p_abort);}

	t_size read(void *p_buffer, t_size p_bytes,abort_callback & p_abort) {
		t_filesize pos;
		pos = r->get_position(p_abort);
		if ( pos != this->pos )
		{
			pos = this->pos;
			r->seek( pos, p_abort );
		}
		if (p_bytes > end - pos) p_bytes = (t_size)(end - pos);
		t_size read = r->read(p_buffer,p_bytes,p_abort);
		this->pos += read;
		return read;
	}

	t_filesize get_size(abort_callback & p_abort) {return end-begin;}

	t_filesize get_position(abort_callback & p_abort) {
		return pos - begin;
	}

	void seek(t_filesize position,abort_callback & p_abort) {
		pos = position + begin;
	}
	bool can_seek() {return r->can_seek();}
	bool is_remote() {return r->is_remote();}
	
	bool get_content_type(pfc::string_base &) {return false;}

	void reopen(abort_callback & p_abort) {seek(0,p_abort);}
};

class audio_chunk_temp_impl_mod : public audio_chunk {
public:
	audio_chunk_temp_impl_mod(const audio_sample * p_data,t_size p_samples,t_uint32 p_sample_rate,t_uint32 p_channels,t_uint32 p_channel_config) :
	m_data(p_data), m_samples(p_samples), m_sample_rate(p_sample_rate), m_channels(p_channels), m_channel_config(p_channel_config)
	{
	}

	audio_sample * get_data() {throw pfc::exception_not_implemented();}
	const audio_sample * get_data() const {return m_data;}
	t_size get_data_size() const {return m_samples * m_channels;}
	void set_data_size(t_size p_new_size) {throw pfc::exception_not_implemented();}
	
	unsigned get_srate() const {return m_sample_rate;}
	void set_srate(unsigned val) {throw pfc::exception_not_implemented();}
	unsigned get_channels() const {return m_channels;}
	unsigned get_channel_config() const {return m_channel_config;}
	void set_channels(unsigned p_count,unsigned p_config) {throw pfc::exception_not_implemented();}

	t_size get_sample_count() const {return m_samples;}
	
	void set_sample_count(t_size val) {throw pfc::exception_not_implemented();}

private:
	t_size m_samples;
	t_uint32 m_sample_rate,m_channels,m_channel_config;
	const audio_sample * m_data;
};

class iff_stream
{
public:
	struct iff_packet_base
	{
		enum
		{
			packet_data,
			packet_file,
			packet_container
		} type;
		char id[4];
		virtual ~iff_packet_base() {}
	};

	struct iff_packet_data : iff_packet_base
	{
		pfc::array_t<t_uint8> data;
		iff_packet_data()
		{
			type = packet_data;
		}
		virtual ~iff_packet_data() {}
	};

	struct iff_packet_file : iff_packet_base
	{
		service_ptr_t<file> data;
		iff_packet_file()
		{
			type = packet_file;
		}
		virtual ~iff_packet_file() {}
	};

	struct iff_packet_container : iff_packet_base
	{
		pfc::ptr_list_t<iff_packet_base> packets;
		iff_packet_container()
		{
			type = packet_container;
		}
		virtual ~iff_packet_container() { packets.delete_all(); }

		bool find_packet( char id [4], const iff_packet_base * & out, unsigned & index, unsigned start = 0 ) const
		{
			for ( unsigned i = start, j = packets.get_count(); i < j; i++ )
			{
				if ( !memcmp( packets[ i ]->id, id, 4 ) )
				{
					out = packets[ i ];
					index = i;
					return true;
				}
			}
			return false;
		}
	};

private:
	void parse_chunks( iff_packet_container & p_container, service_ptr_t<file> & p_file, abort_callback & p_abort )
	{
		char id [4];
		t_filesize length;
		t_filesize chunk_end;

		while ( p_file->get_position( p_abort ) < p_file->get_size_ex( p_abort ) )
		{
			p_file->read_object( id, 4, p_abort );

			if ( &p_container == &m_container )
			{
				if ( memcmp( p_container.id, "FRM8", 4 ) && memcmp( id, "FRM8", 4 ) ) throw exception_io_data( "Not a DSDIFF file" );
			}

			p_file->read_bendian_t( length, p_abort );

			if ( length + 12 > p_file->get_size_ex( p_abort ) ) throw exception_io_data_truncation( "Chunk size too large" );

			chunk_end = p_file->get_position( p_abort ) + length;
			chunk_end = ( chunk_end + 1 ) & ~1;

			if ( !memcmp( id, "FRM8", 4 ) || !memcmp( id, "PROP", 4 ) )
			{
				char type [4];
				p_file->read_object( type, 4, p_abort );
				if ( !memcmp( id, "FRM8", 4 ) && memcmp( type, "DSD ", 4 ) ) throw exception_io_data( "Not a DSDIFF file" );
				else if ( !memcmp( id, "PROP", 4 ) && memcmp( type, "SND ", 4 ) ) throw exception_io_data( "Invalid PROP chunk" );

				t_filesize pos = p_file->get_position( p_abort );
				service_ptr_t<file> p_file_temp = new service_impl_t<reader_limited_v2> ( p_file, pos, pos + length - 4 );

				if ( !memcmp( id, "FRM8", 4 ) )
				{
					memcpy( p_container.id, id, 4 );
					parse_chunks( p_container, p_file_temp, p_abort );
				}
				else
				{
					iff_packet_container * m_chunk = new iff_packet_container;
					memcpy( m_chunk->id, id, 4 );
					try
					{
						parse_chunks( *m_chunk, p_file_temp, p_abort );
						p_container.packets.add_item( m_chunk );
					}
					catch (...)
					{
						delete m_chunk;
						throw;
					}
				}
			}
			else if ( !memcmp( id, "DST ", 4 ) || !memcmp( id, "DIIN", 4 ) )
			{
				t_filesize pos = p_file->get_position( p_abort );
				service_ptr_t<file> p_file_temp = new service_impl_t<reader_limited_v2> ( p_file, pos, pos + length );

				iff_packet_container * m_chunk = new iff_packet_container;
				memcpy( m_chunk->id, id, 4 );
				try
				{
					parse_chunks( *m_chunk, p_file_temp, p_abort );
					p_container.packets.add_item( m_chunk );
				}
				catch (...)
				{
					delete m_chunk;
					throw;
				}
			}
			else if ( !memcmp( id, "DSD ", 4 ) || !memcmp( id, "DSTF", 4 ) || length > 128 * 1024 )
			{
				t_filesize pos = p_file->get_position( p_abort );
				service_ptr_t<file> p_file_temp = new service_impl_t<reader_limited_v2> ( p_file, pos, pos + length );

				iff_packet_file * m_chunk = new iff_packet_file;
				memcpy( m_chunk->id, id, 4 );
				m_chunk->data = p_file_temp;
				try
				{
					p_container.packets.add_item( m_chunk );
				}
				catch (...)
				{
					delete m_chunk;
					throw;
				}
			}
			else
			{
				iff_packet_data * m_chunk = new iff_packet_data;
				memcpy( m_chunk->id, id, 4 );
				try
				{
					m_chunk->data.set_size( length );
					p_file->read_object( m_chunk->data.get_ptr(), length, p_abort );
					p_container.packets.add_item( m_chunk );
				}
				catch (...)
				{
					delete m_chunk;
					throw;
				}
			}

			p_file->seek( chunk_end, p_abort );
		}
	}

public:
	iff_stream() {}

	iff_stream( service_ptr_t< file > & p_file, abort_callback & p_abort )
	{
		memset( m_container.id, 0, 4 );
		parse_chunks( m_container, p_file, p_abort );
	}

	void init( service_ptr_t< file > & p_file, abort_callback & p_abort )
	{
		memset( m_container.id, 0, 4 );
		parse_chunks( m_container, p_file, p_abort );
	}

	inline const iff_packet_container & get() const
	{
		return m_container;
	}

	/*void dump( abort_callback & p_abort )
	{
		dump_chunk( m_container, 0, p_abort );
	}

private:
	void dump_chunk( const iff_packet_base & p_chunk, unsigned indent, abort_callback & p_abort )
	{
		pfc::string8 line;

		line.add_chars( '-', indent );
		line.add_string( p_chunk.id, 4 );

		if ( p_chunk.type == iff_packet_base::packet_container )
		{
			console::print( line );

			const iff_packet_container & p_container = reinterpret_cast<const iff_packet_container&> ( p_chunk );
			for ( unsigned i = 0, j = p_container.packets.get_count(); i < j; i++ )
			{
				dump_chunk( *p_container.packets[ i ], indent + 1, p_abort );
			}
		}
		else
		{
			line += " - ";

			if ( p_chunk.type == iff_packet_base::packet_data )
			{
				const iff_packet_data & p_data = reinterpret_cast<const iff_packet_data&> ( p_chunk );

				line += pfc::format_int( p_data.data.get_size() );
			}
			else if ( p_chunk.type == iff_packet_base::packet_file )
			{
				const iff_packet_file & p_file = reinterpret_cast<const iff_packet_file&> ( p_chunk );

				line += pfc::format_int( p_file.data->get_size_ex( p_abort ) );

				line += " (file reference)";
			}

			console::print( line );
		}
	}*/

private:
	iff_packet_container m_container;
};

class input_dsdiff
{
	iff_stream m_stream;

	service_ptr_t<file> m_file, m_file_stream;

	bool is_dst, first_frame;

	unsigned sample_rate, max_sample_rate, channels, channel_mask, bits_discard, eof;

	// DST
	unsigned frame_rate, frame_count, frame_number, bytes_discard;

	const iff_stream::iff_packet_container * dst_chunks;

	ebunch * Decoder;

	pfc::array_t< t_uint8 > channel_offsets;

	pfc::array_t< t_uint8 > sample_buffer;
	pfc::array_t< audio_sample > sample_buffer_out;

	pfc::array_t< t_uint8 > dst_buffer;

	service_ptr_t< dsp > m_resampler;

	pfc::array_t< resampler_Stage1 > m_resamplers;

public:
	input_dsdiff()
	{
		Decoder = 0;

		max_sample_rate = cfg_max_sample_rate;
	}

	~input_dsdiff()
	{
		if ( Decoder )
		{
			try
			{
				Close( Decoder );
			}
			catch (std::bad_alloc &)
			{
				throw;
			}
			catch (std::exception & e)
			{
				throw exception_io_data( e.what() );
			}
			delete Decoder;
		}
	}

	void open( service_ptr_t<file> p_filehint, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();

		m_file = p_filehint;

		input_open_file_helper( m_file, p_path, p_reason, p_abort );

		m_stream.init( m_file, p_abort );

		const iff_stream::iff_packet_container & p_container = m_stream.get();
		const iff_stream::iff_packet_base * p_packet;
		unsigned index;

		if ( p_container.find_packet( "FVER", p_packet, index ) )
		{
			if ( p_container.find_packet( "FVER", p_packet, index, index + 1 ) )
			{
				throw exception_io_data( "More than one FVER chunk" );
			}

			if ( p_packet->type != iff_stream::iff_packet_base::packet_data )
			{
				throw exception_io_data( "FVER chunk too large" );
			}

			const iff_stream::iff_packet_data & p_data = reinterpret_cast< const iff_stream::iff_packet_data & > ( *p_packet );

			if ( p_data.data.get_size() != 4 )
			{
				throw exception_io_data( "FVER chunk wrong size" );
			}

			t_uint32 version = pfc::byteswap_if_le_t( * ( t_uint32 * ) p_data.data.get_ptr() );

			if ( version < 0x1000000 || version > 0x1050000 )
			{
				throw exception_io_data( "File version not supported" );
			}
		}
		else
		{
			throw exception_io_data( "FVER chunk missing" );
		}

		if ( p_container.find_packet( "PROP", p_packet, index ) )
		{
			if ( p_container.find_packet( "PROP", p_packet, index, index + 1 ) )
			{
				throw exception_io_data( "More than one PROP chunk" );
			}

			if ( p_packet->type != iff_stream::iff_packet_base::packet_container )
			{
				throw exception_io_data( "PROP not parsed as container" );
			}

			bool found_fs = false, found_chnl = false, found_cmpr = false, found_abss = false, found_lsco = false;

			const iff_stream::iff_packet_container & p_props = reinterpret_cast< const iff_stream::iff_packet_container & > ( *p_packet );

			for ( unsigned i = 0, j = p_props.packets.get_count(); i < j; i++ )
			{
				p_packet = p_props.packets[ i ];
				if ( p_packet->type != iff_stream::iff_packet_base::packet_data )
				{
					throw exception_io_data( "PROP sub-packet too large" );
				}
				const iff_stream::iff_packet_data & p_data = reinterpret_cast< const iff_stream::iff_packet_data & > ( *p_packet );
				if ( !memcmp( p_data.id, "FS  ", 4 ) )
				{
					if ( found_fs )
					{
						throw exception_io_data( "Multiple PROP FS chunks found" );
					}
					if ( p_data.data.get_size() != 4 )
					{
						throw exception_io_data( "PROP FS chunk wrong size" );
					}
					found_fs = true;
					sample_rate = pfc::byteswap_if_le_t( * ( t_uint32 * ) p_data.data.get_ptr() );
				}
				else if ( !memcmp( p_data.id, "CHNL", 4 ) )
				{
					if ( found_chnl )
					{
						throw exception_io_data( "Multiple PROP CHNL chunks found" );
					}
					if ( p_data.data.get_size() < 2 )
					{
						throw exception_io_data( "PROP CHNL chunk too small" );
					}
					channels = pfc::byteswap_if_le_t( * ( t_uint16 * ) p_data.data.get_ptr() );
					if ( p_data.data.get_size() < 2 + channels * 4 )
					{
						throw exception_io_data( "PROP CHNL chunk too small" );
					}
					channel_mask = 0;
					pfc::array_t<t_uint8> channel_offsets;
					this->channel_offsets.set_size( channels );
					channel_offsets.set_size( channels );
					for ( unsigned i = 0, j = channels; i < j; i++ )
					{
						const char * channel_id = ( const char * ) p_data.data.get_ptr() + 2 + i * 4;
						unsigned channel_number = 0;
						if ( !memcmp( channel_id, "SLFT", 4 ) ) channel_number = 0;
						else if ( !memcmp( channel_id, "SRGT", 4 ) ) channel_number = 1;
						else if ( !memcmp( channel_id, "C   ", 4 ) ) channel_number = 2;
						else if ( !memcmp( channel_id, "LFE ", 4 ) ) channel_number = 3;
						else if ( !memcmp( channel_id, "LS  ", 4 ) ) channel_number = 4;
						else if ( !memcmp( channel_id, "RS  ", 4 ) ) channel_number = 5;
						else if ( !memcmp( channel_id, "MLFT", 4 ) ) channel_number = 9;
						else if ( !memcmp( channel_id, "MRGT", 4 ) ) channel_number = 10;
						else console::formatter() << "Unknown channel ID: " << pfc::string8( channel_id, 4 );
						channel_mask |= 1 << channel_number;
						channel_offsets[ i ] = channel_number;
					}
					if ( channels == 2 && channel_mask == ( 1 | 2 ) )
					{
						if ( channel_offsets[ 0 ] != 0 || channel_offsets[ 1 ] != 1 )
						{
							throw exception_io_data( "Invalid channel order" );
						}
					}
					else if ( channels == 5 && channel_mask == ( 1 | 2 | 4 | 16 | 32 ) )
					{
						if ( channel_offsets[ 0 ] != 0 || channel_offsets[ 1 ] != 1 ||
							channel_offsets[ 2 ] != 2 || channel_offsets[ 3 ] != 4 ||
							channel_offsets[ 4 ] != 5 )
						{
							throw exception_io_data( "Invalid channel order" );
						}
					}
					else if ( channels == 6 && channel_mask == ( 1 | 2 | 4 | 8 | 16 | 32 ) )
					{
						if ( channel_offsets[ 0 ] != 0 || channel_offsets[ 1 ] != 1 ||
							channel_offsets[ 2 ] != 2 || channel_offsets[ 3 ] != 3 ||
							channel_offsets[ 4 ] != 4 || channel_offsets[ 5 ] != 5 )
						{
							throw exception_io_data( "Invalid channel order" );
						}
					}
					unsigned j = 0;
					for ( unsigned i = 0; i < 32 && j < channels; i++ )
					{
						if ( channel_mask & ( 1 << i ) )
						{
							for ( unsigned k = 0, l = channels; k < l; k++ )
							{
								if ( channel_offsets[ k ] == i )
								{
									this->channel_offsets[ k ] = j;
								}
							}
							j++;
						}
					}
					if ( j < channels )
					{
						throw exception_io_data( "Duplicate channels found" );
					}
					found_chnl = true;
				}
				else if ( !memcmp( p_data.id, "CMPR", 4 ) )
				{
					if ( found_cmpr )
					{
						throw exception_io_data( "Multiple PROP CMPR chunks found" );
					}

					if ( p_data.data.get_size() < 5 )
					{
						throw exception_io_data( "PROP CMPR chunk too small" );
					}

					if ( memcmp( p_data.data.get_ptr(), "DSD ", 4 ) && memcmp( p_data.data.get_ptr(), "DST ", 4 ) )
					{
						throw exception_io_data( pfc::string8() << "Unsupported compression type: " << pfc::string8( ( const char * ) p_data.data.get_ptr() + 5, ( t_size ) * ( p_data.data.get_ptr() + 4 ) ) );
					}

					is_dst = !memcmp( p_data.data.get_ptr(), "DST ", 4 );

					found_cmpr = true;
				}
				else if ( !memcmp( p_data.id, "ABSS", 4 ) )
				{
					if ( found_abss )
					{
						throw exception_io_data( "Multiple PROP ABSS chunks found" );
					}

					if ( p_data.data.get_size() != 8 )
					{
						throw exception_io_data( "PROP ABSS chunk wrong size" );
					}

					found_abss = true;
				}
				else if ( !memcmp( p_data.id, "LSCO", 4 ) )
				{
					if ( found_lsco )
					{
						throw exception_io_data( "Multiple PROP LSCO chunks found" );
					}

					if ( p_data.data.get_size() != 2 )
					{
						throw exception_io_data( "PROP LSCO chunk wrong size" );
					}

					found_lsco = true;
				}
			}

			if ( !found_fs || !found_chnl || !found_cmpr )
			{
				throw exception_io_data( "Required PROP chunks missing" );
			}
		}
		else
		{
			throw exception_io_data( "PROP chunk missing" );
		}

		if ( !is_dst )
		{
			if ( p_container.find_packet( "DSD ", p_packet, index ) )
			{
				if ( p_container.find_packet( "DSD ", p_packet, index, index + 1 ) )
				{
					throw exception_io_data( "More than one DSD chunk" );
				}

				if ( p_packet->type != iff_stream::iff_packet_base::packet_file )
				{
					throw exception_io_data( "DSD chunk not file range" );
				}

				const iff_stream::iff_packet_file & p_file = reinterpret_cast< const iff_stream::iff_packet_file & > ( *p_packet );
				m_file_stream = p_file.data;
			}
			else
			{
				throw exception_io_data( "DSD chunk missing" );
			}
		}
		else
		{
			if ( p_container.find_packet( "DST ", p_packet, index ) )
			{
				if ( p_container.find_packet( "DST ", p_packet, index, index + 1 ) )
				{
					throw exception_io_data( "More than one DST chunk" );
				}

				if ( p_packet->type != iff_stream::iff_packet_base::packet_container )
				{
					throw exception_io_data( "DST chunk not container" );
				}

				const iff_stream::iff_packet_container & p_dst = reinterpret_cast< const iff_stream::iff_packet_container & > ( *p_packet );

				p_packet = p_dst.packets[ 0 ];
				if ( memcmp( p_packet->id, "FRTE", 4 ) )
				{
					throw exception_io_data( "DST chunk is missing FRTE chunk" );
				}

				if ( p_packet->type != iff_stream::iff_packet_base::packet_data )
				{
					throw exception_io_data( "DST FRTE chunk too large" );
				}

				const iff_stream::iff_packet_data & p_frte = reinterpret_cast< const iff_stream::iff_packet_data & > ( *p_packet );

				if ( p_frte.data.get_size() != 6 )
				{
					throw exception_io_data( "DST FRTE chunk wrong size" );
				}

				frame_count = pfc::byteswap_if_le_t( * ( t_uint32 * ) p_frte.data.get_ptr() );
				frame_rate = pfc::byteswap_if_le_t( * ( t_uint16 * ) ( p_frte.data.get_ptr() + 4 ) );

				if ( frame_rate != 75 )
				{
					throw exception_io_data( "Non-standard frame rate" );
				}
			}
		}
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		p_info.info_set( "codec", "DSD" );
		p_info.info_set_int( "original_samplerate", sample_rate );
		p_info.info_set_int( "channels", channels );
		if ( is_dst ) p_info.set_length( (double) frame_count / frame_rate );
		else p_info.set_length( (double) m_file_stream->get_size_ex( p_abort ) / channels * 8 / sample_rate );
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_file->get_stats( p_abort );
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		bits_discard = 0;
		eof = 0;
		first_frame = true;
		m_resamplers.set_size( channels );
		for ( unsigned i = 0; i < channels; i++ ) m_resamplers[ i ].reset();
		if ( !is_dst )
		{
			m_file_stream->reopen( p_abort );
			sample_buffer.set_size( 16384 * channels );
			sample_buffer_out.set_size( 16384 * channels );
		}
		else
		{
			if ( Decoder )
			{
				try
				{
					Close( Decoder );
				}
				catch (std::bad_alloc &)
				{
					throw;
				}
				catch (std::exception & e)
				{
					throw exception_io_data( e.what() );
				}
				delete Decoder;
				Decoder = NULL;
			}

			Decoder = new ebunch;

			try
			{
				Init( Decoder, channels, sample_rate / 44100 );
			}
			catch (std::bad_alloc &)
			{
				throw;
			}
			catch (std::exception & e)
			{
				throw exception_io_data( e.what() );
			}

			unsigned frame_size = sample_rate * channels / frame_rate / 8;
			sample_buffer.set_size( frame_size );
			sample_buffer_out.set_size( frame_size );

			const iff_stream::iff_packet_container & p_container = m_stream.get();
			const iff_stream::iff_packet_base * p_packet;
			unsigned index;

			p_container.find_packet( "DST ", p_packet, index );
			dst_chunks = reinterpret_cast< const iff_stream::iff_packet_container * > ( p_packet );

			if ( ! dst_chunks->find_packet( "DSTF", p_packet, frame_number, 1 ) )
			{
				throw exception_io_data( "No DSTF chunks" );
			}

			bytes_discard = 0;
		}
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if ( eof ) return false;

		unsigned bytes_read;

		if ( !is_dst )
		{
			bytes_read = m_file_stream->read( sample_buffer.get_ptr(), 16384 * channels, p_abort );

			if ( bytes_read < 16384 * channels ) eof = 1;
		}
		else
		{
			const iff_stream::iff_packet_base * p_packet;

			t_uint8 * ptr = sample_buffer.get_ptr();

			bytes_read = 0;

			ULONG frame_size;
			unsigned frame_number;
			if ( dst_chunks->find_packet( "DSTF", p_packet, frame_number, this->frame_number ) )
			{
				if ( p_packet->type != iff_stream::iff_packet_base::packet_file )
				{
					throw exception_io_data( "DSTF chunks parsed incorrectly" );
				}

				const iff_stream::iff_packet_file & p_dst = reinterpret_cast< const iff_stream::iff_packet_file & > ( *p_packet );

				frame_size = p_dst.data->get_size_ex( p_abort );

				dst_buffer.set_size( frame_size );
				p_dst.data->reopen( p_abort );
				p_dst.data->read_object( dst_buffer.get_ptr(), frame_size, p_abort );

				try
				{
					Decode( Decoder, dst_buffer.get_ptr(), ptr, 1, &frame_size );
				}
				catch (std::bad_alloc &)
				{
					throw;
				}
				catch (std::exception & e)
				{
					throw exception_io_data( e.what() );
				}

				ptr += sample_rate * channels / frame_rate / 8;
				bytes_read += sample_rate * channels / frame_rate / 8;

				this->frame_number = frame_number + 1;
			}

			if ( bytes_read && bytes_discard )
			{
				memmove( sample_buffer.get_ptr(), sample_buffer.get_ptr() + bytes_discard, bytes_read - bytes_discard );
				bytes_read -= bytes_discard;
				bytes_discard = 0;
			}

			if ( ! bytes_read ) return false;
		}

		/*
		for ( unsigned i = 0; i < bytes_read; i += channels )
		{
			for ( unsigned j = 0, k = channels; j < k; j++ )
			{
				unsigned char bits = *( sample_buffer.get_ptr() + i + j );
				audio_sample * sample_out = sample_buffer_out.get_ptr() + samples_decoded * channels + channel_offsets[ j ];
				for ( unsigned l = bits_discard; l < 8; l++ )
				{
					*sample_out = ( ( bits << l ) & 0x80 ) ? 1.0f : -1.0f;
					sample_out += channels;
				}
			}
			samples_decoded += 8 - bits_discard;
			bits_discard = 0;
		}
		*/
		for ( unsigned i = 0, j = channels; i < j; i++ )
		{
			m_resamplers[ i ].process( sample_buffer.get_ptr() + i, 0, channels, sample_buffer_out.get_ptr() + i, 0, channels, bytes_read / channels );
		}

		unsigned target_rate = sample_rate >> 3;

		audio_chunk_temp_impl_mod m_chunk( sample_buffer_out.get_ptr(), bytes_read / channels, target_rate, channels, channel_mask );

		if (m_resampler.is_empty() && target_rate > max_sample_rate)
		{
			if ( !resampler_entry::g_create( m_resampler, target_rate, max_sample_rate, 0 ) ) throw exception_io_data( "Unsupported sample rate, no resampler present" );
		}

		if ( target_rate > max_sample_rate )
		{
			if ( m_resampler.is_valid() )
			{
				dsp_chunk_list_impl chunks; chunks.add_chunk( &m_chunk );
				m_resampler->run_abortable( &chunks, NULL, eof ? dsp::END_OF_TRACK : 0, p_abort );
				unsigned sample_count = 0;
				for ( unsigned i = 0, j = chunks.get_count(); i < j; i++ )
				{
					sample_count += chunks.get_item( i )->get_data_size();
				}
				if ( sample_count )
				{
					p_chunk.set_data_size( sample_count );
					audio_sample * sample_out = p_chunk.get_data();
					for ( unsigned i = 0, j = chunks.get_count(); i < j; i++ )
					{
						audio_chunk * chunk = chunks.get_item( i );
						memcpy( sample_out, chunk->get_data(), chunk->get_data_size() * sizeof( audio_sample ) );
						sample_out += chunk->get_data_size();
					}
				}
				else
				{
					sample_count = channels;
					p_chunk.set_data_size( sample_count );
					memset( p_chunk.get_data(), 0, sample_count * sizeof( audio_sample ) );
				}
				p_chunk.set_srate( max_sample_rate );
				p_chunk.set_channels( channels, channel_mask );
				p_chunk.set_sample_count( sample_count / channels );
			}
		}
		else
		{
			p_chunk.copy( m_chunk );
		}

		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		eof = 0;
		first_frame = true;
		t_filesize offset = audio_math::time_to_samples( p_seconds, sample_rate );
		if ( !is_dst )
		{
			m_file_stream->seek( offset / 8 * channels, p_abort );
			bits_discard = offset & 7;
		}
		else
		{
			unsigned samples_per_frame = sample_rate / frame_rate;
			unsigned frame_skip = offset / samples_per_frame;
			unsigned samples_skip = offset % samples_per_frame;
			bits_discard = samples_skip & 7;
			bytes_discard = samples_skip / 8;
			unsigned frame_number;
			const iff_stream::iff_packet_base * p_packet;
			if ( dst_chunks->find_packet( "DSTF", p_packet, this->frame_number, 1 ) )
			{
				while ( frame_skip )
				{
					if ( dst_chunks->find_packet( "DSTF", p_packet, frame_number, this->frame_number + 1 ) )
					{
						frame_skip--;
						this->frame_number = frame_number;
					}
					else break;
				}
			}
			else
			{
				throw exception_io_data( "No DSTF chunks" );
			}
		}

		if ( m_resampler.is_valid() ) m_resampler->flush();
		for ( unsigned i = 0; i < channels; i++ ) m_resamplers[ i ].reset();
	}

	bool decode_can_seek()
	{
		return m_file.is_valid() ? m_file->can_seek() : false;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		if (first_frame)
		{
			unsigned target_rate = sample_rate >> 3;
			if ( target_rate > max_sample_rate ) target_rate = max_sample_rate;

			p_out.info_set_int( "samplerate", target_rate );
			first_frame = false;
			
			return true;
		}

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
		return !stricmp( p_extension, "dff" );
	}
};

static cfg_dropdown_history cfg_history_rate(guid_cfg_history_rate,16);

static const int srate_tab[]={8000,11025,16000,22050,24000,32000,44100,48000,64000,88200,96000,176400,192000};

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_CONFIG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_EDITCHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSelectionChange)
		DROPDOWN_HISTORY_HANDLER(IDC_SAMPLERATE, cfg_history_rate)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	void OnSelectionChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	void enable_vgm_loop_count(BOOL);

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	CWindow w;
	char temp[16];
	int n;
	for(n=tabsize(srate_tab);n--;)
	{
		if (srate_tab[n] != cfg_max_sample_rate)
		{
			itoa(srate_tab[n], temp, 10);
			cfg_history_rate.add_item(temp);
		}
	}
	itoa(cfg_max_sample_rate, temp, 10);
	cfg_history_rate.add_item(temp);
	w = GetDlgItem( IDC_SAMPLERATE );
	cfg_history_rate.setup_dropdown( w );
	::SendMessage( w, CB_SETCURSEL, 0, 0 );
	
	return TRUE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}

void CMyPreferences::OnSelectionChange(UINT, int, CWindow) {
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	SetDlgItemInt( IDC_SAMPLERATE, default_cfg_max_sample_rate );
	
	OnChanged();
}

void CMyPreferences::apply() {
	char temp[16];
	int t = GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE );
	if ( t < 6000 ) t = 6000;
	else if ( t > 192000 ) t = 192000;
	SetDlgItemInt( IDC_SAMPLERATE, t, FALSE );
	itoa( t, temp, 10 );
	cfg_history_rate.add_item( temp );
	cfg_max_sample_rate = t;
	
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	return GetDlgItemInt( IDC_SAMPLERATE, NULL, FALSE ) != cfg_max_sample_rate;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "DSDIFF Decoder";}
	GUID get_guid() {
		// {1A6A112A-DA07-46c8-A4FF-0FB7469D42E2}
		static const GUID guid = { 0x1a6a112a, 0xda07, 0x46c8, { 0xa4, 0xff, 0xf, 0xb7, 0x46, 0x9d, 0x42, 0xe2 } };
		return guid;
	}
	GUID get_parent_guid() {return guid_input;}
};

static input_singletrack_factory_t< input_dsdiff >          g_input_factory_dsdiff;
static preferences_page_factory_t <preferences_page_myimpl> g_config_dsdiff_factory;

DECLARE_FILE_TYPE("DSDIFF Files", "*.dff");

DECLARE_COMPONENT_VERSION("DSDIFF Decoder", MYVERSION, "Decodes DSDIFF streams.");

VALIDATE_COMPONENT_FILENAME("foo_input_dsdiff.dll");
