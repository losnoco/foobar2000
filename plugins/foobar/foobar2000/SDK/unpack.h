#ifndef _UNPACK_H_
#define _UNPACK_H_

class NOVTABLE unpacker : public service_base
{
protected:
	//override this
	virtual t_io_result open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p_source,abort_callback & p_abort)=0;
public:
	static const GUID class_guid;
	static inline const GUID & get_class_guid() {return class_guid;}

	static t_io_result g_open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p_source,abort_callback & p_abort);
};

/*
usage:
you have a reader to an zip/rar/gzip/whatever archive containing just a single file you want to read, eg. a module
do unpacker::g_open() on that reader
returns 0 on failure (not a known archive or cant read it) or pointer to a new reader reading contents of that archive on success
*/

#endif