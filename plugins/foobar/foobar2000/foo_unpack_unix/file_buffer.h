#ifndef _FILE_BUFFER_H_
#define _FILE_BUFFER_H_

class file_buffer : public file
{
	pfc::array_t<t_uint8> m_buffer;
	t_filesize m_position;
	t_filetimestamp m_timestamp;

public:
	file_buffer( t_filesize p_size, t_filetimestamp p_timestamp );

	// stream_reader
	virtual t_size read( void *, t_size, abort_callback & );

	// stream_writer
	virtual void write( const void *, t_size, abort_callback & );

	// file
	virtual t_filesize get_size( abort_callback & );
	virtual t_filesize get_position( abort_callback & );
	virtual void resize( t_filesize, abort_callback & );
	virtual void seek( t_filesize, abort_callback & );
	virtual bool can_seek();
	virtual bool get_content_type( pfc::string_base & );
	virtual bool is_in_memory();
	virtual void on_idle( abort_callback & );
	virtual t_filetimestamp get_timestamp( abort_callback & );
	virtual void reopen( abort_callback & );
	virtual bool is_remote();
};

#endif
