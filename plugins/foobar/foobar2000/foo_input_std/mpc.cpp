#include "stdafx.h"

#include "mpc/in_mpc.h"
#include "mpc/mpc_dec.h"

#include <math.h>

#include <stdio.h>


class mpc_reader_impl : public MPC_Reader
{
public:
	mpc_reader_impl(const service_ptr_t<file> & p_file,abort_callback & p_abort) : m_wrapper(p_file,p_abort) {}

	inline t_io_result get_status() const {return m_wrapper.get_status();}

	unsigned read ( void *ptr, unsigned size )
	{
		return m_wrapper.read(ptr,size);
	}
	bool seek ( unsigned offset )
	{
		return m_wrapper.seek(offset);
	}
	unsigned tell()
	{
		return (unsigned)m_wrapper.get_position();
	}
	unsigned get_size()
	{
		return (unsigned)m_wrapper.get_size();
	}
	bool can_seek()
	{
		return m_wrapper.can_seek();
	}
private:
	file_wrapper_simple m_wrapper;
};

static void write_replaygain_e ( const service_ptr_t<file> & r, const file_info & info, t_int64 headerpos, abort_callback & p_abort )
{

	float TrackGain=0, AlbumGain=0, TrackPeak=0, AlbumPeak=0;

	replaygain_info rg = info.get_replaygain();
	if (rg.is_album_gain_present()) AlbumGain = rg.m_album_gain;
	if (rg.is_album_peak_present()) AlbumPeak = rg.m_album_peak;
	if (rg.is_track_gain_present()) TrackGain = rg.m_track_gain;
	if (rg.is_track_peak_present()) TrackPeak = rg.m_track_peak;

	t_int64 pos = r->get_position_e (p_abort);


	r->seek_e (headerpos, p_abort);

	unsigned char buff[20];
	r->read_object_e (buff, 20, p_abort);

	if ( memcmp (buff, "MP+", 3) || (buff[3] & 15) != 7 ) {
		throw io_result_error_data;
	}

	int val;
	// Title Peak level
	val       = (int)(TrackPeak * 32768.0 + 0.5);
	buff [12] = (unsigned char)(val >> 0);
	buff [13] = (unsigned char)(val >> 8);
	val       = (int)(TrackPeak * 32768.0 / 1.18 + 0.5);
	buff [ 8] = (unsigned char)(val >> 0);
	buff [ 9] = (unsigned char)(val >> 8);
	// Album Peak level
	val       = (int)(AlbumPeak * 32768.0 + 0.5);
	buff [16] = (unsigned char)(val >> 0);
	buff [17] = (unsigned char)(val >> 8);
	// Title RMS
	val       = (int)(floor (TrackGain * 100.0 + 0.5));
	buff [14] = (unsigned char)(val >> 0);
	buff [15] = (unsigned char)(val >> 8);
	// Album RMS
	val       = (int)(floor (AlbumGain * 100.0 + 0.5));
	buff [18] = (unsigned char)(val >> 0);
	buff [19] = (unsigned char)(val >> 8);

	r->seek_e (headerpos, p_abort);
	r->write_object_e ( buff, 20, p_abort);
	r->seek_e ( pos, p_abort);
}

class input_mpc : public input {
public:

	t_io_result set_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		if (!p_reader->can_seek()) return io_result_error_generic;
		m_reader = p_reader;

		t_io_result status;
		t_uint64 dummy;
		status = tag_processor_id3v2::g_remove(m_reader,dummy,p_abort);
		if (io_result_failed( status ) ) return status;

		{
			file_info_impl blah;
			status = read_stream_info(p_location,blah,false,p_abort);
			if (io_result_failed(status)) return status;
		}


        t_int64 headerpos = 0;
        headerpos = m_info.simple.HeaderPosition;

		tag_processor::remove_id3v2(m_reader,p_abort);
		if (p_abort.is_aborting()) return io_result_aborted;

		try {
			write_replaygain_e(m_reader, p_info, headerpos, p_abort);
		} catch(t_io_result code) {return code;}

		p_info.info_set("tagtype","apev2");

		{
			
			file_info_impl l_info;
			l_info.copy(p_info);
			l_info.reset_replaygain();
			return tag_processor::write_apev2(m_reader,l_info,p_abort);
			
		}
	}
public:

	static bool g_is_our_content_type(const char * url,const char * type)
	{
		return !strcmp(type,"audio/musepack") || !strcmp(type,"audio/x-musepack");
	}

	static bool g_test_filename(const char * fn,const char * ext) 
	{
		return !stricmp(ext, "mpc" ) || !stricmp(ext, "mp+") || !stricmp(ext, "mpp");
	}

	input_mpc()
	{
		seek_destination = -1;
	}
	~input_mpc()
	{
	}

	t_io_result get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		m_reader = p_reader;
		t_io_result status;
		status = read_stream_info (p_location,p_info, true, p_abort );
		if (io_result_failed(status)) return status;

		return io_result_success;
	}

	t_io_result open(const service_ptr_t<file> & r,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		m_reader = r;
		t_io_result status;
		status = read_stream_info (p_location,p_info, !!(p_flags & OPEN_FLAG_GET_INFO), p_abort );
		if (io_result_failed(status)) return status;

		m_vbr_handler.reset();

		return init_decode (p_info, p_abort);
	}


	virtual t_io_result run(audio_chunk * chunk, abort_callback & p_abort)
	{
		if (seek_destination>=0)
		{
			if (seek_destination>=m_info.GetLength()) return io_result_eof;
			
			{
				mpc_reader_impl reader(m_reader,p_abort);
				int ret = m_decoder->seek(reader,seek_destination);
				if (io_result_failed(reader.get_status())) return reader.get_status();
				if (!ret) return io_result_error_data;
			}
			seek_destination = -1;
			m_vbr_handler.reset();
		}

        int valid_samples;

		{
			mpc_reader_impl reader(m_reader,p_abort);
			unsigned vbr_update_acc = 0,vbr_update_bits = 0;
			valid_samples = m_decoder->decode ( reader, sample_buffer, &vbr_update_acc, &vbr_update_bits);
			if (io_result_failed(reader.get_status())) return reader.get_status();

			m_vbr_handler.on_frame((double)valid_samples / (double) m_info.simple.SampleFreq ,vbr_update_bits);
		}

			
		if (valid_samples<0) return io_result_error_data;
		else if (valid_samples==0) return io_result_eof;
		else
		{
#ifdef MPC_FIXED_POINT
			if (!chunk->set_data_fixedpoint ( sample_buffer, valid_samples * 2 * sizeof(sample_buffer[0]), (int)m_info.simple.SampleFreq, 2, sizeof(sample_buffer[0]) * 8 )) return -1;
			dsp_util::scale_chunk(chunk,pow(2,32 - (MPC_FIXED_POINT_FRACTPART + MPC_FIXED_POINT_SHIFT)));
			return io_result_success;
#else
			return chunk->set_data_32 ( (float *)sample_buffer, valid_samples, 2, (int)m_info.simple.SampleFreq ) ? io_result_success : io_result_error_generic;
#endif
		}
	}

	virtual t_io_result seek(double s, abort_callback & p_abort)
	{
		if (s<0) return io_result_error_generic;
		seek_destination = s;
		return io_result_success;
	}

	virtual bool get_dynamic_info(file_info & p_out,double * p_timestamp_delta,bool * p_track_change)
	{
		return m_vbr_handler.on_update(p_out,p_timestamp_delta);
	}

	static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {63D1AB93-1587-45d0-BAD7-8862E081257A}
		static const GUID guid = 
		{ 0x63d1ab93, 0x1587, 0x45d0, { 0xba, 0xd7, 0x88, 0x62, 0xe0, 0x81, 0x25, 0x7a } };
		return guid;
	}

	static const char * g_get_name() {return "Musepack decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

private:
	pfc::autoptr_t<MPC_decoder> m_decoder;
	service_ptr_t<file> m_reader;
	StreamInfo m_info;
	int skip_samples;
	dynamic_bitrate_helper m_vbr_handler;
	double seek_destination;

	MPC_SAMPLE_FORMAT sample_buffer [FRAMELEN * 2 * 2];

//  void updateStatusInfo ( MediaInfo *infos, int bitrate = -1 );

 
	t_io_result read_stream_info ( const playable_location & p_location, file_info & info, bool want_tags, abort_callback & p_abort);
	t_io_result init_decode ( file_info & info , abort_callback & p_abort);
};



t_io_result input_mpc::init_decode ( file_info & info, abort_callback & p_abort)
{ //infos done, setup decoder
	m_decoder = new MPC_decoder();
	if (m_decoder.is_empty()) return io_result_error_out_of_memory;

	m_decoder->SetStreamInfo ( &m_info );

	int ret;
	mpc_reader_impl reader(m_reader,p_abort);
	ret = m_decoder->FileInit (reader);
	if (io_result_failed(reader.get_status())) return reader.get_status();

	if ( !ret ) {
		m_decoder.release();
		return io_result_error_data;
	}

	return io_result_success;
}

t_io_result input_mpc::read_stream_info(const playable_location & p_location,file_info & info, bool want_tags, abort_callback & p_abort) { //read infos
	m_info.Clear ();

	if ( m_info.ReadStreamInfo ( mpc_reader_impl(m_reader,p_abort) ) != ERROR_CODE_OK ) {
		return io_result_error_data;//fixme error codes
	}

	info.set_length( m_info.GetLength() );

	if (want_tags)
	{
		info.info_set_int("bitrate",(t_int64)(m_info.simple.AverageBitrate/1000.0+0.5));
		info.info_set_int("samplerate",(t_int64)m_info.simple.SampleFreq);
		info.info_set_int("channels",2);
		if (m_info.simple.ProfileName)
			info.info_set("mpc_profile",m_info.simple.ProfileName);

		if (m_info.simple.Encoder[0])
			info.info_set("tool",m_info.simple.Encoder);

		{
			char temp[4];
			char * ptr = temp;
			*(ptr++) =  (m_info.simple.StreamVersion & 0xF) + '0';
			if (m_info.simple.StreamVersion&0xF0)
			{
				*(ptr++) = '.';
				*(ptr++) = ((m_info.simple.StreamVersion & 0xF0) >> 4) + '0';
			}
			*ptr = 0;
			info.info_set("mpc_streamversion",temp);
		}

        if ( m_info.simple.IsTrueGapless ) info.info_set("mpc_accurate_length", "yes");

		info.info_set("codec","Musepack");

		if (m_info.simple.GainTitle || m_info.simple.PeakTitle) {
			info.info_set_replaygain_track_gain((float)((float)m_info.simple.GainTitle * 0.01));
			if (m_info.simple.PeakTitle) info.info_set_replaygain_track_peak((float)((float)m_info.simple.PeakTitle / 32768.0));
		}

		if (m_info.simple.GainAlbum || m_info.simple.PeakAlbum) {
			info.info_set_replaygain_album_gain((float)((float)m_info.simple.GainAlbum * 0.01));
			if (m_info.simple.PeakAlbum) info.info_set_replaygain_album_peak((float)((float)m_info.simple.PeakAlbum / 32768.0));
		}


		if (m_reader->can_seek())
		{
			t_filesize position_old;
			
			t_io_result status = m_reader->get_position(position_old,p_abort);
			if (io_result_failed(status)) return status;

			status = tag_processor::read_id3v2_trailing(m_reader,info,p_abort);
			if (status != io_result_error_not_found && status != io_result_error_data && io_result_failed(status)) return status;

			status = m_reader->seek(position_old,p_abort);
			if (io_result_failed(status)) return status;
		}
	}

	return io_result_success;
}

static input_factory_t<input_mpc> g_input_mpc_factory;

DECLARE_FILE_TYPE("Musepack","*.MPC;*.MP+;*.MPP");