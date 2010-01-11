/*
Copyright (C) 2005 NSRT Team ( http://nsrt.edgeemu.com )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef JMA_H
#define JMA_H

#include <fstream>
#include <vector>
#include <time.h>

#include <foobar2000.h>

#define _PFC_DECLARE_EXCEPTION(NAME,BASECLASS,DEFAULTMSG)	\
class NAME : public BASECLASS {	\
public:	\
	NAME() : BASECLASS(DEFAULTMSG) {}	\
	NAME(const char * p_msg) : BASECLASS(p_msg) {}	\
	NAME(const NAME & p_source) : BASECLASS(p_source) {}	\
};

namespace JMA
{
  PFC_DECLARE_EXCEPTION( exception_jma_no_create, exception_io_data, "JMA could not be created" );
  _PFC_DECLARE_EXCEPTION( exception_jma_no_mem_alloc, std::bad_alloc, "Memory for JMA could not be allocated" );
  PFC_DECLARE_EXCEPTION( exception_jma_no_open, exception_io_data, "JMA could not be opened" );
  PFC_DECLARE_EXCEPTION( exception_jma_bad_file, exception_io_data, "Invalid/Corrupt JMA" );
  PFC_DECLARE_EXCEPTION( exception_jma_unsupported_version, exception_io_data, "JMA version not supported" );
  PFC_DECLARE_EXCEPTION( exception_jma_compress_failed, exception_io_data, "JMA compression failed" );
  PFC_DECLARE_EXCEPTION( exception_jma_decompress_failed, exception_io_data, "JMA decompression failed" );
  PFC_DECLARE_EXCEPTION( exception_jma_file_not_found, exception_io_not_found, "File not found in JMA" );
  
  struct jma_file_info_base
  {
    pfc::string8 name;
    pfc::string8 comment;
    size_t size;
    unsigned int crc32;
  };
                    
  struct jma_public_file_info : jma_file_info_base
  {
    t_filetimestamp datetime;
  };
  
  struct jma_file_info : jma_file_info_base
  {
    unsigned short date;
    unsigned short time;
  };

  template<class jma_file_type>
  inline t_filesize get_total_size(std::vector<jma_file_type>& files)
  {
    t_filesize size = 0;
    for (typename std::vector<jma_file_type>::iterator i = files.begin(); i != files.end(); i++)
    {
      size += i->size; //We do have a problem if this wraps around
    }
  
    return(size);
  }

  class jma_open
  {
    public:
    jma_open( service_ptr_t<file> const&, abort_callback & ) throw(std::exception);
    ~jma_open();
    
	// let us pander to archive_impl
	t_filestats get_stats_in_archive( const char * p_file, abort_callback & p_abort ) throw(std::exception);
	void open_archive( service_ptr_t<file> & p_out, const char * file, abort_callback & p_abort ) throw(std::exception);

	// and the basic archive
	void archive_list( archive * owner, const char * path, archive_callback & p_out, bool p_want_readers ) throw(std::exception);
    
    private:
    service_ptr_t<file> stream;
    std::vector<jma_file_info> files;
    size_t chunk_size;
  
    void chunk_seek( unsigned int, abort_callback & ) throw(std::exception);
    void retrieve_file_block( abort_callback & ) throw(std::exception);
  };
}
#endif
