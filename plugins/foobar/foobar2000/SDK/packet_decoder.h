//! Provides interface to decode various audio data types to PCM. Use packet_decoder_factory_t template to register.

class packet_decoder : public service_base
{
protected:
	//! Prototype of function that must be implemented by packet_decoder implementation but is not accessible through packet_decoder interface itself.
	//! Determines whether specific packet_decoder implementation supports specified decoder setup data.
	static bool g_is_our_setup(const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size) {return false;}

	//! Prototype of function that must be implemented by packet_decoder implementation but is not accessible through packet_decoder interface itself.
	//! Initializes packet decoder instance with specified decoder setup data. This is called only once, before any other methods.
	//! @param p_decode If set to true, decode() and reset_after_seek() calls can be expected later. If set to false, those methods will not be called on this packet_decoder instance - for an example when caller is only retrieving information about the file rather than preparing to decode it.
	t_io_result open(const GUID & p_owner,bool p_decode,unsigned p_param1,const void * p_param2,unsigned p_param2size,abort_callback & p_abort) {return io_result_error_data;}
public:


	//! Forwards additional information about stream being decoded. \n
	//! Calling: this must be called immediately after packet_decoder object is created, before any other methods are called.\n
	//! Implementation: this is called after open() (which is called by implementation framework immediately after creation), and before any other methods are called.
	virtual unsigned set_stream_property(const GUID & p_type,unsigned p_param1,const void * p_param2,unsigned p_param2size) = 0;

	
	//! Retrieves additional usre-readable tech infos that decoder can provide.
	virtual t_io_result get_info(file_info & p_info) = 0;

	//! Returns many frames back to start decoding when seeking.
	virtual unsigned get_max_frame_dependency()=0;
	//! Returns much time back to start decoding when seeking (for containers where going back by specified number of frames is not trivial).
	virtual double get_max_frame_dependency_time()=0;

	//! Flushes decoder after seeking.
	virtual t_io_result reset_after_seek()=0;

	//! Decodes a block of audio data.\n
	//! It may return empty chunk even when successful (caused by encoder+decoder delay for an example), caller must check for it and handle it appropriately.
	virtual t_io_result decode(const void * p_buffer,unsigned p_bytes,audio_chunk & p_chunk,abort_callback & p_abort)=0;

	//! Returns whether this packet decoder supports analyze_first_frame() function.
	virtual bool analyze_first_frame_supported() = 0;  
	//! Optional. Some codecs need to analyze first frame of the stream to return additional info about the stream, such as encoding setup. This can be only called immediately after instantiation (and set_stream_property() if present), before any actual decoding or get_info(). Caller can determine whether this method is supported or not by calling analyze_first_frame_supported(), to avoid reading first frame when decoder won't utiilize the extra info for an example. If particular decoder can't utilize first frame info in any way (and analyze_first_frame_supported() returns false), this function should do nothing and succeed.
	virtual t_io_result analyze_first_frame(const void * p_buffer,unsigned p_bytes,abort_callback & p_abort) = 0;
	
	//! Static helper, creates a packet_decoder instance and initializes it with specific decoder setup data.
	static t_io_result g_open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size,abort_callback & p_abort);

	static const GUID owner_MP4,owner_matroska,owner_MP3,owner_MP2,owner_MP1,owner_MP4_ALAC,owner_ADTS,owner_ADIF;

	struct matroska_setup
	{
		const char * codec_id;
		unsigned sample_rate,sample_rate_output;
		unsigned channels;
		unsigned codec_private_size;
		const void * codec_private;
	};
	//owner_MP4: param1 - codec ID (MP4 audio type), param2 - MP4 codec initialization data
	//owner_MP3: raw MP3/MP2 file, parameters ignored
	//owner_matroska: param2 = matroska_setup struct, param2size size must be equal to sizeof(matroska_setup)


	//these are used to initialize PCM decoder
	static const GUID property_samplerate,property_bitspersample,property_channels,property_byteorder,property_signed,property_channelmask;
	//property_samplerate : param1 == sample rate in hz
	//property_bitspersample : param1 == bits per sample
	//property_channels : param1 == channel count
	//property_byteorder : if (param1) little_endian; else big_endian;
	//property_signed : if (param1) signed; else unsigned;


	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

protected:
	packet_decoder() {}
	~packet_decoder() {}
};

class packet_decoder_streamparse : public packet_decoder
{
public:
	virtual t_io_result decode_ex(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_processed,audio_chunk & p_chunk,abort_callback & p_abort) = 0;
	virtual t_io_result analyze_first_frame_ex(const void * p_buffer,unsigned p_bytes,unsigned & p_bytes_processed,abort_callback & p_abort) = 0;

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return packet_decoder::service_query(guid);
	}

protected:
	packet_decoder_streamparse() {}
	~packet_decoder_streamparse() {}
};

class packet_decoder_entry : public service_base
{
public:
	virtual bool is_our_setup(const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size) = 0;
	virtual t_io_result open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size,abort_callback & p_abort) = 0;
	
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
	
	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}
};


template<class T>
class packet_decoder_entry_impl_t : public packet_decoder_entry
{
public:
	bool is_our_setup(const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size)
	{
		return T::g_is_our_setup(p_owner,p_param1,p_param2,p_param2size);
	}
	t_io_result open(service_ptr_t<packet_decoder> & p_out,bool p_decode,const GUID & p_owner,unsigned p_param1,const void * p_param2,unsigned p_param2size,abort_callback & p_abort)
	{
		assert(is_our_setup(p_owner,p_param1,p_param2,p_param2size));
		service_ptr_t<T> instance = new service_impl_t<T>();
		if (instance.is_empty()) return io_result_error_out_of_memory;
		t_io_result status;
		status = instance->open(p_owner,p_decode,p_param1,p_param2,p_param2size,p_abort);
		if (io_result_failed(status)) return status;
		p_out = instance.get_ptr();
		return io_result_success;
	}
};

template<class T>
class packet_decoder_factory_t : public service_factory_single_t<packet_decoder_entry,packet_decoder_entry_impl_t<T> > {};