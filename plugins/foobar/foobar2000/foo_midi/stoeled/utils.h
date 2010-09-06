#if !defined(_UTILS_H_INCLUDED_)
#define _UTILS_H_INCLUDED_


//#include <foobar2000.h>

#include <pfc/pfc.h>
#include <shared.h>

class grow_buf
{
public:
	enum mem_logic_t {ALLOC_DEFAULT,ALLOC_FAST,ALLOC_FAST_DONTGODOWN};
private:
	void * data;
	unsigned size,used;
	mem_logic_t mem_logic;
public:
	inline void set_mem_logic(mem_logic_t v) {mem_logic=v;}
	inline mem_logic_t get_mem_logic() const {return mem_logic;}

	void prealloc(unsigned size);

	inline grow_buf() {data=0;size=0;used=0;mem_logic=ALLOC_DEFAULT;}
	inline ~grow_buf() {if (data) free(data);}

	inline unsigned get_size() const {return used;}

	inline const void * get_ptr() const {return data;}
	inline void * get_ptr() {return data;}

	inline void truncate(unsigned new_used) {set_size(new_used);}
	void * set_size(unsigned new_used);

	inline void * check_size(unsigned new_size)
	{
		if (used<new_size) return set_size(new_size);
		else return get_ptr();
	}

	inline operator const void * () const {return get_ptr();}
	inline operator void * () {return get_ptr();}

	void* write_ptr(const void *ptr, unsigned bytes,unsigned start=0);

	inline void* write(const void *ptr, unsigned bytes) {return write_ptr(ptr,bytes,used);}
	inline void* write_byte(const unsigned char val) {return write(&val, sizeof(val));}
	inline void* write_word(const unsigned short val) {return write(&val, sizeof(val));}
	inline void* write_dword(const unsigned val) {return write(&val, sizeof(val));}

	inline void* write_dword_ptr(const unsigned val, unsigned start) {return write_ptr(&val, sizeof(val), start);}

	inline void zeromemory() {memset(get_ptr(),0,used);}

	inline void reset() {if (data) free(data);data=0;size=0;used=0;}

	void * set_data(const void * src,unsigned size)
	{
		void * out = set_size(size);
		if (out) memcpy(out,src,size);
		return out;
	}

	void operator=(const grow_buf & src) {set_data(src.get_ptr(),src.get_size());}

	void * finish()
	{
		void * rval = data;
		data = 0;
		size = 0;
		used = 0;
		return rval;
	}
};

class NOVTABLE CStream
{
public:
	virtual UINT ReadData(void*,UINT,bool*)=0;
	virtual void Flush()=0;
	virtual ~CStream() {};

	//for sampling
	virtual void Pause(int) {};
	virtual void Eof() {}
};

class CPipe : public CStream
{
	BYTE* buf;
	volatile UINT buf_s,buf_n,buf_rp,buf_wp;
	critical_section sec;
	UINT align;
	volatile bool closed;
public:	
	void WriteData(void*,UINT);
	UINT CanWrite() {return buf_s-buf_n;}
	UINT ReadData(void*,UINT,bool*);
	void Flush()
	{
		sec.enter();
		buf_n=0;
		sec.leave();

	}
	CPipe(UINT _align=4,UINT freq=44100)
	{
		buf_s=MulDiv(1024*256,freq,22050);
		buf=(BYTE*)malloc(buf_s);
		buf_wp=buf_rp=0;
		buf_n=0;
		align=_align;
		closed=0;
	}
	~CPipe()
	{
		if (buf) free(buf);
	}
	void Eof() {closed=1;}
};


class MIDI_file;

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	WORD fmt,trax,dtx;
} MIDIHEADER;
#pragma pack(pop)

WORD _inline rev16(WORD x) {return (x>>8)|(x<<8);}
//#define rev16(X) (((X)&0xFF)<<8)|(((X)>>8)&0xFF)
DWORD _fastcall rev32(DWORD);

#define _rv(X) ((((DWORD)(X)&0xFF)<<24)|(((DWORD)(X)&0xFF00)<<8)|(((DWORD)(X)&0xFF0000)>>8)|(((DWORD)(X)&0xFF000000)>>24))

#define FixHeader(H) {(H).fmt=rev16((H).fmt);(H).trax=rev16((H).trax);(H).dtx=rev16((H).dtx);}

struct write_buf;

typedef struct
{
	int pos,tm;
} TMAP_ENTRY;

struct CTempoMap
{
public:
	TMAP_ENTRY *data;
	int pos,size;
	void AddEntry(int _p,int tm);
	~CTempoMap() {if (data) free(data);}
	int BuildTrack(grow_buf & out);
};

CTempoMap* tmap_merge(CTempoMap*,CTempoMap*);

typedef struct
{
	int pos,ofs,len;
} SYSEX_ENTRY;

struct CSysexMap
{
public:
	DWORD d_size,e_size;
	SYSEX_ENTRY *events;
	BYTE* data;
	int pos,d_pos;
	void CleanUp();
	void AddEvent(const BYTE* e,DWORD s,DWORD t);
	~CSysexMap();
	CSysexMap* Translate(MIDI_file* _mf);//MIDI_file* mf
	int BuildTrack(grow_buf & out);
	const char* GetType();
};

struct CMarkerMap
{
public:
	DWORD d_size,e_size;
	SYSEX_ENTRY *events;
	BYTE* data;
	int pos,d_pos;
	void AddEvent(const BYTE* e,DWORD s,DWORD t);
	~CMarkerMap();
	CMarkerMap* Translate(MIDI_file* _mf);//MIDI_file* mf
};


typedef struct tagKAR
{
	UINT time;
	UINT start,end;
	BOOL foo;
} KAR_ENTRY;


KAR_ENTRY * kmap_create(MIDI_file* mf,UINT prec,UINT * num,char** text);


CTempoMap* tmap_create();
CSysexMap* smap_create();

CMarkerMap* mmap_create();


int EncodeDelta(BYTE* dst,int d);
int DecodeDelta(const BYTE* src,int* _d,int sz);
int ReadSysex(const BYTE* src,int ml);

BOOL DoOpenFile(HWND w,char* fn,char* filt,char* ext,BOOL save);

typedef void (*SYSEXFUNC)(void*,BYTE*,UINT);
void sysex_startup(SYSEXFUNC,void*);
void sysex_startup_midiout(UINT m_id);
bool need_sysex_start();

typedef struct
{
	DWORD tm;
	DWORD ev;
} MIDI_EVENT;

MIDI_EVENT* do_table(MIDI_file* mf,UINT prec,UINT * size,/*UINT* _lstart,*/DWORD cflags);

void gb_write_delta(grow_buf & gb,DWORD d);

void do_messages(HWND w,bool* br);
ATOM do_callback_class(WNDPROC p);
HWND create_callback_wnd(ATOM cl,void* p);

class sysex_table
{
private:
	struct entry
	{
		entry * next;
		int size,time;
		BYTE * data;
	};
	entry * entries;
	enum {MHP_MAGIC='0PHM'};
public:
	sysex_table() {entries=0;}
	~sysex_table() {reset();}
	int num_entries() const;
	int get_entry(int idx,BYTE ** p_data,int * p_size,int * p_time) const;
	void insert_entry(int idx,BYTE * data,int size,int time);
	int remove_entry(int idx);
	
	inline void add_entry(BYTE * data,int size,int time) {insert_entry(num_entries(),data,size,time);}
	inline void modify_entry(int idx,BYTE * data,int size,int time) {remove_entry(idx);insert_entry(idx,data,size,time);}
	inline void reset() {while(entries) remove_entry(0);}
	inline int get_time(int idx) const {int time;return get_entry(idx,0,0,&time) ? time : 0;}

	int file_read(const char * path);
	int file_write(const char * path) const;
	void * memblock_write(int * size) const;
	int memblock_read(const void * ptr,int size);

	int print_preview(int idx,char * out) const;
	void print_edit(int idx,HWND wnd) const;
	
	void copy(const sysex_table & src);
	sysex_table(const sysex_table & src) {entries=0;copy(src);}
	sysex_table& operator=(const sysex_table & src) {copy(src);return *this;}
	int is_empty() {return !entries;}
};

class meta_table
{
	struct entry
	{
		entry * next;
		int id;
		char * data;
	};
	entry * entries;
public:
	meta_table() : entries(0) {}
	~meta_table() {reset();}
	unsigned num_entries() const;

	inline const char * enum_name(unsigned idx) const { return enum_name(enum_entry(idx)); }
	inline const char * enum_data(unsigned idx) const { return enum_data(enum_entry(idx)); }

	static const char * enum_name(const void * entry);
	static const char * enum_data(const void * entry);

	const void * enum_entry(unsigned idx) const;
	static const void * enum_next_entry(const void * ent);

	void insert_entry(unsigned idx, int id, const char * data, unsigned size);
	bool remove_entry(unsigned idx);

	inline void add_entry(int id, const char * data, unsigned size) {insert_entry(num_entries(), id, data, size);}
	inline void modify_entry(unsigned idx, int id, const char * data, unsigned size) {remove_entry(idx); insert_entry(idx, id, data, size);}
	inline void reset() {while(entries) remove_entry(0);}

	bool is_empty() {return !entries;}
};

#endif
