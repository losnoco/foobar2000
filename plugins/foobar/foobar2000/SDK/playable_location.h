#ifndef _FOOBAR2000_PLAYABLE_LOCATION_H_
#define _FOOBAR2000_PLAYABLE_LOCATION_H_

#include "service.h"

//playable_location stores location of a playable resource, currently implemented as file path and integer for indicating multiple playable "subsongs" per file
//also see: file_info.h
//for getting more info about resource referenced by a playable_location, see metadb.h

//char* strings are all UTF-8

class NOVTABLE playable_location//interface (for passing around between DLLs)
{
protected:
	playable_location() {}
	~playable_location() {}
public:
	virtual const char * get_path() const =0;
	virtual void set_path(const char*)=0;
	virtual t_uint32 get_subsong() const =0;
	virtual void set_subsong(t_uint32)=0;
	
	void copy(const playable_location & src)
	{
		set_path(src.get_path());
		set_subsong(src.get_subsong());
	}

	int compare(const playable_location & src) const;

	const playable_location & operator=(const playable_location & src)
	{
		copy(src);
		return *this;
	}	

	inline bool is_empty() {return get_path()[0]==0 && get_subsong()==0;}
	inline void reset() {set_path("");set_subsong(0);}
	inline t_uint32 get_subsong_index() const {return get_subsong();}
	inline void set_subsong_index(t_uint32 v) {set_subsong(v);}
};

class playable_location_i : public playable_location//implementation
{
	string_simple path;
	t_uint32 subsong;
public:

	virtual const char * get_path() const {return path;}
	virtual void set_path(const char* z) {path=z;}
	virtual t_uint32 get_subsong() const {return subsong;}
	virtual void set_subsong(t_uint32 z) {subsong=z;}

	playable_location_i() {subsong=0;}

	const playable_location_i & operator=(const playable_location & src)
	{
		copy(src);
		return *this;
	}

	playable_location_i(const char * p_path,t_uint32 p_subsong) : path(p_path), subsong(p_subsong) {}

	playable_location_i(const playable_location & src) {copy(src);}
};


// usage: something( make_playable_location("file://c:\blah.ogg",0) );
// only for use as a parameter to a function taking const playable_location &
class make_playable_location : public playable_location
{
	const char * path;
	t_uint32 num;
	
	void set_path(const char*) {assert(0);}
	void set_subsong(t_uint32) {assert(0);}

public:
	const char * get_path() const {return path;}
	t_uint32 get_subsong() const {return num;}

	make_playable_location(const char * p_path,t_uint32 p_num) : path(p_path), num(p_num) {}
};

string_formatter & operator<<(string_formatter & p_fmt,const playable_location & p_location);

#endif //_FOOBAR2000_PLAYABLE_LOCATION_H_