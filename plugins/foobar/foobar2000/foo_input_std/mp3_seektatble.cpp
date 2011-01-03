#include "stdafx.h"
#include "mp3_seektable.h"


void mp3_seektable::on_detach()
{
	detached = true;
	if (references == 0) delete this;
}
unsigned mp3_seektable::get_refcount()
{
	unsigned val;
	owner->lock();
	val = references;
	owner->unlock();
	return val;
}
void mp3_seektable::add_ref()
{
	owner->lock();
	references++;
	owner->unlock();
}
void mp3_seektable::release()
{
	seektable_manager * p_owner = owner;//we might get deleted
	p_owner->lock();
	references--;
	if (references==0) 
	{
		if (detached) delete this;
		else p_owner->on_table_released();
	}
	p_owner->unlock();
}
void mp3_seektable::add_entry(t_int64 frame,t_int64 offset)
{
	owner->lock();
	int num = data.get_count();
	if (num==0 || data[num-1].frame+64<=frame)
		data.add_item(entry(frame,offset));
	owner->unlock();
}
mp3_seektable::mp3_seektable(const char * src,t_filetimestamp p_timestamp,t_int64 counter,class seektable_manager * p_owner) : owner(p_owner), references(0), file(src), last_used(counter), detached(false), m_timestamp(p_timestamp) {}
void mp3_seektable::set_last_used(t_int64 num) {last_used = num;}
t_int64 mp3_seektable::get_last_used() {return last_used;}
bool mp3_seektable::is_our_file(const char * p_file,t_filetimestamp p_timestamp) const {return metadb::path_compare(file,p_file) == 0 && p_timestamp == m_timestamp;}
bool mp3_seektable::is_our_file(const char * p_file) const {return metadb::path_compare(file,p_file) == 0;}

void mp3_seektable::find_entry(t_int64 desired_frame,t_int64 & out_frame,t_int64 & out_offset)
{
	owner->lock();
	unsigned idx = infinite;
	if (data.get_count()>0)
	{
		data.bsearch_t(compare,desired_frame,idx);
		if (idx>=data.get_count()) idx = data.get_count()-1;
		while(idx>0 && data[idx].frame > desired_frame) idx--;
	}
	if (idx == infinite)
	{
		out_frame = 0;
		out_offset = 0;				
	}
	else
	{
		out_frame = data[idx].frame;
		out_offset = data[idx].offset;
	}

#ifdef _DEBUG
	{
		t_int64 out_frame_test,out_offset_test;
		find_entry_slow(desired_frame,out_frame_test,out_offset_test);
		assert(out_frame == out_frame_test && out_offset==out_offset_test);
	}
#endif
	owner->unlock();
}

void mp3_seektable::find_entry_slow(t_int64 desired_frame,t_int64 & out_frame,t_int64 & out_offset)
{
	owner->lock();

	unsigned n;

	for(n=0;n<data.get_count();n++)
	{
		if (data[n].frame > desired_frame) break;
	}
	
	if (n==0)
	{
		out_frame=0;
		out_offset=0;
	}
	else
	{
		n--;
		out_frame = data[n].frame;
		out_offset = data[n].offset;
	}

	owner->unlock();
}


void seektable_manager::find_entry(const char * file,t_filetimestamp p_timestamp,t_int64 desired_frame,t_int64 &out_frame,t_int64 &out_offset)
{
	insync(sync);
	int n = find_table(file,p_timestamp);
	if (n>=0)
	{
		tables[n]->find_entry(desired_frame,out_frame,out_offset);
	}
	else
	{
		out_frame = 0;
		out_offset = 0;
	}
}


seektable_manager::seektable_manager()
{
	usage_counter = 0;
}

seektable_manager::~seektable_manager()
{
	tables.delete_all();
}

void seektable_manager::add_entry(const char * file,t_filetimestamp p_timestamp,t_int64 frame,t_int64 offset)
{
	insync(sync);
	int n = find_table(file,p_timestamp);
	if (n>=0)
	{
		tables[n]->add_entry(frame,offset);
	}
}

mp3_seektable * seektable_manager::make_table(const char * file,t_filetimestamp p_timestamp)
{
	insync(sync);
	{
		int n = find_table(file,p_timestamp);
		if (n>=0)
		{
			mp3_seektable * item = tables[n];
			item->add_ref();
			item->set_last_used(++usage_counter);
			return item;
		}
	}

	remove_old_tables(MAX_TABLES-1);

	mp3_seektable * item = new mp3_seektable(file,p_timestamp,++usage_counter,this);
	tables.add_item(item);
	item->add_ref();
	return item;
}


int seektable_manager::find_table(const char * file,t_filetimestamp p_timestamp)
{
	unsigned n;
	for(n=0;n<tables.get_count();n++)
	{
		if (tables[n]->is_our_file(file,p_timestamp))
		{
			return n;
		}
	}
	return -1;
}


void seektable_manager::remove_old_tables(unsigned max)
{
	while(tables.get_count()>=max)
	{
		unsigned n;
		int found = -1;
		t_int64 val;
		for(n=0;n<tables.get_count();n++)
		{
			if (found<0 || val<tables[n]->get_last_used() && tables[n]->get_refcount()==0)
			{
				found = n;
				val = tables[n]->get_last_used();
			}
		}
		if (found==-1) break;
		tables.delete_by_idx(found);
	}
}


void seektable_manager::remove_table(const char * file)
{//todo
}
