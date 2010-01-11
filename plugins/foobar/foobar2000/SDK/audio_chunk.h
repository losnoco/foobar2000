#ifndef _AUDIO_CHUNK_H_
#define _AUDIO_CHUNK_H_



class NOVTABLE audio_chunk
{
protected:
	audio_chunk() {}
	~audio_chunk() {}	
public:

	//order of interleaved channel data must be same as order of indicator declarations
	enum
	{
		channel_front_left			= 1<<0,
		channel_front_right			= 1<<1,
		channel_front_center		= 1<<2,
		channel_lfe					= 1<<3,
		channel_back_left			= 1<<4,
		channel_back_right			= 1<<5,
		channel_front_center_right	= 1<<6,
		channel_front_center_left	= 1<<7,
		channel_back_center			= 1<<8,
		channel_side_left			= 1<<9,
		channel_side_right			= 1<<10,
		channel_top_center			= 1<<11,
		channel_top_front_left		= 1<<12,
		channel_top_front_center	= 1<<13,
		channel_top_front_right		= 1<<14,
		channel_top_back_left		= 1<<15,
		channel_top_back_center		= 1<<16,
		channel_top_back_right		= 1<<17,

		channel_config_mono = channel_front_center,
		channel_config_stereo = channel_front_left | channel_front_right,
		channel_config_5point1 = channel_front_left | channel_front_right | channel_back_left | channel_back_right | channel_front_center | channel_lfe,
	};

	static unsigned g_guess_channel_config(unsigned count);

	static DWORD g_channel_config_to_wfx(unsigned p_config);
	static unsigned g_channel_config_from_wfx(DWORD p_wfx);

	static unsigned g_extract_channel_flag(unsigned p_config,unsigned p_index);
	static unsigned g_count_channels(unsigned p_config);

	

	virtual audio_sample * get_data()=0;
	virtual const audio_sample * get_data() const = 0;
	virtual unsigned get_data_size() const = 0;//buffer size in audio_samples; must be at least samples * nch;
	virtual bool set_data_size(unsigned new_size)=0;
	
	virtual unsigned get_srate() const = 0;
	inline unsigned get_sample_rate() const {return get_srate();}
	virtual void set_srate(unsigned val)=0;
	inline void set_sample_rate(unsigned val) {set_srate(val);}
	virtual unsigned get_channels() const = 0;
	virtual unsigned get_channel_config() const = 0;
	virtual void set_channels(unsigned val,unsigned setup)=0;

	void set_channels(unsigned val);

	virtual unsigned get_sample_count() const = 0;
	virtual void set_sample_count(unsigned val)=0;

	inline bool check_data_size(unsigned new_size) {if (new_size > get_data_size()) return set_data_size(new_size); else return true;}
	
	inline bool copy_from(const audio_chunk * src)
	{
		return set_data(src->get_data(),src->get_sample_count(),src->get_channels(),src->get_srate(),src->get_channel_config());
	}

	inline double get_duration() const
	{
		double rv = 0;
		unsigned srate = get_srate (), samples = get_sample_count();
		if (srate>0 && samples>0) rv = (double)samples/(double)srate;
		return rv;
	}
	
	inline bool is_empty() const {return get_channels()==0 || get_srate()==0 || get_sample_count()==0;}
	
	bool is_valid();

	inline unsigned get_data_length() const {return get_sample_count() * get_channels();}//actual amount of audio_samples in buffer

	inline void reset()
	{
		set_sample_count(0);
		set_srate(0);
		set_channels(0);
	}
	inline void reset_data()
	{
		reset();
		set_data_size(0);
	}
	
	bool set_data(const audio_sample * src,unsigned samples,unsigned nch,unsigned srate,unsigned channel_config);//returns false on failure (eg. memory error)
	
	inline bool set_data(const audio_sample * src,unsigned samples,unsigned nch,unsigned srate) {return set_data(src,samples,nch,srate,g_guess_channel_config(nch));}
	

	//helper routines for converting different input data formats
	inline bool set_data_fixedpoint(const void * ptr,unsigned bytes,unsigned srate,unsigned nch,unsigned bps,unsigned channel_config)
	{
		return set_data_fixedpoint_ex(ptr,bytes,srate,nch,bps,(bps==8 ? FLAG_UNSIGNED : FLAG_SIGNED) | flags_autoendian(), channel_config);
	}

	inline bool set_data_fixedpoint_unsigned(const void * ptr,unsigned bytes,unsigned srate,unsigned nch,unsigned bps,unsigned channel_config)
	{
		return set_data_fixedpoint_ex(ptr,bytes,srate,nch,bps,FLAG_UNSIGNED | flags_autoendian(), channel_config);
	}

	inline bool set_data_fixedpoint_signed(const void * ptr,unsigned bytes,unsigned srate,unsigned nch,unsigned bps,unsigned channel_config)
	{
		return set_data_fixedpoint_ex(ptr,bytes,srate,nch,bps,FLAG_SIGNED | flags_autoendian(), channel_config);
	}

	enum
	{
		FLAG_LITTLE_ENDIAN = 1,
		FLAG_BIG_ENDIAN = 2,
		FLAG_SIGNED = 4,
		FLAG_UNSIGNED = 8,
	};

	inline static unsigned flags_autoendian()
	{
		return byte_order::machine_is_big_endian() ? FLAG_BIG_ENDIAN : FLAG_LITTLE_ENDIAN;
	}

	bool set_data_fixedpoint_ex(const void * ptr,unsigned bytes,unsigned p_sample_rate,unsigned p_channels,unsigned p_bits_per_sample,unsigned p_flags,unsigned p_channel_config);//p_flags - see FLAG_* above

	bool set_data_floatingpoint_ex(const void * ptr,unsigned bytes,unsigned p_sample_rate,unsigned p_channels,unsigned p_bits_per_sample,unsigned p_flags,unsigned p_channel_config);//signed/unsigned flags dont apply

#if audio_sample_size == 64
	bool set_data_32(const float * src,unsigned samples,unsigned nch,unsigned srate);
	inline bool set_data_64(const double * src,unsigned samples,unsigned nch,unsigned srate) {return set_data(src,samples,nch,srate);}
#else
	inline bool set_data_32(const float * src,unsigned samples,unsigned nch,unsigned srate) {return set_data(src,samples,nch,srate);}
	bool set_data_64(const double * src,unsigned samples,unsigned nch,unsigned srate);
#endif

	bool pad_with_silence_ex(unsigned samples,unsigned hint_nch,unsigned hint_srate);
	bool pad_with_silence(unsigned samples);
	bool insert_silence_fromstart(unsigned samples);
	unsigned skip_first_samples(unsigned samples);

	audio_sample get_peak(audio_sample p_peak = 0) const;

	void scale(audio_sample p_value);


	const audio_chunk & operator=(const audio_chunk & p_source) {copy_from(&p_source);return *this;}
};

class audio_chunk_impl : public audio_chunk
{
	mem_block_aligned_t<audio_sample> m_data;
	unsigned m_srate,m_nch,m_samples,m_setup;
public:
	audio_chunk_impl() : m_srate(0), m_nch(0), m_samples(0), m_setup(0) {}
	audio_chunk_impl(const audio_sample * src,unsigned samples,unsigned nch,unsigned srate) : m_srate(0), m_nch(0), m_samples(0)
	{set_data(src,samples,nch,srate);}
	
	virtual audio_sample * get_data() {return m_data;}
	virtual const audio_sample * get_data() const {return m_data;}
	virtual unsigned get_data_size() const {return m_data.get_size();}
	virtual bool set_data_size(unsigned new_size) {return m_data.set_size(new_size);}
	
	virtual unsigned get_srate() const {return m_srate;}
	virtual void set_srate(unsigned val) {m_srate=val;}
	virtual unsigned get_channels() const {return m_nch;}
	virtual unsigned get_channel_config() const {return m_setup;}
	virtual void set_channels(unsigned val,unsigned setup) {m_nch = val;m_setup = setup;}
	void set_channels(unsigned val);

	virtual unsigned get_sample_count() const {return m_samples;}
	virtual void set_sample_count(unsigned val) {m_samples = val;}

	inline void set_mem_logic(mem_block::t_mem_logic v) {m_data.set_mem_logic(v);}
	inline mem_block::t_mem_logic get_mem_logic() const {return m_data.get_mem_logic();}

	const audio_chunk_impl & operator=(const audio_chunk & p_source) {copy_from(&p_source);return *this;}
	const audio_chunk_impl & operator=(const audio_chunk_impl & p_source) {copy_from(&p_source);return *this;}
};

typedef audio_chunk_impl audio_chunk_i;//for compatibility

#endif //_AUDIO_CHUNK_H_