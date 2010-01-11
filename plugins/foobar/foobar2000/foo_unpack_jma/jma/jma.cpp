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

#include "jma.h"

#include "portable.h"
#include "7z.h"
#include "crc32.h"

class ISequentialInStream_File : public ISequentialInStream
{
  service_ptr_t<file> data;
  abort_callback & m_abort;
public:
  ISequentialInStream_File(service_ptr_t<file> const& Adata, abort_callback & p_abort) : data(Adata), m_abort(p_abort) { }
  
  HRESULT Read( void *aData, UINT32 aSize, UINT32 *aProcessedSize )
  {
    *aProcessedSize = data->read( aData, aSize, m_abort );
    return(S_OK);
  }
};

class ISequentialOutStream_File : public ISequentialOutStream
{
protected:
  service_ptr_t<file> data;
  abort_callback & m_abort;
  unsigned int total;
public:
  ISequentialOutStream_File(service_ptr_t<file> const& Adata, abort_callback & p_abort) : data(Adata), m_abort(p_abort), total(0) { }

  bool overflow_get() const { return(false); }
  unsigned int size_get() const { return(total); }

  HRESULT Write( const void *aData, UINT32 aSize, UINT32 *aProcessedSize )
  {
    *aProcessedSize = aSize;
    data->write( aData, aSize, m_abort );
    total += aSize;
    return(S_OK);
  }
};

class ISequentialOutStreamCRC32_File : public ISequentialOutStream_File
{
  unsigned int crc32, accept, skip, skipped;
public:
  ISequentialOutStreamCRC32_File( service_ptr_t<file> const& Adata, unsigned int Askip, unsigned int Aaccept, abort_callback & p_abort )
    : crc32( 0 ), accept( Aaccept ), skip( Askip ), skipped( 0 ), ISequentialOutStream_File( Adata, p_abort ) { }
  
  unsigned int size_get() const { return(total + skipped); }
  unsigned int size_get_real() const { return(total); }

  unsigned int crc32_get() const { return(crc32); }

  HRESULT Write( const void *aData, UINT32 aSize, UINT32 *aProcessedSize )
  {
    if ( aSize <= skip || total >= accept )
    {
      skip -= aSize;
      skipped += aSize;
      *aProcessedSize = aSize;
      return(S_OK);
    }
    else
    {
      *aProcessedSize = skip;
      aSize -= skip;
      skipped += skip;
      aData = ( const char * ) aData + skip;
      skip = 0;

      unsigned int remain = accept - total;
      if ( remain > aSize ) remain = aSize;

      UINT32 processed = 0;
      ISequentialOutStream_File::Write( aData, remain, &processed );
      crc32 = CRC32lib::CRC32( ( unsigned char * ) aData, processed, ~crc32 );
      *aProcessedSize += processed + ( aSize - remain ) ;
      return(S_OK);
    }
  }
};

struct file_set
{
  pfc::string8 name;
  t_filestats stats;
  unsigned int crc32;

  file_set & operator = ( const file_set & p )
  {
    name = p.name;
    stats = p.stats;
    crc32 = p.crc32;
    return *this;
  }
};

// Oh, the lengths I'll go to see that not all of the data is unpacked to memory...
class ISequentialOutStream_FileSet : public ISequentialOutStream_File
{
  bool overflow;
  unsigned int crc32;
  unsigned int total_offset, total_offset_ret;
  std::vector< file_set > & set;
  std::vector< file_set >::iterator it;

  archive_callback & m_out;
  archive * owner;
  const char * archive_path;

  pfc::string8_fastalloc temp;

public:
  ISequentialOutStream_FileSet( archive * p_owner, const char * p_archive_path, std::vector< file_set > & p_set, archive_callback & p_out )
    : owner( p_owner ), archive_path( p_archive_path ), set( p_set ), m_out( p_out ), overflow( false ), 
      crc32( 0 ), total_offset( 0 ), total_offset_ret( 0 ), ISequentialOutStream_File( data, p_out )
  {
    it = set.begin();
    filesystem::g_open_tempmem( data, m_abort );
  }

  bool overflow_get() const { return(overflow); }

  unsigned int size_get() const { return(total - total_offset_ret); }

  unsigned int size_get_total() const { return(total); }

  void Reset() { total_offset_ret = total; }

  HRESULT Write( const void *aData, UINT32 aSize, UINT32 *aProcessedSize )
  {
    m_out.check();

    *aProcessedSize = 0;

    if ( it == set.end() )
    {
      overflow = true;
      return(S_FALSE);
    }

    unsigned int remain = it->stats.m_size - total - total_offset;
    while ( remain <= aSize )
    {
      UINT32 processed_size = 0;
      ISequentialOutStream_File::Write( aData, remain, &processed_size );
      *aProcessedSize += processed_size;
      if ( processed_size != remain )
        return(S_FALSE);

      crc32 = CRC32lib::CRC32( ( const unsigned char * ) aData, remain, ~crc32 );
      if ( crc32 != it->crc32 )
        return(S_FALSE);

      data->reopen( m_abort );

      archive_impl::g_make_unpack_path( temp, archive_path, it->name, "jma" );
      if ( ! m_out.on_entry( owner, temp, it->stats, data ) )
        return(S_FALSE);

      data.release();

      if ( ++it == set.end() )
      {
        aSize -= processed_size;
        if ( aSize )
        {
          overflow = true;
          return(S_FALSE);
        }
        else return(S_OK);
      }

      filesystem::g_open_tempmem( data, m_abort );

      crc32 = 0;
      total_offset = total;
      aData = ( const char * ) aData + processed_size;
      aSize -= processed_size;
      remain = it->stats.m_size;
    }

    if ( aSize )
    {
      UINT32 processed_size = 0;
      ISequentialOutStream_File::Write( aData, aSize, &processed_size );
      *aProcessedSize += processed_size;
      crc32 = CRC32lib::CRC32( ( const unsigned char * ) aData, processed_size, ~crc32 );
      return(S_OK);
    }

    return(S_OK);
  }
};

namespace JMA
{
  const char jma_magic[] = { 'J', 'M', 'A', 0, 'N' };
  const unsigned int jma_header_length = 5;  
  const unsigned char jma_version = 1;
  const unsigned int jma_version_length = 1;
  const unsigned int jma_total_header_length = jma_header_length + jma_version_length + UINT_SIZE;
  
  //Convert DOS/zip/JMA integer time to to t_filetimestamp
  t_filetimestamp uint_to_filetimestamp(unsigned short date, unsigned short time)
  {
    SYSTEMTIME formatted_time = {0};
    FILETIME local_time, utc_time;
    t_filetimestamp ret;
  
    formatted_time.wDay    = date & 0x1F;
    formatted_time.wMonth  = (date >> 5) & 0xF;
    formatted_time.wYear   = ((date >> 9) & 0x7f) + 1980;
    formatted_time.wSecond = (time & 0x1F) * 2;
    formatted_time.wMinute = (time >> 5) & 0x3F;
    formatted_time.wHour   = (time >> 11) & 0x1F;

    if ( ! SystemTimeToFileTime( &formatted_time, &local_time ) ||
         ! LocalFileTimeToFileTime( &local_time, &utc_time ) )
      throw exception_win32( GetLastError() );

    ret = ( t_filetimestamp ) utc_time.dwHighDateTime << 32;
    ret |= utc_time.dwLowDateTime;

    return ret;
  }
    
  
  //Retreive the file block
  void jma_open::retrieve_file_block( abort_callback & p_abort ) throw(std::exception)
  {
    union
    {
      t_uint8  byte;
      t_uint16 ushort;
      t_uint32 uint;
    };

    //Retrieve file size
    t_filesize jma_file_size = stream->get_size_ex( p_abort );

    if ( jma_file_size < sizeof( uint ) )
    {
      throw exception_jma_bad_file();
    }
    
    //File block size is the last UINT in the file
    stream->seek( jma_file_size - 4, p_abort );
    stream->read_bendian_t( uint, p_abort );
    size_t file_block_size = uint;
    
    //The file block can't be larger than the JMA file without it's header.
    //This if can probably be improved
    if ( file_block_size >= jma_file_size - jma_total_header_length )
    {
      throw exception_jma_bad_file();
    }
    
    //Seek to before file block so we can read the file block
    stream->seek( jma_file_size - file_block_size - sizeof( uint ), p_abort );
    
    //This is needed if the file block is compressed
    service_ptr_t< file > decompressed_file_block;
    //Pointer to where to read file block from (file or decompressed buffer)
    service_ptr_t< file > file_block_stream;
    
    //Setup file info buffer and byte to read with
    jma_file_info file_info;
    
    stream->read_object_t( byte, p_abort );
    if ( !byte ) //If file block is compressed
    {
      //Compressed size isn't counting the byte we just read or the UINT for compressed size
      size_t compressed_size = file_block_size - ( 1 + sizeof( uint ) );
      
      //Read decompressed size / true file block size
      stream->read_bendian_t( uint, p_abort );
      file_block_size = uint;
      
      //Setup access methods for decompression
      filesystem::g_open_tempmem( decompressed_file_block, p_abort );

      ISequentialInStream_File compressed_data( stream, p_abort );
      ISequentialOutStream_File decompressed_data( decompressed_file_block, p_abort );
      
      //Decompress the data
      if ( ! decompress_lzma_7z( compressed_data, compressed_size, decompressed_data, file_block_size ) )
      {
        throw exception_jma_decompress_failed();
      }   
      
      //Go to beginning, setup pointer to buffer
      decompressed_file_block->reopen( p_abort );
      file_block_stream = decompressed_file_block;
    }
    else
    {
      stream->seek_ex( -1, file::seek_from_current, p_abort );
      file_block_stream = stream;
    }
    
    //Minimum file name length is 2 bytes, a char and a null
    //Minimum comment length is 1 byte, a null
    //There are currently 2 UINTs and 2 USHORTs per file
    while ( file_block_size >= 2 + 1 + sizeof(uint) * 2 + sizeof(ushort) * 2 ) //This does allow for a gap, but that's okay
    {
      //First stored in the file block is the file name null terminated
      file_info.name = "";
      
      file_block_stream->read_object_t( byte, p_abort );
      while ( byte )
      {
        file_info.name.add_byte( byte );
        file_block_stream->read_object_t( byte, p_abort );
      }

      //There must be a file name or the file is bad
      if ( !file_info.name.length() )
      {
        throw exception_jma_bad_file();
      }
      
      //Same trick as above for the comment
      file_info.comment = "";

      file_block_stream->read_object_t( byte, p_abort );
      while ( byte )
      {
        file_info.comment.add_byte( byte );
        file_block_stream->read_object_t( byte, p_abort );
      }
      
      //Next is a UINT representing the file's size
      file_block_stream->read_bendian_t( uint, p_abort );
      file_info.size = uint;
      
      //Followed by CRC32
      file_block_stream->read_bendian_t( uint, p_abort );
      file_info.crc32 = uint;
      
      //Special USHORT representation of file's date
      file_block_stream->read_bendian_t( ushort, p_abort );
      file_info.date = ushort;
      
      //Special USHORT representation of file's time
      file_block_stream->read_bendian_t( ushort, p_abort );
      file_info.time = ushort;
      
      files.push_back( file_info ); //Put file info into our structure
      
      //Subtract size of the file info we just read
      file_block_size -= file_info.name.length() + file_info.comment.length() + 2 + sizeof( uint ) * 2 + sizeof( ushort ) * 2;
    }
  }
  
  //Constructor for opening JMA files for reading
  jma_open::jma_open( service_ptr_t<file> const& p_file, abort_callback & p_abort ) throw (std::exception) 
  {
    if ( p_file.is_empty() )
    {
      throw exception_jma_no_open();
    }

    stream = p_file;

    //Header is "JMA\0N"
    union
    {
      t_uint8 header [jma_header_length];
      t_uint32 uint;
    };
    stream->read_object( header, jma_header_length, p_abort );
    if ( memcmp( jma_magic, header, jma_header_length ) )
    {
      throw exception_jma_bad_file();
    }
    
    stream->read_object_t( *header, p_abort );
    if ( *header <= jma_version )
    {
      stream->read_bendian_t( uint, p_abort );
      chunk_size = uint; //Chunk size is a UINT that follows version #
      retrieve_file_block( p_abort );
    }
    else
    {
      throw exception_jma_unsupported_version();
    }
  }

  jma_open::~jma_open()
  {
  }

  //Skip forward a given number of chunks
  void jma_open::chunk_seek( unsigned int chunk_num, abort_callback & p_abort ) throw(std::exception)
  {
    //Check the stream is open
    if (stream.is_empty())
    {
      throw exception_jma_no_open();
    }
   
    //Move forward over header
    stream->seek( jma_total_header_length, p_abort );

    t_uint32 int4;
    
    while ( chunk_num-- )
    {
      //Read in size of chunk
      stream->read_bendian_t( int4, p_abort );
        
      //Skip chunk plus it's CRC32
      stream->seek_ex( int4 + sizeof( int4 ), file::seek_from_current, p_abort );
    }
  }

  t_filestats jma_open::get_stats_in_archive( const char * p_file, abort_callback & p_abort ) throw(std::exception)
  {
    for ( std::vector<jma_file_info>::iterator i = files.begin(); i != files.end(); ++i )
    {
      p_abort.check();
      if ( !strcmp( i->name, p_file ) )
      {
        t_filestats ret = { i->size, uint_to_filetimestamp( i->date, i->time ) };
        return ret;
      }
    }

    throw exception_jma_file_not_found();
  }

  void jma_open::open_archive( service_ptr_t<file> & p_out, const char * file, abort_callback & p_abort ) throw(std::exception)
  {
    t_filesize size_to_skip = 0;
    t_filesize file_size = 0;
    t_uint32 crc32;

    for ( std::vector<jma_file_info>::iterator i = files.begin(); i != files.end(); ++i )
    {
      if ( !strcmp( i->name, file ) )
      {
        file_size = i->size;
        crc32 = i->crc32;
        break;
      }

      //Keep a running total of size
      size_to_skip += i->size;
    }

    if ( ! file_size )
      throw exception_jma_file_not_found();

    filesystem::g_open_tempmem( p_out, p_abort );

    ISequentialInStream_File compressed_data( stream, p_abort );

    if ( chunk_size )
    {
      unsigned int chunks_to_skip = size_to_skip / chunk_size;

      chunk_seek( chunks_to_skip, p_abort );

      pfc::array_t<t_uint8> compressed_buffer;
      ISequentialOutStreamCRC32_File decompressed_data( p_out, size_to_skip % chunk_size, file_size, p_abort );

      while ( decompressed_data.size_get_real() < file_size )
      {
        t_uint32 compressed_size, crc32;

        stream->read_bendian_t( compressed_size, p_abort );
        compressed_buffer.set_size( compressed_size );
        stream->read_object( compressed_buffer.get_ptr(), compressed_size, p_abort );
        stream->read_bendian_t( crc32, p_abort );

        if ( CRC32lib::CRC32( compressed_buffer.get_ptr(), compressed_size ) != crc32 )
          throw exception_jma_bad_file();

        ISequentialInStream_Array compressed_data( ( const char * ) compressed_buffer.get_ptr(), compressed_size );

        size_t decompressed_size = file_size - decompressed_data.size_get_real();
        if ( decompressed_size > chunk_size ) decompressed_size = chunk_size;

        if ( ! decompress_lzma_7z( compressed_data, compressed_size, decompressed_data, decompressed_size ) )
          throw exception_jma_decompress_failed();
      }

      if ( decompressed_data.crc32_get() != crc32 )
        throw exception_jma_bad_file();
    }
    else
    {
      t_uint32 compressed_size;

      chunk_seek( 0, p_abort );

      stream->read_bendian_t( compressed_size, p_abort );

      ISequentialOutStreamCRC32_File decompressed_data( p_out, size_to_skip, file_size, p_abort );

      if ( ! decompress_lzma_7z( compressed_data, compressed_size, decompressed_data, file_size + size_to_skip ) )
        throw exception_jma_decompress_failed();

      if ( decompressed_data.crc32_get() != crc32 )
        throw exception_jma_bad_file();
    }

    p_out->reopen( p_abort );
  }

  void jma_open::archive_list( archive * owner, const char * path, archive_callback & p_out, bool p_want_readers ) throw(std::exception)
  {
    service_ptr_t<file> m_reader;
    pfc::string8_fastalloc m_url;
    t_filestats m_stats;
    if ( ! p_want_readers )
    {
      for ( std::vector<jma_file_info>::iterator i = files.begin(); i != files.end(); ++i )
      {
        p_out.check();
        m_stats.m_size = i->size;
        m_stats.m_timestamp = uint_to_filetimestamp( i->date, i->time );
        archive_impl::g_make_unpack_path( m_url, path, i->name, "jma" );
        if ( !p_out.on_entry( owner, m_url, m_stats, m_reader ) ) break;
      }
    }
    else
    {
      std::vector< file_set > set;

      file_set file;

      for ( std::vector<jma_file_info>::iterator i = files.begin(); i != files.end(); ++i )
      {
        p_out.check();
        file.name = i->name;
        file.stats.m_size = i->size;
        file.stats.m_timestamp = uint_to_filetimestamp( i->date, i->time );
        file.crc32 = i->crc32;
        set.push_back( file );
      }

      t_filesize total_size = get_total_size( files );

      ISequentialOutStream_FileSet decompressed_data( owner, path, set, p_out );

      chunk_seek( 0, p_out );

      if ( chunk_size )
      {
        while ( decompressed_data.size_get_total() < total_size )
        {
          decompressed_data.Reset();

          t_uint32 compressed_size, crc32;
          pfc::array_t<t_uint8> compressed_buffer;

          stream->read_bendian_t( compressed_size, p_out );
          compressed_buffer.set_size( compressed_size );
          stream->read_object( compressed_buffer.get_ptr(), compressed_size, p_out );
          stream->read_bendian_t( crc32, p_out );

          if ( CRC32lib::CRC32( compressed_buffer.get_ptr(), compressed_size ) != crc32 )
            throw exception_jma_bad_file();

          ISequentialInStream_Array compressed_data( ( const char * ) compressed_buffer.get_ptr(), compressed_size );

          if ( ! decompress_lzma_7z( compressed_data, compressed_size, decompressed_data, chunk_size ) )
            throw exception_jma_decompress_failed();
        }
      }
      else
      {
        t_uint32 compressed_size;

        stream->read_bendian_t( compressed_size, p_out );

        ISequentialInStream_File compressed_data( stream, p_out );

        if ( ! decompress_lzma_7z( compressed_data, compressed_size, decompressed_data, total_size ) )
          throw exception_jma_decompress_failed();
      }
    }
  }
}


