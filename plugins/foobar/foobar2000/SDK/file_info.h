#ifndef _FILE_INFO_H_
#define _FILE_INFO_H_

struct replaygain_info
{
	float m_album_gain,m_track_gain;
	float m_album_peak,m_track_peak;

	enum {text_buffer_size = 16 };
	typedef char t_text_buffer[text_buffer_size];

	enum { peak_invalid = -1, gain_invalid = -1000 };

	static bool g_format_gain(float p_value,char p_buffer[text_buffer_size]);
	static bool g_format_peak(float p_value,char p_buffer[text_buffer_size]);

	inline bool format_album_gain(char p_buffer[text_buffer_size]) const {return g_format_gain(m_album_gain,p_buffer);}
	inline bool format_track_gain(char p_buffer[text_buffer_size]) const {return g_format_gain(m_track_gain,p_buffer);}
	inline bool format_album_peak(char p_buffer[text_buffer_size]) const {return g_format_peak(m_album_peak,p_buffer);}
	inline bool format_track_peak(char p_buffer[text_buffer_size]) const {return g_format_peak(m_track_peak,p_buffer);}

	void set_album_gain_text(const char * p_text,unsigned p_text_len = infinite);
	void set_track_gain_text(const char * p_text,unsigned p_text_len = infinite);
	void set_album_peak_text(const char * p_text,unsigned p_text_len = infinite);
	void set_track_peak_text(const char * p_text,unsigned p_text_len = infinite);

	static bool g_is_meta_replaygain(const char * p_name,unsigned p_name_len = infinite);
	bool set_from_meta_ex(const char * p_name,unsigned p_name_len,const char * p_value,unsigned p_value_len);
	inline bool set_from_meta(const char * p_name,const char * p_value) {return set_from_meta_ex(p_name,infinite,p_value,infinite);}

	inline bool is_album_gain_present() const {return m_album_gain != gain_invalid;}
	inline bool is_track_gain_present() const {return m_track_gain != gain_invalid;}
	inline bool is_album_peak_present() const {return m_album_peak != peak_invalid;}
	inline bool is_track_peak_present() const {return m_track_peak != peak_invalid;}
	
	inline void remove_album_gain() {m_album_gain = gain_invalid;}
	inline void remove_track_gain() {m_track_gain = gain_invalid;}
	inline void remove_album_peak() {m_album_peak = peak_invalid;}
	inline void remove_track_peak() {m_track_peak = peak_invalid;}

	unsigned	get_value_count();

	static replaygain_info g_merge(replaygain_info r1,replaygain_info r2);

	static bool g_equal(const replaygain_info & item1,const replaygain_info & item2);

	void reset();	
};

inline bool operator==(const replaygain_info & item1,const replaygain_info & item2) {return replaygain_info::g_equal(item1,item2);}
inline bool operator!=(const replaygain_info & item1,const replaygain_info & item2) {return !replaygain_info::g_equal(item1,item2);}

static const replaygain_info replaygain_info_invalid = {replaygain_info::gain_invalid,replaygain_info::gain_invalid,replaygain_info::peak_invalid,replaygain_info::peak_invalid};

class NOVTABLE file_info
{
public:
	virtual double		get_length() const = 0;
	virtual void		set_length(double p_length) = 0;

	virtual void			set_replaygain(const replaygain_info & p_info) = 0;
	virtual replaygain_info	get_replaygain() const = 0;

	virtual unsigned	meta_get_count() const = 0;
	virtual const char*	meta_enum_name(unsigned p_index) const = 0;
	virtual unsigned	meta_enum_value_count(unsigned p_index) const = 0;
	virtual const char*	meta_enum_value(unsigned p_index,unsigned p_value_number) const = 0;
	virtual unsigned	meta_find_ex(const char * p_name,unsigned p_name_length) const;
	virtual unsigned	meta_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) = 0;
	virtual void		meta_insert_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length) = 0;
	virtual void		meta_remove_mask(const bit_array & p_mask) = 0;
	virtual void		meta_reorder(const unsigned * p_order) = 0;
	virtual void		meta_remove_values(unsigned p_index,const bit_array & p_mask) = 0;

	virtual void		meta_modify_value_ex(unsigned p_index,unsigned p_value_index,const char * p_value,unsigned p_value_length) = 0;
	
	virtual unsigned	info_get_count() const = 0;
	virtual const char*	info_enum_name(unsigned p_index) const = 0;
	virtual const char*	info_enum_value(unsigned p_index) const = 0;
	virtual unsigned	info_set_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) = 0;
	virtual void		info_remove_mask(const bit_array & p_mask) = 0;
	virtual unsigned	info_find_ex(const char * p_name,unsigned p_name_length) const;

	virtual void		copy(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	virtual void		copy_meta(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass
	virtual void		copy_info(const file_info & p_source);//virtualized for performance reasons, can be faster in two-pass

	bool			meta_exists_ex(const char * p_name,unsigned p_name_length) const;
	void			meta_remove_field_ex(const char * p_name,unsigned p_name_length);
	void			meta_remove_index(unsigned p_index);
	void			meta_remove_all();
	void			meta_remove_value(unsigned p_index,unsigned p_value);
	const char *	meta_get_ex(const char * p_name,unsigned p_name_length,unsigned p_index) const;
	unsigned		meta_get_count_by_name_ex(const char * p_name,unsigned p_name_length) const;
	void			meta_add_value_ex(unsigned p_index,const char * p_value,unsigned p_value_length);
	unsigned		meta_add_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	unsigned		meta_calc_total_value_count() const;
	bool			meta_format(const char * p_name,string_base & p_out) const;
	
	
	bool			info_exists_ex(const char * p_name,unsigned p_name_length) const;
	void			info_remove_index(unsigned p_index);
	void			info_remove_all();
	bool			info_remove_ex(const char * p_name,unsigned p_name_length);
	const char *	info_get_ex(const char * p_name,unsigned p_name_length) const;

	inline unsigned		meta_find(const char * p_name) const	{return meta_find_ex(p_name,infinite);}
	inline bool			meta_exists(const char * p_name) const		{return meta_exists_ex(p_name,infinite);}
	inline void			meta_remove_field(const char * p_name)		{meta_remove_field_ex(p_name,infinite);}
	inline unsigned		meta_set(const char * p_name,const char * p_value)		{return meta_set_ex(p_name,infinite,p_value,infinite);}
	inline void			meta_insert_value(unsigned p_index,unsigned p_value_index,const char * p_value) {meta_insert_value_ex(p_index,p_value_index,p_value,infinite);}
	inline void			meta_add_value(unsigned p_index,const char * p_value)	{meta_add_value_ex(p_index,p_value,infinite);}
	inline const char*	meta_get(const char * p_name,unsigned p_index) const	{return meta_get_ex(p_name,infinite,p_index);}
	inline unsigned		meta_get_count_by_name(const char * p_name) const		{return meta_get_count_by_name_ex(p_name,infinite);}
	inline unsigned		meta_add(const char * p_name,const char * p_value)		{return meta_add_ex(p_name,infinite,p_value,infinite);}
	inline void			meta_modify_value(unsigned p_index,unsigned p_value_index,const char * p_value) {meta_modify_value_ex(p_index,p_value_index,p_value,infinite);}

	

	inline unsigned		info_set(const char * p_name,const char * p_value)	{return info_set_ex(p_name,infinite,p_value,infinite);}
	inline unsigned		info_find(const char * p_name) const				{return info_find_ex(p_name,infinite);}
	inline unsigned		info_exists(const char * p_name) const				{return info_exists_ex(p_name,infinite);}
	inline bool			info_remove(const char * p_name)					{return info_remove_ex(p_name,infinite);}
	inline const char *	info_get(const char * p_name) const					{return info_get_ex(p_name,infinite);}

	bool				info_set_replaygain_ex(const char * p_name,unsigned p_name_len,const char * p_value,unsigned p_value_len);
	inline bool			info_set_replaygain(const char * p_name,const char * p_value) {return info_set_replaygain_ex(p_name,infinite,p_value,infinite);}
	void				info_set_replaygain_auto_ex(const char * p_name,unsigned p_name_len,const char * p_value,unsigned p_value_len);
	inline void			info_set_replaygain_auto(const char * p_name,const char * p_value) {info_set_replaygain_auto_ex(p_name,infinite,p_value,infinite);}

	

	void		copy_meta_single(const file_info & p_source,unsigned p_index);
	void		copy_info_single(const file_info & p_source,unsigned p_index);
	void		copy_meta_single_by_name_ex(const file_info & p_source,const char * p_name,unsigned p_name_length);
	void		copy_info_single_by_name_ex(const file_info & p_source,const char * p_name,unsigned p_name_length);
	inline void	copy_meta_single_by_name(const file_info & p_source,const char * p_name) {copy_meta_single_by_name_ex(p_source,p_name,infinite);}
	inline void	copy_info_single_by_name(const file_info & p_source,const char * p_name) {copy_info_single_by_name_ex(p_source,p_name,infinite);}
	void		reset();
	void		reset_replaygain();
	void		copy_meta_single_rename_ex(const file_info & p_source,unsigned p_index,const char * p_new_name,unsigned p_new_name_length);
	inline void	copy_meta_single_rename(const file_info & p_source,unsigned p_index,const char * p_new_name) {copy_meta_single_rename_ex(p_source,p_index,p_new_name,infinite);}

	t_int64 info_get_int(const char * name) const;
	t_int64 info_get_length_samples() const;
	double info_get_float(const char * name) const;
	void info_set_int(const char * name,t_int64 value);
	void info_set_float(const char * name,double value,unsigned precision,bool force_sign = false,const char * unit = 0);
	void info_set_replaygain_track_gain(float value);
	void info_set_replaygain_album_gain(float value);
	void info_set_replaygain_track_peak(float value);
	void info_set_replaygain_album_peak(float value);

	inline t_int64 info_get_bitrate_vbr() const {return info_get_int("bitrate_dynamic");}
	inline void info_set_bitrate_vbr(t_int64 val) {info_set_int("bitrate_dynamic",val);}
	inline t_int64 info_get_bitrate() const {return info_get_int("bitrate");}
	inline void info_set_bitrate(t_int64 val) {info_set_int("bitrate",val);}

	void info_calculate_bitrate(t_filesize p_filesize,double p_length);

	unsigned info_get_decoded_bps() const;//what bps the stream originally was (before converting to audio_sample), 0 if unknown

	void merge(const list_base_const_t<const file_info*> & p_sources);

	void meta_set_ansi_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length);
	inline void meta_set_ansi(const char * p_name,const char * p_value) {meta_set_ansi_ex(p_name,infinite,p_value,infinite);}

	bool are_meta_fields_identical(unsigned p_index1,unsigned p_index2) const;

	inline const file_info & operator=(const file_info & p_source) {copy(p_source);return *this;}

protected:
	file_info() {}
	~file_info() {}
	void	copy_meta_single_nocheck(const file_info & p_source,unsigned p_index);
	void	copy_info_single_nocheck(const file_info & p_source,unsigned p_index);
	void	copy_meta_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,unsigned p_name_length);
	void	copy_info_single_by_name_nocheck_ex(const file_info & p_source,const char * p_name,unsigned p_name_length);
	inline void	copy_meta_single_by_name_nocheck(const file_info & p_source,const char * p_name) {copy_meta_single_by_name_nocheck_ex(p_source,p_name,infinite);}
	inline void	copy_info_single_by_name_nocheck(const file_info & p_source,const char * p_name) {copy_info_single_by_name_nocheck_ex(p_source,p_name,infinite);}

	virtual unsigned	meta_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) = 0;
	virtual unsigned	info_set_nocheck_ex(const char * p_name,unsigned p_name_length,const char * p_value,unsigned p_value_length) = 0;
	inline unsigned		meta_set_nocheck(const char * p_name,const char * p_value) {return meta_set_nocheck_ex(p_name,infinite,p_value,infinite);}
	inline unsigned		info_set_nocheck(const char * p_name,const char * p_value) {return info_set_nocheck_ex(p_name,infinite,p_value,infinite);}
};


#endif //_FILE_INFO_H_