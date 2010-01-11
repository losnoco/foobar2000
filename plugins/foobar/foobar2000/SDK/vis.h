#ifndef _FOOBAR2000_VIS_H_
#define _FOOBAR2000_VIS_H_

#include "service.h"
#include "audio_chunk.h"

class NOVTABLE visualisation_stream : public service_base {
public:
	//! Retrieves absolute playback time since last playback start or seek. You typically pass value retrieved by this function - optionally with offset added - to other visualisation_stream methods.
	virtual bool get_absolute_time(double & p_value) = 0;
	
	//! Retrieves an audio chunk starting at specified offset (see get_absolute_time()), of specified length.
	//! @returns False when requested timestamp is out of available range, true on success.
	virtual bool get_chunk_absolute(audio_chunk & p_chunk,double p_offset,double p_requested_length) = 0;
	//! Retrieves spectrum for audio data at specified offset (see get_absolute_time()), with specified FFT size.
	//! @param p_chunk Receives spectrum data. audio_chunk type is used for consistency (since required functionality is identical to provided by audio_chunk), the data is *not* PCM. Returned sample count is equal to half of FFT size; channels and sample rate are as in audio stream the spectrum was generated from.
	//! @param p_offset Timestamp of spectrum to retrieve. See get_absolute_time().
	//! @param p_fft_size FFT size to use for spectrum generation. Must be a power of 2.	
	//! @returns False when requested timestamp is out of available range, true on success.
	virtual bool get_spectrum_absolute(audio_chunk & p_chunk,double p_offset,unsigned p_fft_size) = 0;
	
	//! Generates fake audio chunk to display when get_chunk_absolute() fails - e.g. shortly after visualisation_stream creation data for currently played audio might not be available yet.
	//! Throws std::exception derivatives on failure.
	virtual void make_fake_chunk_absolute(audio_chunk & p_chunk,double p_offset,double p_requested_length) = 0;
	//! Generates fake spectrum to display when get_spectrum_absolute() fails - e.g. shortly after visualisation_stream creation data for currently played audio might not be available yet.
	//! Throws std::exception derivatives on failure.
	virtual void make_fake_spectrum_absolute(audio_chunk & p_chunk,double p_offset,unsigned p_fft_size) = 0;

	
	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	visualisation_stream() {}
	~visualisation_stream() {}
};

class NOVTABLE visualisation_manager : public service_base {
public:
	//! @param p_flags reserved for future use. Must be set to zero.
	virtual void create_stream(service_ptr_t<visualisation_stream> & p_out,unsigned p_flags) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	visualisation_manager() {}
	~visualisation_manager() {}
};


#endif //_FOOBAR2000_VIS_H_