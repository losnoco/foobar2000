#ifndef _CVT_FLOAT_TO_LINEAR_H_
#define _CVT_FLOAT_TO_LINEAR_H_

//! This class handles conversion of audio data (audio_chunk) to various linear PCM types, with optional dithering.

class NOVTABLE audio_postprocessor : public service_base
{
public:
	//! Processes one chunk of audio data.
	//! @param p_chunk Chunk of audio data to process.
	//! @param p_output Receives output linear signed PCM data.
	//! @param p_out_bps Desired bit depth of output.
	//! @param p_out_bps_physical Desired physical word width of output. Must be either 8, 16, 24 or 32, greater or equal to p_out_bps. This is typically set to same value as p_out_bps.
	//! @param p_dither Indicates whether dithering should be used. Note that dithering is CPU-heavy.
	//! @param p_prescale Value to scale all audio samples by when converting. Set to 1.0 to do nothing.
	//! @param p_clipflag Set to true when clipping has occured.
	//! @returns One of t_io_result codes. Possible values are: io_result_success, io_result_error_data (invalid parameters), io_result_error_out_of_memory.

	virtual t_io_result run(const audio_chunk & p_chunk,
		mem_block_container & p_output,
		unsigned p_out_bps,
		unsigned p_out_bps_physical,
		bool p_dither,
		audio_sample p_prescale,
		bool & p_clipflag
		) = 0;


	//! Static helper; creates an instance of audio_postprocessor service.
	inline static bool g_create(service_ptr_t<audio_postprocessor> & p_out) {return service_enum_create_t(p_out,0);}


	virtual service_base * service_query(const GUID & guid)
	{
		if (guid == get_class_guid()) {service_add_ref();return this;}
		else return service_base::service_query(guid);
	}

	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}
protected:
	inline audio_postprocessor() {}
	inline ~audio_postprocessor() {}
};

#endif