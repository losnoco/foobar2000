#ifndef _FOOBAR2000_SDK_OUTPUT_H_
#define _FOOBAR2000_SDK_OUTPUT_H_

//! Structure describing PCM audio data format, with basic helper functions.
struct t_pcmspec
{
	inline t_pcmspec() {reset();}
	inline t_pcmspec(const t_pcmspec & p_source) {*this = p_source;}
	unsigned m_sample_rate;
	unsigned m_bits_per_sample;
	unsigned m_channels,m_channel_config;
	bool m_float;

	inline unsigned align() const {return (m_bits_per_sample / 8) * m_channels;}

	t_size time_to_bytes(double p_time) const {return (t_size) (p_time * m_sample_rate) * (m_bits_per_sample / 8) * m_channels;}
	double bytes_to_time(t_size p_bytes) const {return (double) (p_bytes / ((m_bits_per_sample / 8) * m_channels)) / (double) m_sample_rate;}

	inline bool operator==(/*const t_pcmspec & p_spec1,*/const t_pcmspec & p_spec2) const
	{
		return /*p_spec1.*/m_sample_rate == p_spec2.m_sample_rate 
			&& /*p_spec1.*/m_bits_per_sample == p_spec2.m_bits_per_sample
			&& /*p_spec1.*/m_channels == p_spec2.m_channels
			&& /*p_spec1.*/m_channel_config == p_spec2.m_channel_config
			&& /*p_spec1.*/m_float == p_spec2.m_float;
	}

	inline bool operator!=(/*const t_pcmspec & p_spec1,*/const t_pcmspec & p_spec2) const
	{
		return !(*this == p_spec2);
	}

	inline void reset() {m_sample_rate = 0; m_bits_per_sample = 0; m_channels = 0; m_channel_config = 0; m_float = false;}
	inline bool is_valid() const
	{
		return m_sample_rate >= 1000 && m_sample_rate <= 1000000 &&
			m_channels > 0 && m_channels <= 256 && m_channel_config != 0 &&
			(m_bits_per_sample == 8 || m_bits_per_sample == 16 || m_bits_per_sample == 24 || m_bits_per_sample == 32);
	}
};

class NOVTABLE output_device_enum_callback
{
public:
	virtual void on_device(const GUID & p_guid,const char * p_name,unsigned p_name_length) = 0;
};

class NOVTABLE output : public service_base
{
public:
	//! Retrieves amount of audio data queued for playback, in seconds.
	virtual double get_latency()=0;
	//! Sends new samples to the device. Allowed to be called only when update() indicates that the device is ready.
	virtual void process_samples(const t_pcmspec & p_spec,const void * p_buffer,t_size p_bytes)=0;
	//! Updates playback; queries whether the device is ready to receive new data.
	//! @param p_ready On success, receives value indicating whether the device is ready for next process_samples() call.
	virtual void update(bool & p_ready)=0;
	//! Pauses/unpauses playback.
	virtual void pause(bool p_state)=0;
	//! Flushes queued audio data. Called after seeking.
	virtual void flush()=0;
	//! Forces playback of queued data. Called when there's no more data to send, to prevent infinite waiting if output implementation starts actually playing after amount of data in internal buffer reaches some level.
	virtual void force_play()=0;
	
	//! Tests whether volume_set() method is supported.
	virtual bool volume_query_support() = 0;
	//! Sets playback volume. Some devices don't support this method; those return false from volume_query_support().
	//! @p_val Volume level in dB. Value of 0 indicates full ("100%") volume, negative values indciate different attenuation levels.
	virtual void volume_set(double p_val) = 0;

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	output() {}
	~output() {}
};

class NOVTABLE output_entry : public service_base
{
public:
	//! Instantiates output class.
	virtual void instantiate(service_ptr_t<output> & p_out,const GUID & p_device,double p_buffer_length) = 0;
	//! Enumerates devices supported by this output_entry implementation.
	virtual void enum_devices(output_device_enum_callback & p_callback) = 0;
	//! For internal use by backend. Each output_entry implementation must have its own guid.
	virtual GUID get_guid() = 0;
	//! For internal use by backend. Retrieves human-readable name of this output_entry implementation.
	virtual const char * get_name() = 0;

	//! Queries whether this output_entry implementation has advanced settings dialog - see advanced_settings_popup().
	virtual bool advanced_settings_query() = 0;
	//! Pops up advanced settings dialog. This method is optional - see advanced_settings_query().
	//! @param p_parent Parent window for the dialog.
	//! @param p_menupoint Point in screen coordinates - can be used to display a simple popup menu with options to be checked instead of a full dialog.
	virtual void advanced_settings_popup(HWND p_parent,POINT p_menupoint) = 0;

	//! Helper - locates output_entry instance with specified GUID. See: output_entry::get_guid().
	static bool g_find(service_ptr_t<output_entry> & p_out,const GUID & p_guid);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	output_entry() {}
	~output_entry() {}
};

//! Helper; implements output_entry for specific output class implementation. output_entry methods are forwarded to static methods of your output class. Use output_factory_t<myoutputclass> instead of using this class directly.
template<class T>
class output_entry_impl_t : public output_entry
{
public:
	void instantiate(service_ptr_t<output> & p_out,const GUID & p_device,double p_buffer_length)
	{
		service_ptr_t<T> ptr = new service_impl_t<T>();
		ptr->initialize(p_device,p_buffer_length);
		p_out = ptr.get_ptr();
	}
	void enum_devices(output_device_enum_callback & p_callback) {T::g_enum_devices(p_callback);}
	GUID get_guid() {return T::g_get_guid();}
	const char * get_name() {return T::g_get_name();}
	bool advanced_settings_query() {return T::g_advanced_settings_query();}
	void advanced_settings_popup(HWND p_parent,POINT p_menupoint) {T::g_advanced_settings_popup(p_parent,p_menupoint);}
};


//! Use this to register your output implementation.
template<class T>
class output_factory_t : public service_factory_single_t<output_entry,output_entry_impl_t<T> > {};

//! Helper; base class for output implementations, simplifying some operations such as device opening on PCM format change or updates.
class output_impl : public output
{
protected:
	//! To be implemented by output_impl derived class. Opens device with specified parameters; called before any write() calls.
	virtual void open(const t_pcmspec & p_spec) = 0;
	//! To be implemented by output_impl derived class. Queues new data to be written to device.
	virtual void write(const void * p_buffer,t_size p_bytes) = 0;
	//! To be implemented by output_impl derived class. Returns how much data can be currently queued.
	virtual t_size can_write_bytes() = 0;
	//! To be implemented by output_impl derived class. Returns how much data is currently pending playback.
	virtual t_size get_latency_bytes() = 0;

	//! To be implemented by output_impl derived class. See: output::pause()
	virtual void pause(bool p_state)=0;
	//! To be implemented by output_impl derived class. See: output::flush()
	virtual void on_flush() = 0;
	//! To be implemented by output_impl derived class. Called periodically with each update.
	virtual void on_update() = 0;
	//! To be implemented by output_impl derived class. See: output::force_play()
	virtual void force_play()=0;
	//! To be implemented by output_impl derived class. See: output::volume_query_support()
	virtual bool volume_query_support() = 0;
	//! To be implemented by output_impl derived class. See: output::volume_set()
	virtual void volume_set(double p_val) = 0;

	output_impl();
private:
	void process_samples(const t_pcmspec & p_spec,const void * p_buffer,t_size p_bytes);
	void update(bool & p_ready);
	void flush();
	double get_latency();

	t_pcmspec m_pcmspec;
	t_pcmspec m_incoming_pcmspec;
	pfc::array_t<t_uint8,pfc::alloc_fast_aggressive> m_incoming_buffer;
	t_size m_incoming_buffer_ptr;
	bool m_is_open;
};

#endif //_FOOBAR2000_SDK_OUTPUT_H_