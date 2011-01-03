#include "stdafx.h"
#include "../../MAC_SDK/Source/Shared/All.h"
#include "../../MAC_SDK/Source/Shared/IO.h"
#include "../../MAC_SDK/Source/MACLib/APEInfo.h"
//#include <Shared/All.h>
//#include <Shared/IO.h>
//#include <MACLib/APEInfo.h>



// -------------------------------------

static struct {
    int code;
    const char *msg;
} error_codes[] = { ERROR_EXPLANATION };

class input_monkey : public input, private CIO
{
private:
    mem_block_t<char> buffer;
	pfc::autoptr_t<IAPEDecompress> m_APEDecompress;
    service_ptr_t<file> m_reader;
    
    double dur;
	double seek_to;

	t_io_result m_io_status;

	abort_callback * m_abort;

//CIO stuff
private:
    virtual int Open(const wchar_t * pName) {return 0;}
    virtual int Close() {return 0;}

    virtual int Read(void * pBuffer, unsigned int nBytesToRead, unsigned int * pBytesRead)
    {
		assert(m_abort);
		if (io_result_failed(m_io_status)) return ERROR_IO_READ;
        unsigned io_bytes_done;
		m_io_status = m_reader->read(pBuffer,nBytesToRead,io_bytes_done,*m_abort);
		if (io_result_failed(m_io_status)) return ERROR_IO_READ;
        if (pBytesRead) *pBytesRead=io_bytes_done;
        return ERROR_SUCCESS;
    }
    virtual int Write(const void * pBuffer, unsigned int nBytesToWrite, unsigned int * pBytesWritten)
    {
        if (pBytesWritten) *pBytesWritten=0;
        return ERROR_IO_WRITE;
    }

    virtual int Seek(int nDistance, unsigned int nMoveMode)
    {
		assert(m_abort);
		if (io_result_failed(m_io_status)) return 1;
		m_io_status = m_reader->seek2(nDistance,nMoveMode,*m_abort);
		if (io_result_failed(m_io_status)) return 1;
        return 0;
    }

    virtual int Create(const wchar_t * pName) {return 0;}
    virtual int Delete() {return 0;}

    virtual int SetEOF() {return 1;}

    // attributes
    virtual int GetPosition()
	{
		t_filesize ret;
		if (io_result_failed(m_io_status)) return -1;
		m_io_status = m_reader->get_position(ret,*m_abort);
		if (io_result_failed(m_io_status)) return -1;
		if (ret == filesize_invalid) return -1;
		return (int)ret;
	}
    virtual int GetSize()
	{
		t_filesize ret;
		if (io_result_failed(m_io_status)) return -1;
		m_io_status = m_reader->get_size(ret,*m_abort);
		if (io_result_failed(m_io_status)) return -1;
		if (ret == filesize_invalid) return -1;
		return (int)ret;
	}
    virtual int GetName(wchar_t * pBuffer)
    {
        if (pBuffer) *pBuffer=0;
        return 1;
    }

    enum {DELTA_SAMPLES=1024};

    const char *get_error(int error)
    {
        for (int i=0; i<tabsize(error_codes); i++) {
            if (error == error_codes[i].code) return error_codes[i].msg;
        }

        return 0;
    }

public:

    inline static bool g_test_filename(const char * fn,const char * ext) 
    {
        return !stricmp_utf8(ext,"APE") || !stricmp_utf8(ext,"MAC");
    }

    input_monkey() : m_io_status(io_result_success)
    {
		seek_to = -1;
		m_abort = 0;
    }

    ~input_monkey()
    {
    }

	t_io_result get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,OPEN_FLAG_GET_INFO,false);
	}
	
	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,p_flags,true);
	}


	t_io_result open_internal(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags,bool p_decode)
    {
		vartoggle_t<abort_callback*> l_aborttoggle(m_abort,&p_abort);

		t_io_result status;

        m_reader = p_reader;

        m_APEDecompress = CreateIAPEDecompressEx(this);
		if (io_result_failed(m_io_status)) return m_io_status;
        if (m_APEDecompress.is_empty()) return io_result_error_data;

        dur = (double)m_APEDecompress->GetInfo(APE_INFO_TOTAL_BLOCKS) / (double)m_APEDecompress->GetInfo(APE_INFO_SAMPLE_RATE);


        if (p_flags & OPEN_FLAG_GET_INFO) {
            p_info.set_length(dur);
			t_filesize length;
			if (io_result_failed(status = m_reader->get_size(length,p_abort))) return status;

            if (m_reader->can_seek()) p_info.info_set_int("bitrate", (__int64)((double)length * 8.0 / dur / 1000.0 + 0.5) );
            p_info.info_set_int("samplerate",m_APEDecompress->GetInfo(APE_INFO_SAMPLE_RATE));
            p_info.info_set_int("channels",m_APEDecompress->GetInfo(APE_INFO_CHANNELS));
		
            p_info.info_set_int("bitspersample",m_APEDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE));
            p_info.info_set_int("flags", m_APEDecompress->GetInfo(APE_INFO_FORMAT_FLAGS));
            p_info.info_set("codec","Monkey\'s Audio");

            {
                int compression = m_APEDecompress->GetInfo(APE_INFO_COMPRESSION_LEVEL);
                const char * msg = 0;
                if (compression == COMPRESSION_LEVEL_FAST) msg = "Fast";
                else if (compression == COMPRESSION_LEVEL_NORMAL) msg = "Normal";
                else if (compression == COMPRESSION_LEVEL_HIGH) msg = "High";
                else if (compression == COMPRESSION_LEVEL_EXTRA_HIGH) msg = "Extra High";
                else if (compression == COMPRESSION_LEVEL_INSANE) msg = "Insane";
                else msg = "Unknown";
                p_info.info_set("compression",msg);
            }

            {
                int version = m_APEDecompress->GetInfo(APE_INFO_FILE_VERSION);
                char tmp[32];
                if (version%10 == 0)
                    sprintf(tmp, "%i.%i", version/1000, (version/10)%100);
                else
                    sprintf(tmp, "%i.%i", version/1000, version%1000);
                p_info.info_set("version",tmp);

                if (version >= 3980) {
                    APE_FILE_INFO *apeinfo = (APE_FILE_INFO *)m_APEDecompress->GetInfo(APE_INTERNAL_INFO);
                    if (apeinfo && apeinfo->spAPEDescriptor) {
                        char temp[64];
                        const unsigned char *s = (const unsigned char *)apeinfo->spAPEDescriptor->cFileMD5;
                        char *d = (char *)temp;
                        for (int i=0; i<16; i++) {
                            int c = *s++;
                            d += sprintf ( d, "%x", c );
                        }
                        p_info.info_set("md5 hash",temp);
                    }
                }
            }

			t_uint64 offset_backup;
			status = m_reader->get_position(offset_backup,p_abort);
			if (io_result_failed(status)) return status;

			status = tag_processor::read_trailing(m_reader,p_info,p_abort);
			if (status != io_result_error_not_found && status != io_result_error_data && io_result_failed(status)) return status;

			status = m_reader->seek(offset_backup,p_abort);
			if (io_result_failed(status)) return status;
        }

        return io_result_success;
    }

    t_io_result set_info(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & info,abort_callback & p_abort)
    {
		return tag_processor::write_apev2(p_file,info,p_abort);
    }

    virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
    {
		vartoggle_t<abort_callback*> l_aborttoggle(m_abort,&p_abort);

		if (seek_to>=0)
		{
			if (seek_to >= dur) {
				return io_result_eof;
			}
			double seconds = seek_to;
			seek_to = -1;
			int status = m_APEDecompress->Seek((int)(seconds * (double)m_APEDecompress->GetInfo(APE_INFO_SAMPLE_RATE)));
			if (io_result_failed(m_io_status)) return m_io_status;
			if (status != 0) return io_result_error_data;
		}

        int nBlocksRetrieved = 0;
        char * ptr = buffer.set_size(m_APEDecompress->GetInfo(APE_INFO_BLOCK_ALIGN) * DELTA_SAMPLES);
		if (ptr == 0) return io_result_error_out_of_memory;
        int nRetVal = m_APEDecompress->GetData(ptr, DELTA_SAMPLES, &nBlocksRetrieved);
		if (io_result_failed(m_io_status)) return m_io_status;
		if (nRetVal == ERROR_END_OF_MEDIA)
			return io_result_eof;

        if (nRetVal != ERROR_SUCCESS) {
            const char *error = get_error(nRetVal);
            if (error)
				console::info(error);
			return io_result_error_data;
        }
        
		if (nBlocksRetrieved<=0) return io_result_eof;

		unsigned out_size = nBlocksRetrieved * m_APEDecompress->GetInfo(APE_INFO_BLOCK_ALIGN),
			srate = m_APEDecompress->GetInfo(APE_INFO_SAMPLE_RATE),
			nch = m_APEDecompress->GetInfo(APE_INFO_CHANNELS),
			bps = m_APEDecompress->GetInfo(APE_INFO_BITS_PER_SAMPLE);

		if (!chunk->set_data_fixedpoint(
			ptr,
			out_size,
			srate,
			nch,
			bps,
			audio_chunk::g_guess_channel_config(nch)
		)) return io_result_error_out_of_memory;

        return io_result_success;
    }

    virtual t_io_result seek(double seconds,abort_callback & p_abort)
    {
		seek_to = seconds;
		return io_result_success;
    }

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}
	inline static bool g_needs_reader() {return true;}

	static GUID g_get_guid()
	{
		// {A34D1C6E-B43E-4e5e-A297-4A635BF8D87B}
		static const GUID guid = 
		{ 0xa34d1c6e, 0xb43e, 0x4e5e, { 0xa2, 0x97, 0x4a, 0x63, 0x5b, 0xf8, 0xd8, 0x7b } };
		return guid;
	}

	static const char * g_get_name() {return "Monkey\'s Audio decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}
};


static input_factory_t<cue_parser::input_wrapper_cue_t<input_monkey> > foo_input_monkey_factory;

static service_factory_single_t<track_indexer,cue_parser::track_indexer_impl_t<input_monkey> > g_track_indexer_factory;

DECLARE_COMPONENT_VERSION("Monkey's Audio decoder", "1.2", "Uses MAClib version " MAC_VERSION_STRING);

DECLARE_FILE_TYPE("Monkey\'s Audio","*.APE");
