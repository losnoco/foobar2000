class mp3_seektable
{
	class seektable_manager * owner;
	unsigned references;
	t_int64 last_used;
	t_filetimestamp m_timestamp;
	string8 file;
	bool detached;
	struct entry
	{
		t_int64 frame,offset;	//offsets are from the beginning of first frame, table of entries is sorted by frame number
		inline entry() {}
		inline entry(t_int64 p_frame,t_int64 p_offset) : frame(p_frame), offset(p_offset) {}
	};
	mem_block_list<entry> data;
	static int compare(entry const & e1,const t_int64 & desired_frame)
	{
		if (desired_frame > e1.frame) return -1;
		else if (desired_frame < e1.frame) return 1;
		else return 0;
	}
public:
	void on_detach();
	unsigned get_refcount();
	void add_ref();
	void release();
	void add_entry(t_int64 frame,t_int64 offset);
	mp3_seektable(const char * src,t_filetimestamp p_timestamp,t_int64 counter,class seektable_manager * p_owner);
	void set_last_used(t_int64 num);
	t_int64 get_last_used();
	bool is_our_file(const char * p_file,t_filetimestamp p_timestamp) const;
	bool is_our_file(const char * p_file) const;

	void find_entry(t_int64 desired_frame,t_int64 & out_frame,t_int64 & out_offset);
	void find_entry_slow(t_int64 desired_frame,t_int64 & out_frame,t_int64 & out_offset);
};


class seektable_manager
{
public:
	inline void lock() {sync.enter();}
	inline void unlock() {sync.leave();}

	void on_table_released()
	{
		remove_old_tables(MAX_TABLES);
	}
private:
	ptr_list_t<mp3_seektable> tables;
	enum {MAX_TABLES = 16};
	t_int64 usage_counter;
	critical_section sync;

	int find_table(const char * file,t_filetimestamp p_timestamp);
	void remove_old_tables(unsigned max);

public:
	void find_entry(const char * file,t_filetimestamp p_timestamp,t_int64 desired_frame,t_int64 &out_frame,t_int64 &out_offset);

	seektable_manager();

	~seektable_manager();

	void add_entry(const char * file,t_filetimestamp p_timestamp,t_int64 frame,t_int64 offset);

	void remove_table(const char * file);

	mp3_seektable * make_table(const char * file,t_filetimestamp p_timestamp);

};
