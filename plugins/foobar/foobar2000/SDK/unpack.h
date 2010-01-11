#ifndef _UNPACK_H_
#define _UNPACK_H_

class NOVTABLE unpacker : public service_base {
public:
	//! Attempts to open specified file for unpacking, creates interface to virtual file with uncompressed data on success. When examined file doesn't appear to be one of formats supported by this unpacker implementation, throws exception_io_data.
	//! @param p_out Receives interface to virtual file with uncompressed data on success.
	//! @param p_source Source file to process.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	virtual void open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p_source,abort_callback & p_abort) = 0;

	//! Static helper querying existing unpacker implementations until one that successfully opens specified file is found. Attempts to open specified file for unpacking, creates interface to virtual file with uncompressed data on success. When examined file doesn't appear to be one of formats supported by registered unpacker implementations, throws exception_io_data.
	//! @param p_out Receives interface to virtual file with uncompressed data on success.
	//! @param p_source Source file to process.
	//! @param p_abort abort_callback object signaling user aborting the operation.
	static void g_open(service_ptr_t<file> & p_out,const service_ptr_t<file> & p_source,abort_callback & p_abort);

	static const GUID class_guid;

	virtual bool FB2KAPI service_query(service_ptr_t<service_base> & p_out,const GUID & p_guid) {
		if (p_guid == class_guid) {p_out = this; return true;}
		else return service_base::service_query(p_out,p_guid);
	}
protected:
	unpacker() {}
	~unpacker() {}
};

#endif