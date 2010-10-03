//! \since 1.1
//! This class allows postprocessing of the output of decoders, mainly intended to postprocess PCM data extracted from a file, to decode DTS/HDCD/etc. \n
//! Instead of calling this directly, you probably want to use input_postprocessed template.
class decode_postprocessor_instance : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE(decode_postprocessor_instance, service_base);
public:
	enum {
		//! End of stream. Flush any buffered data during this call.
		flag_eof = 1 << 0,
		//! Stream has already been altered by another instance.
		flag_altered = 1 << 1,
	};
	//! @returns True if the chunk list has been altered by the call, false if not - to tell possible other running instances whether the stream has already been altered or not.
	virtual bool run(dsp_chunk_list & p_chunk_list,t_uint32 p_flags,abort_callback & p_abort) = 0;
	virtual bool get_dynamic_info(file_info & p_out) = 0;
	virtual void flush() = 0;
	virtual double get_buffer_ahead() = 0;
};

//! \since 1.1
//! Entrypoint class for instantiating decode_postprocessor_instance. \n
//! Instead of calling this directly, you probably want to use input_postprocessed template.
class decode_postprocessor_entry : public service_base {
	FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(decode_postprocessor_entry)
public:
	virtual bool instantiate(const file_info & info, decode_postprocessor_instance::ptr & out) = 0;
};


FOOGUIDDECL const GUID decode_postprocessor_entry::class_guid = { 0xb105c345, 0xf642, 0x4a26, { 0xa7, 0x80, 0xbb, 0x90, 0xe1, 0xb4, 0xac, 0xb1 } };
FOOGUIDDECL const GUID decode_postprocessor_instance::class_guid = { 0xfacabec0, 0xeeee, 0x46d1, { 0xa5, 0x39, 0xaa, 0x57, 0x5, 0xaf, 0x5d, 0x45 } };