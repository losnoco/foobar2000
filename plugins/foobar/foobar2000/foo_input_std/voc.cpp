// changelog

// 2003-04-29 21:55 - kode54
// - Fixed block type 9 handler in get_samples_pcm()
// - Fixed block type 8 handlers

#include "stdafx.h"

class voc : public input
{
private:
	

	t_int64 offset, loopstart;
	int blocklength, loopcount, srate, bps, nch;
	int tc, packtype, lastblocktype, no_loop, silence;
	double length;
	service_ptr_t<file> m_reader;
	mem_block_t<char> buffer;

	t_io_result voc_parse(const service_ptr_t<file> & r,file_info & info,abort_callback & p_abort)
	{
		try {
			bool done;

			r->seek_e(20,p_abort);
			offset = 0;
			r->read_object_e(&offset, 2,p_abort);
			r->seek_e(offset,p_abort);
			loopcount = 0;
			loopstart = 0;

			done = FALSE;

			length = 0.;

			tc = 0;
			packtype = 0;
			lastblocktype = 0;

			nch = 1;

			int blocktype;

			while (!done)
			{
				blocktype = 0;
				blocklength = 0;
				
				r->read_object_e(&blocktype, 1,p_abort);
				r->read_object_e(&blocklength, 3,p_abort);
				
				offset += 4;
				
				switch (blocktype)
				{
				case 0:
					// End of data
					done = TRUE;
					break;
				case 1:
					// Sound data block
					bps = 8;
					if (lastblocktype != 8)
					{
						tc = 0;
						r->read_object_e(((char*)&tc)+1, 1,p_abort);
						r->read_object_e(&packtype, 1,p_abort);
					}
					offset += 2;
					r->seek_e(offset,p_abort);
					blocklength -= 2;
					
					srate = 256000000L / (65536 - tc);

					if (nch > 1) srate /= nch;
					
					if (packtype == 0)
						length += (double)blocklength / (double)(srate * (bps/8) * nch);
					break;
				case 2:
					// Sound continuation block
					length += (double)blocklength / (double)(srate * (bps/8) * nch);
					break;
				case 3:
					// Silence
					{
						int period = 0, thistc = 0, thisrate;
						r->read_object_e(&period, 2,p_abort);
						r->read_object_e(&thistc, 1,p_abort);
						offset += 3;
						blocklength -= 3;
						
						thisrate = 1000000L / (256 - thistc);
						length += (double)(period + 1) / (double)thisrate;
					}
					break;
	/*
				case 4:
					// Marker
					break;
	*/
				case 5:
					// ASCII string
					{
						char * blah;
						blah = new char[blocklength+1];
						r->read_object_e(blah, blocklength,p_abort);
						blah[blocklength] = 0;
						offset += blocklength;
						blocklength = 0;
						{
							unsigned index = info.meta_find("comment");
							if (index!=infinite) info.meta_add_value(index,blah);
							else info.meta_set("comment",blah);
						}
						delete [] blah;
					}
					break;
				case 6:
					// Repeat begin
					if (!loopstart)
					{
						r->read_object_e(&loopcount, 2, p_abort);
						offset += 2;
						blocklength -= 2;
						loopstart = offset + blocklength;
					}
					break;
				case 7:
					// Repeat end
					if (lastblocktype == 6)
					{
						loopcount = 0;
					}
					else
					{
						if ((loopcount > 0) && loopstart)
						{
							r->seek_e(offset = loopstart , p_abort);
							if (loopcount < 0xffff)
							{
								loopcount--;
								if (loopcount == 0)
								{
									loopstart = 0;
								}
							}
							else
							{
								done = TRUE;
							}
						}
					}
					break;
				case 8:
					// Extended block
					{
						int voicemode = 0;
						bps = 8;
						r->read_object_e(&tc, 2, p_abort);
						r->read_object_e(&packtype, 1, p_abort);
						r->read_object_e(&voicemode, 1, p_abort);
						offset += 4;
						blocklength -= 4;
						if (voicemode == 0) nch = 1;
						else if (voicemode == 1) nch = 2;
						else packtype = -1;
					}
					break;
				case 9:
					// New sound data block
					{
						int format = 0;
						bps = 0;
						r->read_object_e(&srate, 4, p_abort);
						r->read_object_e(&bps, 1, p_abort);
						r->read_object_e(&nch, 1, p_abort);
						r->read_object_e(&format, 2, p_abort);
						offset += 12;
						blocklength -= 12;
						
						if (format == 0 || format == 4)
						{
							length += (double)blocklength / (double)(srate * (bps/8) * nch);
						}
					}
					break;
				}
				
				lastblocktype = blocktype;
				offset += blocklength;
				r->seek_e(offset, p_abort);
			}

			return length>0 ? io_result_success : io_result_error_data;
		}
		catch(t_io_result code)
		{
			return code;
		}
	}

	void get_info_internal_e(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort)
	{
		t_io_result status;
		status = voc_test(r,p_location,info,p_abort);
		if (io_result_failed(status)) throw status;
		status = voc_parse(r,info,p_abort);
		if (io_result_failed(status)) throw status;
		info.set_length(length);
		info.info_set_int("bitrate",(int)((double)offset / length / 125.));
		info.info_set_int("samplerate",srate);
		info.info_set_int("channels",nch);
		if (bps>0) info.info_set_int("bitspersample",bps);
	}

	t_io_result voc_test(const service_ptr_t<file> & r,const playable_location & p_location,file_info & info,abort_callback & p_abort)
	{
		if (stricmp_utf8(string_extension_8(p_location.get_path()),"VOC")) return io_result_error_data;

		t_io_result io_status;

		t_filesize len;

		io_status = r->get_size(len,p_abort);
		if (io_result_failed(io_status)) return io_status;
		if (len == filesize_invalid) return io_result_error_generic;

		if (len>0x80000000) return io_result_error_data;
		if (len<26) return io_result_error_data;
		

		char foo[20];
		io_status = r->read_object(&foo,20,p_abort);
		if (io_result_failed(io_status)) return io_status;

		if (strncmp(foo, "Creative Voice File\x1A", 20)) return io_result_error_data;
		io_status = r->read_object(&foo,6,p_abort);
		if (io_result_failed(io_status)) return io_status;

		if (len < *(unsigned short*)&foo) return io_result_error_data;
		unsigned short blah = ~(((unsigned short*)&foo)[1]) + 0x1234;
		if (blah != ((unsigned short*)&foo)[2]) return io_result_error_data;
		return io_result_success;
	}

public:
	voc()
	{
	}
	~voc()
	{
	}
	
	inline static bool g_test_filename(const char * fn,const char * ext) {return !stricmp_utf8(ext,"VOC");}

	t_io_result get_info(const service_ptr_t<file> & p_reader, const playable_location & p_location, file_info & p_info, abort_callback & p_abort)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,false,true,false);
	}

	t_io_result open(const service_ptr_t<file> & p_reader,const playable_location & p_location, file_info & p_info,abort_callback & p_abort,unsigned p_flags)
	{
		return open_internal(p_reader,p_location,p_info,p_abort,true,!!(p_flags&OPEN_FLAG_GET_INFO),!!(p_flags&OPEN_FLAG_NO_LOOPING));
	}

	t_io_result open_internal(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info,bool p_noloop)
	{
		try {
			open_internal_e(p_file,p_location,p_info,p_abort,p_decode,p_want_info,p_noloop);
			return io_result_success;
		} catch(t_io_result status) {
			return status;
		}
	}

	void open_internal_e(const service_ptr_t<file> & p_file,const playable_location & p_location,file_info & p_info,abort_callback & p_abort,bool p_decode,bool p_want_info,bool p_noloop)
	{
		if (!p_file->can_seek()) throw io_result_error_data;
		get_info_internal_e(p_file,p_location,p_info,p_abort);
		if (p_decode)
		{
			no_loop = p_noloop ? 1 : 0;
			m_reader = p_file;
			blocklength = 0;
			loopstart = 0;
			loopcount = 0;
			tc = 0;
			packtype = 0;
			lastblocktype = 0;
			no_loop = 0;
			silence = 0;
			bps = 8;
			nch = 1;
			offset = 0;
			m_reader->seek_e(20,p_abort);
			m_reader->read_object_e(&offset, 2, p_abort);
			m_reader->seek_e(offset, p_abort);
		}		
	}

	virtual t_io_result run(audio_chunk * chunk,abort_callback & p_abort)
	{
		try {
			if (!blocklength)
			{
				int blocktype;
				bool done = FALSE;
					
				while (!done)
				{
					blocktype = 0;
					blocklength = 0;

					m_reader->read_object_e(&blocktype, 1 , p_abort);
					m_reader->read_object_e(&blocklength, 3, p_abort);

					offset += 4;

					switch (blocktype)
					{
					case 0:
						// End of data
						if (!loopstart || no_loop || (loopstart >= (offset - 4)))
						{
							return io_result_error_generic;
						}
						else
						{
							m_reader->seek_e(offset = loopstart, p_abort);
						}
						break;
					case 1:
						// Sound data block
						bps = 8;
						if (lastblocktype != 8)
						{
							tc = 0;
							m_reader->read_object_e(((char*)&tc)+1, 1, p_abort);
							m_reader->read_object_e(&packtype, 1, p_abort);
						}
						offset += 2;
						m_reader->seek_e(offset, p_abort);
						blocklength -= 2;

						srate = 256000000L / (65536 - tc);

						if (nch > 1) srate /= nch;

						if (packtype == 0) done = TRUE;
						break;
					case 2:
						// Sound continuation block
						done = TRUE;
						break;
					case 3:
						// Silence
						{
							int period = 0, thistc = 0, thisrate;
							m_reader->read_object_e(&period, 2, p_abort);
							m_reader->read_object_e(&thistc, 1, p_abort);
							offset += 3;
							blocklength -= 3;
							if (blocklength)
							{
								offset += blocklength;
								m_reader->seek_e(offset, p_abort);
							}

							thisrate = 1000000L / (256 - thistc);
							silence = ((int)((double)(period+1) / (double)thisrate * (double)srate)) * (bps/8) * nch;
							done = TRUE;
						}
						break;
	/*
					case 4:
						// Marker
						break;
					case 5:
						// ASCII string
						break;
	*/
						case 6:
						// Repeat begin
						if (!loopstart)
						{
							m_reader->read_object_e(&loopcount, 2, p_abort);
							offset += 2;
							blocklength -= 2;
							loopstart = offset + blocklength;
						}
						break;
					case 7:
						// Repeat end
						if (lastblocktype == 6)
						{
							loopcount = 0;
						}
						else
						{
							if ((loopcount > 0) && loopstart)
							{
								m_reader->seek_e(offset = loopstart, p_abort);
								if (loopcount < 0xffff)
								{
									loopcount--;
									if (loopcount == 0)
									{
										loopstart = 0;
									}
								}
								else
								{
									if (no_loop) done = TRUE;
								}
							}
						}
						break;
					case 8:
						// Extended block
						{
							int voicemode = 0;
							bps = 8;
							m_reader->read_object_e(&tc, 2, p_abort);
							m_reader->read_object_e(&packtype, 1, p_abort);
							m_reader->read_object_e(&voicemode, 1, p_abort);
							offset += 4;
							blocklength -= 4;
							if (voicemode == 0)
							{
								nch = 1;
							}
							else if (voicemode == 1)
							{
								nch = 2;
							}
							else
							{
								packtype = -1;
							}
						}
						break;
					case 9:
						// New sound data block
						{
							int format = 0;
							m_reader->read_object_e(&srate, 4, p_abort);
							m_reader->read_object_e(&bps, 1, p_abort);
							m_reader->read_object_e(&nch, 1, p_abort);
							m_reader->read_object_e(&format, 2, p_abort);
							offset += 12;
							m_reader->seek_e(offset, p_abort);
							blocklength -= 12;

							if (format == 0 || format == 4)
							{
								done = TRUE;
							}
						}
						break;
					}

					lastblocktype = blocktype;

					if (!done)
					{
						offset += blocklength;
						m_reader->seek_e(offset, p_abort);
					}
				}
			}

			if (blocklength > 0 || silence > 0)
			{
				unsigned out_size;
				void * out_buffer;
				
				if (silence)
				{
					out_size = min(silence, 576 * (bps/8) * nch);
					out_buffer = buffer.check_size(out_size);
					if (bps == 8) memset(out_buffer, 0x80, out_size);
					else memset(out_buffer, 0, out_size);
					silence -= out_size;
				}
				else
				{
					out_size = min(blocklength, 576 * (bps/8) * nch);
					out_buffer = buffer.check_size(out_size);
					m_reader->read_object_e(out_buffer,out_size,p_abort);
					offset += out_size;
					blocklength -= out_size;
				}
				if (out_size == 0) return io_result_error_generic;
				chunk->set_data_fixedpoint(out_buffer,out_size,srate,nch,bps,audio_chunk::g_guess_channel_config(nch));
				return io_result_success;
			}
			return io_result_eof;
		}
		catch(t_io_result bah)
		{
			return io_result_failed(bah) ? bah : io_result_error_generic;
		}		
	}

	virtual t_io_result seek(double seconds,abort_callback & p_abort)
	{
		return io_result_error_generic;
	}

	virtual bool can_seek()
	{
		return false;
	}
	inline static bool g_is_our_content_type(const char*,const char*) {return false;}
	inline static bool g_needs_reader() {return true;}

	t_io_result set_info(const service_ptr_t<file> &r,const playable_location & p_location,file_info & info, abort_callback & p_abort) {return io_result_error_data;}

	static GUID g_get_guid()
	{
		// {DC3076CE-69C5-429f-827F-F17B89E4692A}
		static const GUID guid = 
		{ 0xdc3076ce, 0x69c5, 0x429f, { 0x82, 0x7f, 0xf1, 0x7b, 0x89, 0xe4, 0x69, 0x2a } };
		return guid;
	}

	static const char * g_get_name() {return "VOC decoder";}

	inline static t_io_result g_get_extended_data(const service_ptr_t<file> & p_reader,const playable_location & p_location,const GUID & p_guid,stream_writer * p_out,abort_callback & p_abort) {return io_result_error_data;}

};

static input_factory_t<voc> g_input_voc_factory;


DECLARE_FILE_TYPE("VOC files","*.VOC");