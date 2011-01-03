/*

   Copyright (C) 2003, Chris Moeller,
   All rights reserved.                          

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote 
        products derived from this software without specific prior written 
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

// change log

// 2004-02-29 12:37 UTC - kode54
// - Uh, I shouldn't call services from initializers, like in cfg_var::set_raw_data
// - Version is now 1.0.17

// 2004-02-29 01:41 UTC - kode54
// - Changed history format to playlist item list
// - Version is now 1.0.16

// 2004-02-28 18:29 UTC - kode54
// - Added persistent shuffle history
// - Added history save/reset options to configuration dialog
// - Added menu item for resetting history
// - Version is now 1.0.15

// 2004-01-28 01:35 UTC - kode54
// - Changed to playback_flow_control_v2
// - Version is now 1.0.14

// 2003-11-02 23:35 UTC - kode54
// - Added synchronized deconstructor to shuffle_history class, just in case
// - Added text skipping to tracknumber and disc handler
// - Version is now 1.0.12

// 2003-10-27 21:59 UTC - kode54
// - Fixed error checking for initial album queries used by both album modes
// - Version is now 1.0.11

// 2003-10-19 17:48 UTC - kode54
// - Mucked around a bit with single item handling
// - Added history reduction check to other shuffle modes
// - Fixed stupid mistake with album 2 mode
// - Version is now 1.0.10

// 2003-10-19 16:36 UTC - kode54
// - Album 2 now uses full random for normal track selection instead of jumping
// - Version is now 1.0.9

// 2003-10-19 13:55 UTC - kode54
// - Changed reduce range to 0-99%

// 2003-10-19 13:08 UTC - kode54
// - Album 2 mode now accounts for DISC value
// - Version is now 1.0.8

// 2003-10-19 10:03 UTC - kode54
// - Properly checked for handle_query_locked returning NULL
// - Version is now 1.0.7

// 2003-10-19 02:57 UTC - kode54
// - Added second album mode
// - Version is now 1.0.6

// 2003-10-19 02:26 UTC - kode54
// - Fixed tag set + condition check (stupid typo)
// - Version is now 1.0.5

// 2003-10-18 22:31 UTC - kode54
// - Added tag rule set mode
// - Version is now 1.0.4

// 2003-10-18 07:09 UTC - kode54
// - Added shuffle album mode
// - Version is now 1.0.3

// 2003-10-17 23:21 UTC - kode54
// - Added minimum jump size
// - Version is now 1.0.2

// 2003-10-17 21:18 UTC - kode54
// - Added follow cursor handler
// - Version is now 1.0.1

#include <foobar2000.h>

#include "mt19937ar-cok.h"

#include "resource.h"

static cfg_int cfg_recycle_percent("shuffle_recycle", 54);
static cfg_int cfg_reduce_percent("shuffle_reduce", 42);
static cfg_int cfg_range_percent("shuffle_range", 75);
static cfg_int cfg_range_minimum("shuffle_range_minimum", 20);

static cfg_int cfg_save("shuffle_save_history", 0);

static critical_section sync_tag_set;
static cfg_string cfg_tag_set("shuffle_tag_set", "-album");

static bool g_inited = false;
static bool g_loaded = false;

/*
static critical_section sync_format;
static cfg_string cfg_format("shuffle_format", "0[%album%]");
*/

static mt_rand g_rand;

typedef unsigned int timestamp;
#define TS_UNPLAYED 0
#define TS_MAX 0xFFFFFFFF

static int tardtoi(const char * ptr)
{
	while (*ptr)
	{
		if (isdigit(*ptr))
		{
			return atoi(ptr);
		}
		ptr++;
	}
	return 0;
}

class NOVTABLE shuffle_history
{
public:
	virtual ~shuffle_history() {}

	virtual void on_items_added(int start, int count) = 0;
	virtual void on_order(const int * order, int count) = 0;
	virtual void on_items_removed(const bit_array & mask) = 0;
	virtual void on_replaced(int idx) = 0;
};

class shuffle_history_i_dummy : public shuffle_history
{
public:
	~shuffle_history_i_dummy() {}
	void on_items_added(int start, int count) {}
	void on_order(const int * order, int count) {}
	void on_items_removed(const bit_array & mask) {}
	void on_replaced(int idx) {}
};

class shuffle_history_i : public shuffle_history
{
private:
	critical_section sync;
	mem_block_list_t<timestamp> list;
	unsigned played, stopped;
	timestamp m_ts;

	bool loaded;

public:
	shuffle_history_i()
	{
		played = 0;
		stopped = 0;
		m_ts = TS_UNPLAYED;
		loaded = false;
	}

	~shuffle_history_i()
	{
		insync(sync);
		played = 0;
		list.remove_all();
	}

// stuff for playlist_callback

	void on_items_added(int start, int count)
	{
		insync(sync);
		int count2 = list.get_count();
		list.add_items_repeat(TS_UNPLAYED, count);
		if (count2 == start || !played) return;

		int idx;

		for (idx = start, count = count2; idx < count; idx++, count2++)
		{
			list[count2] = list[idx];
			list[idx] = TS_UNPLAYED;
		}
	}

	void on_order(const int * order, int count)
	{
		insync(sync);
		if (played) list.apply_order(order, count);
	}

	void on_items_removed(const bit_array & mask)
	{
		insync(sync);
		list.remove_mask(mask);
		if (played)
		{
			played = 0;
			unsigned idx, count = list.get_count();
			for (idx = 0; idx < count; idx++)
			{
				if (list[idx] != TS_UNPLAYED) played++;
			}
		}
	}

	void on_replaced(int idx)
	{
		insync(sync);
		if (played)
		{
			if (list[idx] != TS_UNPLAYED)
			{
				list[idx] = TS_UNPLAYED;
				played--;
			}
		}
	}

// for playback_flow_control
	unsigned get_count()
	{
		insync(sync);
		return list.get_count();
	}

	void set_size(unsigned size)
	{
		insync(sync);
		list.remove_all();
		if (size) list.add_items_repeat(TS_UNPLAYED, size);
		played = 0;
		loaded = false;
	}

	void mark_as_played(unsigned idx)
	{
		insync(sync);
		list[idx] = ++m_ts;
		if (m_ts == TS_MAX)
		{
			// bloody hell!
			reduce_timestamps();
		}
		played++;
		unsigned count = list.get_count();
		unsigned percent = cfg_recycle_percent;
		if (percent == 100)
		{
			if (played >= count)
			{
				stopped = 1;
				played = count;
			}
		}
		else if (played > percent * count / 100) 
		{
			reduce_history();
		}
	}

	unsigned find_oldest()
	{
		insync(sync);
		if (!played || stopped == 2 || loaded) return 0xFFFFFFFF;

		unsigned idx, count = list.get_count();
		timestamp ts = TS_MAX;
		unsigned oldest = 0xFFFFFFFF;
		for (idx = 0; idx < count; idx++)
		{
			if (list[idx] == TS_UNPLAYED) continue;
			if (list[idx] < ts)
			{
				ts = list[idx];
				oldest = idx;
			}
		}
		return oldest;
	}

	unsigned find_nearest_unplayed(unsigned idx, int offset)
	{
		if (idx == 0xFFFFFFFF) idx = 0;
		insync(sync);
		if (stopped >= 2)
		{
			reduce_history();
		}

		unsigned count = list.get_count();

		idx += (unsigned) offset;
		if (idx >= count)
		{
			if (idx - count > count) idx += count;
			else idx -= count;
		}
		// exact match
		if (!played || list[idx] == TS_UNPLAYED) return idx;

		unsigned distance1, distance2, idx1, idx2;

		for (distance1 = 0, idx1 = idx; distance1 < count; distance1++)
		{
			idx1++;
			if (idx1 == count) idx1 = 0;
			if (list[idx1] == TS_UNPLAYED) break;
		}

		for (distance2 = 0, idx2 = idx; distance2 < count; distance2++)
		{
			if (idx2 == 0) idx2 = count;
			idx2--;
			if (list[idx2] == TS_UNPLAYED) break;
		}

		if (distance1 < distance2) return idx1;
		else return idx2;
	}

	unsigned jump(unsigned idx, int offset)
	{
		if (!offset)
		{
			loaded = false;
			return idx;
		}

		insync(sync);

		timestamp check;

		if ((loaded || stopped == 2) && idx == 0xFFFFFFFF && offset < 0)
		{
			if (loaded) loaded = false;
			if (stopped == 2) stopped = 3;
			check = TS_MAX;
		}
		else
		{
			if (idx == 0xFFFFFFFF) return idx;

			check = list[idx];
			if (check == TS_UNPLAYED)
			{
				if (offset < 0) check = TS_MAX;
				else return 0xFFFFFFFF;
			}
		}

		unsigned rv, count = list.get_count();
		while (offset > 0)
		{
			timestamp ts = TS_MAX;
			for (idx = 0; idx < count; idx++)
			{
				if (list[idx] <= check) continue;
				if (list[idx] < ts)
				{
					ts = list[idx];
					rv = idx;
				}
			}
			if (ts == TS_MAX)
			{
				if (stopped == 1 || stopped == 3)
				{
					stopped = 2;
					return 0xFFFFFFFE;
				}
				else
					return 0xFFFFFFFF;
			}
			check = ts;
			offset--;
		}
		while (offset < 0)
		{
			timestamp ts = TS_UNPLAYED;
			for (idx = 0; idx < count; idx++)
			{
				if (list[idx] >= check) continue;
				if (list[idx] > ts)
				{
					ts = list[idx];
					rv = idx;
				}
			}
			if (ts == TS_UNPLAYED) return 0xFFFFFFFF;
			check = ts;
			offset++;
		}
		return rv;
	}

	void find_album(unsigned idx, mem_block_list_t<unsigned> & out)
	{
		string8 album;
		const file_info * info;
		playlist_oper * po = playlist_oper::get();
		metadb_handle * handle = po->get_item(idx);
		if (!handle) return;
		handle->handle_lock();
		info = handle->handle_query_locked();
		if (info)
		{
			album = info->meta_get("ALBUM");
		}
		handle->handle_unlock();
		handle->handle_release();
		if (album.is_empty()) return;

		insync(sync);
		unsigned n, count = po->get_count();
		for (n = 0; n < count; n++)
		{
			if (n == idx) continue;
			if (list[n] != TS_UNPLAYED) continue;
			handle = po->get_item(n);
			if (!handle) continue;
			handle->handle_lock();
			info = handle->handle_query_locked();
			if (info)
			{
				const char * ptr = info->meta_get("ALBUM");
				if (ptr && !stricmp_utf8(album, ptr))
				{
					out.add_item(n);
				}
			}
			handle->handle_unlock();
			handle->handle_release();
		}
	}

	void find_unplayed(mem_block_list_t<unsigned> & out)
	{
		insync(sync);
		unsigned n, count = list.get_count();
		for (n = 0; n < count; n++)
		{
			if (list[n] == TS_UNPLAYED) out.add_item(n);
		}
	}

	void find_tag_set(unsigned idx, const char * tag_set, mem_block_list_t<unsigned> & out)
	{
		ptr_list_t<string8> tags;
		ptr_list_t<string8> check;
		string8 temp;
		temp.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
		playlist_oper * po = playlist_oper::get();
		metadb_handle * handle = po->get_item(idx);
		if (!handle) return;
		{
			string8 tagset(tag_set);
			handle->handle_lock();
			const file_info * info = handle->handle_query_locked();
			if (info)
			{
				int s = 0, e;
				while ((e = tagset.find_first('|', s)) >= 0)
				{
					temp.set_string(tagset.get_ptr() + s, e - s);
					s = e + 1;
					if (temp.length() < 2) continue;
					const char * ptr = info->meta_get(temp.get_ptr() + 1);
					if (ptr && *ptr)
					{
						tags.add_item(new string8(temp));
						check.add_item(new string8(ptr));
					}
				}
				if (strlen(tagset.get_ptr() + s))
				{
					temp.set_string(tagset.get_ptr() + s);
					if (temp.length() > 1)
					{
						const char * ptr = info->meta_get(temp.get_ptr() + 1);
						if (ptr && *ptr)
						{
							tags.add_item(new string8(temp));
							check.add_item(new string8(ptr));
						}
					}
				}
			}
			handle->handle_unlock();
			handle->handle_release();
		}
		if (!tags.get_count()) return;

		insync(sync);
		unsigned n, count = po->get_count(), t, tcount = tags.get_count(), match;
		for (n = 0; n < count; n++)
		{
			if (n == idx) continue;
			if (list[n] != TS_UNPLAYED) continue;
			handle = po->get_item(n);
			if (!handle) continue;

			handle->handle_lock();
			const file_info * info = handle->handle_query_locked();

			if (info)
			{
				for (t = 0, match = 0; t < tcount; t++)
				{
					string8 * tag = tags.get_item(t);
					string8 * chk = check.get_item(t);
					const char * ptr = info->meta_get(tag->get_ptr() + 1);
					switch (*(tag->get_ptr()))
					{
					case '-':
						if (ptr && *ptr)
						{
							if (stricmp_utf8(ptr, chk->get_ptr())) match++;
							else t = tcount;
						}
						else
							match++;
						break;
					case '+':
						if (ptr && !stricmp_utf8(ptr, chk->get_ptr())) match++;
						else t = tcount;
						break;
					}
				}
			}
			handle->handle_unlock();
			handle->handle_release();

			if (match == tcount) out.add_item(n);
		}
		tags.delete_all();
		check.delete_all();
	}

	unsigned find_first_unplayed_track(unsigned idx)
	{
		string8 album;
		const file_info * info;
		playlist_oper * po = playlist_oper::get();
		metadb_handle * handle = po->get_item(idx);
		if (!handle) return 0xFFFFFFFF;
		handle->handle_lock();
		info = handle->handle_query_locked();
		if (info)
		{
			album = info->meta_get("ALBUM");
		}
		handle->handle_unlock();
		handle->handle_release();
		if (album.is_empty()) return 0xFFFFFFFF;

		unsigned rv = 0xFFFFFFFF, tn, tracknumber = 0xFFFFFFFF;

		insync(sync);
		unsigned n, count = po->get_count();
		for (n = 0; n < count; n++)
		{
			if (list[n] != TS_UNPLAYED) continue;
			handle = po->get_item(n);
			if (!handle) continue;
			handle->handle_lock();
			info = handle->handle_query_locked();
			if (info)
			{
				const char * ptr = info->meta_get("ALBUM");
				if (ptr && !stricmp_utf8(album, ptr))
				{
					ptr = info->meta_get("TRACKNUMBER");
					if (ptr)
					{
						tn = tardtoi(ptr);
						ptr = info->meta_get("DISC");
						if (ptr)
						{
							tn += tardtoi(ptr) * 1024;
						}
						if (tn < tracknumber)
						{
							tracknumber = tn;
							rv = n;
						}
					}
				}
			}
			handle->handle_unlock();
			handle->handle_release();
		}
		return rv;
	}

/*
	void find_format_set(unsigned idx, const char * format, mem_block_list_t<unsigned> & out)
	{
		ptr_list_t<string8> ffmt;
		ptr_list_t<string8> check;
		string8 temp;
		temp.set_mem_logic(mem_block::ALLOC_FAST_DONTGODOWN);
		playlist_oper * po = playlist_oper::get();
		metadb_handle * handle = po->get_item(idx);
		if (!handle) return;
		{
			string8 fmt(format);
			int s = 0, e;
			while ((e = fmt.find_first('|', s)) >= 0)
			{
				temp.set_string(fmt.get_ptr() + s, e - s);
				s = e + 1;
				if (temp.length() < 2) continue;
				string8 * fld = new string8;
				handle->handle_format_title(*fld, temp.get_ptr() + 1, 0);
				if (fld->is_empty()) delete fld;
				else
				{
					ffmt.add_item(new string8(temp));
					check.add_item(fld);
				}
			}
			if (strlen(fmt.get_ptr() + s))
			{
				temp.set_string(fmt.get_ptr() + s);
				if (temp.length() > 1)
				{
					string8 * fld = new string8;
					handle->handle_format_title(*fld, temp.get_ptr() + 1, 0);
					if (fld->is_empty()) delete fld;
					else
					{
						ffmt.add_item(new string8(temp));
						check.add_item(fld);
					}
				}
			}
			handle->handle_release();
		}
		if (!ffmt.get_count()) return;

		insync(sync);
		unsigned n, count = po->get_count(), f, fcount = ffmt.get_count(), match;
		for (n = 0; n < count; n++)
		{
			if (n == idx) continue;
			if (list[n] != TS_UNPLAYED) continue;
			handle = po->get_item(n);
			if (!handle) continue;

			for (f = 0, match = 0; f < fcount; f++)
			{
				string8 * fmt = ffmt.get_item(f);
				string8 * chk = check.get_item(f);
				handle->handle_format_title(temp, fmt->get_ptr() + 1, 0);
				switch (*(fmt->get_ptr()))
				{
				case '0':
					if (temp.length())
					{
						if (stricmp_utf8(temp, chk->get_ptr())) match++;
						else f = fcount;
					}
					else
						match++;
					break;
				case '1':
					if (temp.length() && !stricmp_utf8(temp, chk->get_ptr())) match++;
					else f = fcount;
					break;
				}
			}
			handle->handle_release();

			if (match == fcount) out.add_item(n);
		}
		ffmt.delete_all();
		check.delete_all();
	}
*/

	void check_reduce()
	{
		insync(sync);
		if (stopped >= 2)
		{
			reduce_history();
		}
	}

private:
	void reduce_timestamps()
	{
		unsigned idx = find_oldest();
		timestamp ts = list[idx] - 1;
		unsigned count = list.get_count();
		m_ts = TS_UNPLAYED;
		for (idx = 0; idx < count; idx++)
		{
			if (list[idx] == TS_UNPLAYED) continue;
			list[idx] -= ts;
			if (list[idx] > m_ts) m_ts = list[idx];
		}
	}

	void reduce_history()
	{
		unsigned count = list.get_count();
		unsigned percent = cfg_recycle_percent;
		if (percent == 100) percent = cfg_reduce_percent;
		stopped = 0;
		if (!percent)
		{
			set_size(count);
			played = 0;
		}
		else
		{
			percent = percent * count / 100;
			while (played > percent)
			{
				unsigned idx = find_oldest();
				if (idx == 0xFFFFFFFF) break;
				list[idx] = TS_UNPLAYED;
				played--;
			}
		}
	}
public:
	void get_raw_data(cfg_var::write_config_callback * out, unsigned ver)
	{
		insync(sync);
		loaded = false;
		unsigned i, n = list.get_count();
		if (ver == 1)
		{
			out->write_dword_le(n);
			out->write_dword_le(played);
		}
		else if (ver == 2)
		{
			out->write_dword_var(n);
			out->write_dword_var(played);
		}
		if (played)
		{
			if (ver == 1)
			{
				out->write_dword_le(stopped);
			}
			else if (ver == 2)
			{
				out->write_dword_var(stopped);
			}
			reduce_timestamps();
			mem_block_t<unsigned> ts_block;
			unsigned my_ts = m_ts;
			unsigned * ts = (my_ts * sizeof(unsigned) <= PFC_ALLOCA_LIMIT) ? (unsigned *) alloca(my_ts * sizeof(unsigned)) : ts_block.set_size(my_ts);
			memset(ts, 255, my_ts * sizeof(unsigned));
			for (i = 0; i < n; i++)
			{
				if (list[i]) ts[list[i] - 1] = i;
			}
			if (my_ts > played)
			{
				// gap killer!
				for (i = 0; i < my_ts;)
				{
					if (ts[i] == 0xFFFFFFFF)
					{
						if (i + 1 < my_ts)
						{
							ts[i] = ts[i + 1];
						}
						my_ts--;
						continue;
					}
					i++;
				}
			}
			if (ver == 1)
			{
				for (i = 0; i < my_ts; i++)
				{
					out->write_dword_le(ts[i]);
				}
			}
			else if (ver == 2)
			{
				for (i = 0; i < my_ts; i++)
				{
					out->write_dword_var(ts[i]);
				}
			}
		}
	}

	bool set_raw_data(cfg_var::read_config_helper & r, unsigned ver)
	{
		insync(sync);
		list.remove_all();
		played = 0;
		stopped = 0;
		m_ts = TS_UNPLAYED;
		DWORD i, count, p_played, p_stopped;
		if (ver == 1)
		{
			if (r.get_remaining() < 8) return false;
			if (!r.read_dword_le(count)) return false;
			if (!r.read_dword_le(p_played)) return false;
		}
		else if (ver == 2)
		{
			if (!r.read_dword_var(count)) return false;
			if (!r.read_dword_var(p_played)) return false;
		}
		list.add_items_repeat(TS_UNPLAYED, count);
		played = p_played;
		if (played)
		{
			if (ver == 1)
			{
				if (!r.read_dword_le(p_stopped)) goto fail;
				if (r.get_remaining() < played * sizeof(unsigned)) goto fail;
				for (i = 0; i < played; i++)
				{
					DWORD item;
					if (!r.read_dword_le(item)) goto fail;
					if (item >= count) goto fail;
					list[item] = ++m_ts;
				}
			}
			else if (ver == 2)
			{
				if (!r.read_dword_var(p_stopped)) goto fail;
				for (i = 0; i < played; i++)
				{
					DWORD item;
					if (!r.read_dword_var(item)) goto fail;
					if (item >= count) goto fail;
					list[item] = ++m_ts;
				}
			}
			stopped = p_stopped;
			loaded = true;
		}
		return true;

fail:
		list.remove_all();
		played = 0;
		stopped = 0;
		return false;
	}
};

class shuffle_history_list
{
private:
	critical_section sync;
	bool use_dummy;
	shuffle_history_i_dummy dummy;
	ptr_list_t<shuffle_history_i> history;

public:
	shuffle_history_list()
	{
		use_dummy = true;
	}

	~shuffle_history_list()
	{
		history.delete_all();
	}

	void on_playlist_switch_before()
	{
		insync(sync);
		use_dummy = true;
	}

	void on_playlist_switch_after(unsigned from, unsigned to, unsigned count)
	{
		insync(sync);
		use_dummy = false;
		unsigned my_count = history.get_count();
		while (my_count < count)
		{
			history.add_item(new shuffle_history_i);
			my_count++;
		}
	}

	void on_reorder(const int * order, unsigned count)
	{
		insync(sync);
		history.apply_order(order, count);
	}

	void on_new_playlist(unsigned idx)
	{
		insync(sync);
		history.insert_item(new shuffle_history_i, idx);
	}

	void on_delete_playlist(unsigned idx)
	{
		insync(sync);
		history.delete_by_idx(idx);
	}

	void on_item_replaced(unsigned pls, unsigned item)
	{
		insync(sync);
		shuffle_history * sh = history[pls];
		sh->on_replaced(item);
	}

	shuffle_history * get_ptr(unsigned idx)
	{
		insync(sync);

		unsigned my_count = history.get_count(), count = idx + 1;
		while (my_count < count)
		{
			history.add_item(new shuffle_history_i);
			my_count++;
		}
		return history[idx];
	}

	inline shuffle_history * operator -> ()
	{
		if (use_dummy) return &dummy;
		return get_ptr(playlist_switcher::get()->get_active_playlist());
	}

	inline shuffle_history_i * operator [](unsigned n)
	{
		return static_cast<shuffle_history_i *>(get_ptr(n));
	}

	bool get_raw_data(cfg_var::write_config_callback * out, unsigned ver)
	{
		insync(sync);

		unsigned i, n = history.get_count();

		if (!n) return false;

		if (ver == 1)
			out->write_dword_le(n);
		else if (ver == 2)
			out->write_dword_var(n);

		for (i = 0; i < n; i++)
		{
			history[i]->get_raw_data(out, ver);
		}

		return true;
	}

	void set_raw_data(cfg_var::read_config_helper & r, unsigned ver)
	{
		insync(sync);

		DWORD count;
		if (ver == 1)
		{
			if (r.get_remaining() < 4) return;
			if (!r.read_dword_le(count)) return;
		}
		else if (ver == 2)
		{
			if (!r.read_dword_var(count)) return;
		}
		history.delete_all();
		unsigned i;
		for (i = 0; i < count; i++)
		{
			shuffle_history_i * sh = new shuffle_history_i;
			if (sh->set_raw_data(r, ver))
			{
				history.add_item(sh);
			}
			else
			{
				// corrupt configuration, reset
				history.delete_all();
				return;
			}
		}
		g_loaded = true;
	}

	void reset()
	{
		insync(sync);
		playlist_switcher * ps = playlist_switcher::get();
		if (!g_loaded)
		{
			history.delete_all();
			unsigned i, count = ps->get_num_playlists();
			for (i = 0; i < count; i++)
			{
				history.add_item(new shuffle_history_i);
			}
		}
	}
};

static shuffle_history_list g_shuffle_history;

class cfg_history_v2 : public cfg_var
{
private:
	virtual bool get_raw_data(write_config_callback * out)
	{
		return false;
	}

	virtual void set_raw_data(const void * data, int size)
	{
		read_config_helper r(data, size);
		g_shuffle_history.set_raw_data(r, 1);
	}
public:
	explicit inline cfg_history_v2(const char * name) : cfg_var(name) {}
};

class cfg_history_v3 : public cfg_var
{
private:
	virtual bool get_raw_data(write_config_callback * out)
	{
		if (cfg_save)
		{
			return g_shuffle_history.get_raw_data(out, 2);
		}
		else
		{
			return false;
		}
	}

	virtual void set_raw_data(const void * data, int size)
	{
		read_config_helper r(data, size);
		g_shuffle_history.set_raw_data(r, 2);
	}
public:
	explicit inline cfg_history_v3(const char * name) : cfg_var(name) {}
};

static cfg_history_v2 cfg_save_history_v2("shuffle_history_v2");
static cfg_history_v3 cfg_save_history_v3("shuffle_history_v3");

class shuffle_flow_control : public playback_flow_control_v2
{
public:
	virtual const char * get_name()
	{
		return "Shuffle";
	}

	virtual int get_next(int previous_index, int focus_item, int total, int advance, bool follow_focus, bool user_advance, unsigned playlist)
	{
		if (g_shuffle_history[playlist]->get_count() != (unsigned)total)
		{
			g_shuffle_history[playlist]->set_size(total);
		}

		if (total < 2)
		{
			if (previous_index == -1 || (previous_index == 0 && advance == 0)) return 0;
			else return -1;
		}

		if (previous_index < 0 && advance >= 0)
		{
			previous_index = (int)g_shuffle_history[playlist]->find_oldest();
			if (advance > 0) advance--;
		}
		unsigned idx = g_shuffle_history[playlist]->jump((unsigned)previous_index, advance);

		if (idx == 0xFFFFFFFE && user_advance) idx = 0xFFFFFFFF;

		if (idx == 0xFFFFFFFF && advance >= 0)
		{
			if (follow_focus && focus_item != previous_index)
			{
				idx = focus_item;
			}
			else
			{
				double offset;
				double percent = (double)(int)cfg_range_percent * .005; // radius
				double minimum = (double)(int)cfg_range_minimum * percent * .01;

				percent -= minimum;

				minimum *= (double)total;
		
				offset = ((g_rand.genrand_real1() * 2. - 1.) * (double)total * percent);

				if (offset < 0.) offset -= minimum;
				else offset += minimum;

				idx = g_shuffle_history[playlist]->find_nearest_unplayed((unsigned)previous_index, (int)offset);
			}
			
			if (idx != 0xFFFFFFFF) g_shuffle_history[playlist]->mark_as_played(idx);
		}

		if (idx == 0xFFFFFFFE) idx = 0xFFFFFFFF;

		return (int) idx;
	}
};

class shuffle_album_flow_control : public playback_flow_control_v2
{
public:
	virtual const char * get_name()
	{
		return "Shuffle album";
	}

	virtual int get_next(int previous_index, int focus_item, int total, int advance, bool follow_focus, bool user_advance, unsigned playlist)
	{
		if (g_shuffle_history[playlist]->get_count() != (unsigned)total)
		{
			g_shuffle_history[playlist]->set_size(total);
		}

		if (total < 2)
		{
			if (previous_index == -1 || (previous_index == 0 && advance == 0)) return 0;
			else return -1;
		}

		if (previous_index < 0 && advance >= 0)
		{
			previous_index = (int)g_shuffle_history[playlist]->find_oldest();
			if (advance > 0) advance--;
		}
		unsigned idx = g_shuffle_history[playlist]->jump((unsigned)previous_index, advance);

		if (idx == 0xFFFFFFFE && user_advance) idx = 0xFFFFFFFF;

		if (idx == 0xFFFFFFFF && advance >= 0)
		{
			if (follow_focus && focus_item != previous_index)
			{
				idx = focus_item;
			}
			else
			{
				mem_block_list_t<unsigned> album;

				g_shuffle_history[playlist]->find_album((unsigned)previous_index, album);

				if (album.get_count())
				{
					total = album.get_count();

					idx = (unsigned)(g_rand.genrand_real1() * (double)(total - 1));
					idx = album[idx];
					g_shuffle_history[playlist]->check_reduce();
				}
				else
				{
					double offset;					
					double percent = (double)(int)cfg_range_percent * .005; // radius
					double minimum = (double)(int)cfg_range_minimum * percent * .01;
					percent -= minimum;
					minimum *= (double)total;
		
					offset = ((g_rand.genrand_real1() * 2. - 1.) * (double)total * percent);

					if (offset < 0.) offset -= minimum;
					else offset += minimum;

					idx = g_shuffle_history[playlist]->find_nearest_unplayed((unsigned)previous_index, (int)offset);
				}
			}
			
			if (idx != 0xFFFFFFFF) g_shuffle_history[playlist]->mark_as_played(idx);
		}

		if (idx == 0xFFFFFFFE) idx = 0xFFFFFFFF;

		return (int) idx;
	}
};

class shuffle_album2_flow_control : public playback_flow_control_v2
{
public:
	virtual const char * get_name()
	{
		return "Shuffle album 2";
	}

	virtual int get_next(int previous_index, int focus_item, int total, int advance, bool follow_focus, bool user_advance, unsigned playlist)
	{
		if (g_shuffle_history[playlist]->get_count() != (unsigned)total)
		{
			g_shuffle_history[playlist]->set_size(total);
		}

		if (total < 2)
		{
			if (previous_index == -1 || (previous_index == 0 && advance == 0)) return 0;
			else return -1;
		}

		if (previous_index < 0 && advance >= 0)
		{
			previous_index = (int)g_shuffle_history[playlist]->find_oldest();
			if (advance > 0) advance--;
		}
		unsigned idx = g_shuffle_history[playlist]->jump((unsigned)previous_index, advance);

		if (idx == 0xFFFFFFFE && user_advance) idx = 0xFFFFFFFF;

		if (idx == 0xFFFFFFFF && advance >= 0)
		{
			if (follow_focus && focus_item != previous_index)
			{
				idx = focus_item;
			}
			else
			{
				if (previous_index >= 0) idx = g_shuffle_history[playlist]->find_first_unplayed_track((unsigned)previous_index);
				else idx = 0xFFFFFFFF;

				if (idx == 0xFFFFFFFF)
				{
					mem_block_list_t<unsigned> set;

					g_shuffle_history[playlist]->find_unplayed(set);

					if (set.get_count())
					{
						total = set.get_count();

						idx = (unsigned)(g_rand.genrand_real1() * (double)(total - 1));
						idx = set[idx];
						unsigned next = g_shuffle_history[playlist]->find_first_unplayed_track(idx);
						if (next != 0xFFFFFFFF) idx = next;
						g_shuffle_history[playlist]->check_reduce();
					}
				}
			}
			
			if (idx != 0xFFFFFFFF) g_shuffle_history[playlist]->mark_as_played(idx);
		}

		if (idx == 0xFFFFFFFE) idx = 0xFFFFFFFF;

		return (int) idx;
	}
};

class shuffle_tagset_flow_control : public playback_flow_control_v2
{
public:
	virtual const char * get_name()
	{
		return "Shuffle tag set";
	}

	virtual int get_next(int previous_index, int focus_item, int total, int advance, bool follow_focus, bool user_advance, unsigned playlist)
	{
		if (g_shuffle_history[playlist]->get_count() != (unsigned)total)
		{
			g_shuffle_history[playlist]->set_size(total);
		}

		if (total < 2)
		{
			if (previous_index == -1 || (previous_index == 0 && advance == 0)) return 0;
			else return -1;
		}

		if (previous_index < 0 && advance >= 0)
		{
			previous_index = (int)g_shuffle_history[playlist]->find_oldest();
			if (advance > 0) advance--;
		}
		unsigned idx = g_shuffle_history[playlist]->jump((unsigned)previous_index, advance);

		if (idx == 0xFFFFFFFE && user_advance) idx = 0xFFFFFFFF;

		if (idx == 0xFFFFFFFF && advance >= 0)
		{
			if (follow_focus && focus_item != previous_index)
			{
				idx = focus_item;
			}
			else
			{
				mem_block_list_t<unsigned> set;

				sync_tag_set.enter();
				g_shuffle_history[playlist]->find_tag_set((unsigned)previous_index, cfg_tag_set, set);
				sync_tag_set.leave();

				if (set.get_count())
				{
					total = set.get_count();

					idx = (unsigned)(g_rand.genrand_real1() * (double)(total - 1));
					idx = set[idx];
					g_shuffle_history[playlist]->check_reduce();
				}
				else
				{
					double offset;					
					double percent = (double)(int)cfg_range_percent * .005; // radius
					double minimum = (double)(int)cfg_range_minimum * percent * .01;
					percent -= minimum;
					minimum *= (double)total;
		
					offset = ((g_rand.genrand_real1() * 2. - 1.) * (double)total * percent);

					if (offset < 0.) offset -= minimum;
					else offset += minimum;

					idx = g_shuffle_history[playlist]->find_nearest_unplayed((unsigned)previous_index, (int)offset);
				}
			}
			
			if (idx != 0xFFFFFFFF) g_shuffle_history[playlist]->mark_as_played(idx);
		}

		if (idx == 0xFFFFFFFE) idx = 0xFFFFFFFF;

		return (int) idx;
	}
};

class shuffle_playlist_callback : public playlist_callback
{
public:
	virtual void on_items_removing(const bit_array & mask) {}
	virtual void on_sel_change(int idx, bool state) {}
	virtual void on_sel_change_multi(const bit_array & before, const bit_array & after, int count) {}
	virtual void on_focus_change(int from, int to) {}
	virtual void on_modified(int idx) {}

	virtual void on_items_added(int start, int count)
	{
		if (g_inited) g_shuffle_history->on_items_added(start, count);
	}

	virtual void on_order(const int * order, int count)
	{
		if (g_inited) g_shuffle_history->on_order(order, count);
	}

	virtual void on_items_removed(const bit_array & mask)
	{
		if (g_inited) g_shuffle_history->on_items_removed(mask);
	}

	virtual void on_replaced(int idx)
	{
		if (g_inited) g_shuffle_history->on_replaced(idx);
	}
};

class shuffle_playlist_switcher_callback : public playlist_switcher_callback
{
public:
	virtual void on_rename_playlist(unsigned idx, const char * new_name) {}

	virtual void on_playlist_switch_before(unsigned from, unsigned to)
	{
		if (g_inited) g_shuffle_history.on_playlist_switch_before();
	}

	virtual void on_playlist_switch_after(unsigned from, unsigned to)
	{
		if (g_inited) g_shuffle_history.on_playlist_switch_after(from, to, playlist_switcher::get()->get_num_playlists());
	}

	virtual void on_reorder(const int * order, unsigned count)
	{
		if (g_inited) g_shuffle_history.on_reorder(order, count);
	}

	virtual void on_new_playlist(const char * name, unsigned int idx, const ptr_list_interface<metadb_handle> & data)
	{
		if (g_inited) g_shuffle_history.on_new_playlist(idx);
	}

	virtual void on_delete_playlist(unsigned idx)
	{
		if (g_inited) g_shuffle_history.on_delete_playlist(idx);
	}

	virtual void on_item_replaced(unsigned pls, unsigned item, metadb_handle * from, metadb_handle * to)
	{
		if (g_inited) g_shuffle_history.on_item_replaced(pls, item);
	}
};

static void set_recycle(HWND wnd, int percent)
{
	EnableWindow(GetDlgItem(wnd, IDC_REDUCETO), percent == 100);
	EnableWindow(GetDlgItem(wnd, IDC_REDUCE), percent == 100);
	cfg_recycle_percent = percent;
}

static BOOL CALLBACK ConfigProc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		{
			uSendDlgItemMessage(wnd, IDC_HISTORY_SAVE, BM_SETCHECK, cfg_save, 0);
			HWND w = GetDlgItem(wnd, IDC_RECYCLE);
			uSendMessage(w,TBM_SETRANGE,0,MAKELONG(1,100));
			uSendMessage(w,TBM_SETPOS,1,cfg_recycle_percent);
			set_recycle(wnd, cfg_recycle_percent);
			w = GetDlgItem(wnd, IDC_REDUCE);
			uSendMessage(w,TBM_SETRANGE,0,MAKELONG(0,99));
			uSendMessage(w,TBM_SETPOS,1,cfg_reduce_percent);
			w = GetDlgItem(wnd, IDC_RANGE);
			uSendMessage(w,TBM_SETRANGE,0,MAKELONG(1,100));
			uSendMessage(w,TBM_SETPOS,1,cfg_range_percent);
			w = GetDlgItem(wnd, IDC_MINIMUM);
			uSendMessage(w,TBM_SETRANGE,0,MAKELONG(0,100));
			uSendMessage(w,TBM_SETPOS,1,cfg_range_minimum);
			uSetDlgItemText(wnd, IDC_TAGSET, cfg_tag_set);
		}
		return 1;
	case WM_COMMAND:
		switch (wp)
		{
		case IDC_HISTORY_SAVE:
			cfg_save = uSendMessage((HWND)lp, BM_GETCHECK, 0, 0);
			break;
		case IDC_HISTORY_RESET:
			g_shuffle_history.reset();
			break;
		case (EN_CHANGE << 16) | IDC_TAGSET:
			{
				insync(sync_tag_set);
				cfg_tag_set = string_utf8_from_window((HWND)lp);
			}
			break;
		}
	case WM_HSCROLL:
		switch (uGetWindowLong((HWND)lp,GWL_ID))
		{
		case IDC_RECYCLE:
			set_recycle(wnd, uSendMessage((HWND)lp, TBM_GETPOS, 0, 0));
			break;
		case IDC_REDUCE:
			cfg_reduce_percent = uSendMessage((HWND)lp, TBM_GETPOS, 0, 0);
			break;
		case IDC_RANGE:
			cfg_range_percent = uSendMessage((HWND)lp, TBM_GETPOS, 0, 0);
			break;
		case IDC_MINIMUM:
			cfg_range_minimum = uSendMessage((HWND)lp, TBM_GETPOS, 0, 0);
			break;
		}
		break;
	}
	return 0;
}

class shuffle_config : public config
{
public:
	virtual HWND create(HWND parent)
	{
		return uCreateDialog(IDD_CONFIG, parent, ConfigProc);
	}
	virtual const char * get_name() {return "Shuffle control";}
	virtual const char * get_parent_name() {return "Components";}
};

class initquit_shuffle : public initquit
{
	virtual void on_init()
	{
		g_inited = true;
		g_shuffle_history.reset();
	}
	virtual void on_quit()
	{
		g_inited = false;
	}
};

class menu_shuffle : public menu_item
{
	virtual type get_type()
	{
		return TYPE_MAIN;
	}

	virtual unsigned get_num_items()
	{
		return 1;
	}

	virtual void enum_item(unsigned n, string_base & out)
	{
		out.set_string("Components/Reset shuffle history");
	}

	virtual enabled_state get_enabled_state(unsigned idx)
	{
		return DEFAULT_OFF;
	}

	virtual bool get_display_data(unsigned n, const ptr_list_base<metadb_handle> &data, string_base &out, unsigned & displayflags)
	{
		out.set_string("Reset shuffle history");
		return true;
	}

	virtual void perform_command(unsigned n, const ptr_list_base<metadb_handle> &data)
	{
		g_shuffle_history.reset();
	}
};

static playback_flow_control_factory<shuffle_flow_control> foo;
static playback_flow_control_factory<shuffle_album_flow_control> foo2;
static playback_flow_control_factory<shuffle_album2_flow_control> foo3;
static playback_flow_control_factory<shuffle_tagset_flow_control> foo4;
static service_factory_single_t<playlist_callback,shuffle_playlist_callback> foo5;
static playlist_switcher_callback_factory<shuffle_playlist_switcher_callback> foo6;
static menu_item_factory<menu_shuffle> foo7;
static service_factory_single_t<config,shuffle_config> foo8;
static initquit_factory<initquit_shuffle> foo9;

DECLARE_COMPONENT_VERSION("Shuffle control","1.0.17","Shuffle control with play history.");
