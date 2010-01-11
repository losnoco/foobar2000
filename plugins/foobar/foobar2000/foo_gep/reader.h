#include <foobar2000.h>
#include <Music_Emu.h>

class foobar_File_Reader : public Data_Reader
{
	const service_ptr_t<file> & m_file;
	abort_callback            & m_abort;
	t_filesize                  remain_;

public:
	foobar_File_Reader( const service_ptr_t<file> & p_file, abort_callback & p_abort );

	long read_avail( void *, long );
	error_t read( void *, long );
	long remain() const;
	error_t skip( long n );
};