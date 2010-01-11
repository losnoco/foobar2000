#include "../SDK/foobar2000.h"

#include "avcodec.h"

class input_wma : public input
{
private:
	AVCodecContext * c;
	AVFormatContext * ic;

	int stream_index;

/*    IWMSyncReader *m_pReader;
    reader * m_fReader;
    CROStream *m_pStream;
    WORD m_wAudioStreamNum;
    bool m_bSeekable;
	bool m_bCoInitialized;
	BYTE *m_pSample;*/

	mem_block_t<float> sample_buffer;

    /*HRESULT _getMetaData(file_info * info);
    HRESULT _getAudioStream();
    void _parseMeta(WMT_ATTR_DATATYPE,WCHAR*,WORD,BYTE*,DWORD,file_info *);
    WCHAR *_MetaMorph(WCHAR *pwszName);
    void _MetaType(WCHAR *pwszName);*/
public:
    
	input_wma();
	~input_wma();
	static inline bool g_test_filename(const char * fn,const char * ext) { return (!stricmp(ext, "WMA") || !stricmp(ext, "ASF") || !stricmp(ext, "WMV")) }

	t_io_result get_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO));
	}

private:
	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info);

	virtual t_io_result set_info(const service_ptr_t<file> & p_reader,const playable_location & p_location,file_info & p_info,abort_callback & p_abort);
	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort);
	virtual t_io_result seek(double seconds,abort_callback & p_abort);
	bool can_seek()
	{
		return false;
	    //return m_bSeekable;
	}

	inline static bool g_is_our_content_type(const char*,const char*) {return false;}
	inline static bool g_needs_reader() {return false;}

	static GUID g_get_guid()
	{
		// {A2947A69-7CB5-47c2-9A20-523B855182B0}
		static const GUID guid = 
		{ 0xa2947a69, 0x7cb5, 0x47c2, { 0x9a, 0x20, 0x52, 0x3b, 0x85, 0x51, 0x82, 0xb0 } };
		return guid;
	}

	static const char * g_get_name() {return "ffmpeg WMA decoder";}
};

static service_factory_t<input,input_wma> foo;
